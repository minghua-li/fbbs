#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <sys/uio.h>
#include "bbs.h"
#include "mmap.h"
#include "fbbs/brc.h"
#include "fbbs/convert.h"
#include "fbbs/helper.h"
#include "fbbs/mdbi.h"
#include "fbbs/post.h"
#include "fbbs/session.h"
#include "fbbs/string.h"

int post_index_cmp(const void *p1, const void *p2)
{
	const post_index_board_t *r1 = p1, *r2 = p2;
	return r1->id - r2->id;
}

int post_index_board_open_file(const char *file, record_perm_e rdonly, record_t *rec)
{
	return record_open(file, post_index_cmp, sizeof(post_index_board_t),
			rdonly, rec);
}

int post_index_board_open(int bid, record_perm_e rdonly, record_t *rec)
{
	char file[HOMELEN];
	snprintf(file, sizeof(file), "brdidx/%d", bid);
	return post_index_board_open_file(file, rdonly, rec);
}

int post_index_board_open_sticky(int bid, record_perm_e rdonly, record_t *rec)
{
	char file[HOMELEN];
	snprintf(file, sizeof(file), "brdidx/%d.sticky", bid);
	return post_index_board_open_file(file, rdonly, rec);
}

enum {
	POST_INDEX_BOARD_BUF_SIZE = 50,
};

int post_index_board_to_info(post_index_record_t *pir,
		const post_index_board_t *pib, post_info_t *pi, int count)
{
	post_index_t buf;
	for (int i = 0; i < count; ++i) {
		post_index_record_read(pir, pib->id, &buf);

		pi->id = pib->id;
		pi->reid = pib->id - pib->reid_delta;
		pi->tid = pib->id - pib->tid_delta;
		pi->flag = pib->flag;
		pi->uid = pib->uid;
		pi->stamp = buf.stamp;
		pi->bid = buf.bid;
		pi->replies = buf.replies;
		pi->comments = buf.comments;
		pi->score = buf.score;
		strlcpy(pi->owner, buf.owner, sizeof(pi->owner));
		strlcpy(pi->utf8_title, buf.utf8_title, sizeof(pi->utf8_title));
		pi->estamp = 0;
		pi->ename[0] = '\0';

		++pib;
		++pi;
	}
	return count;
}

int post_index_board_read(record_t *rec, int base, post_index_record_t *pir,
		post_info_t *buf, int size)
{
	if (record_seek(rec, base, RECORD_SET) < 0)
		return 0;

	int records = 0;
	post_index_board_t pib_buf[POST_INDEX_BOARD_BUF_SIZE];
	while (size > 0) {
		int max = POST_INDEX_BOARD_BUF_SIZE;
		max = size > max ? max : size;

		int count = record_read(rec, pib_buf, max);
		if (count <= 0)
			break;

		post_index_board_to_info(pir, pib_buf, buf, count);
		records += count;
		size -= max;
	}
	return records;
}

int post_index_trash_cmp(const void *p1, const void *p2)
{
	const post_index_trash_t *r1 = p1, *r2 = p2;
	int diff = r1->estamp - r2->estamp;
	if (diff)
		return diff;
	return r1->id - r2->id;
}

int post_index_trash_open(int bid, post_index_trash_e trash, record_t *rec)
{
	char file[HOMELEN];
	if (trash)
		snprintf(file, sizeof(file), "brdidx/%d.trash", bid);
	else
		snprintf(file, sizeof(file), "brdidx/%d.junk", bid);
	return record_open(file, post_index_trash_cmp, sizeof(post_index_trash_t),
			RECORD_WRITE, rec);
}

enum {
	POST_INDEX_PER_FILE = 100000,
};

void post_index_record_open(post_index_record_t *pir)
{
	pir->base = 0;
	pir->rdonly = RECORD_READ;
}

static int post_index_record_cmp(const void *p1, const void *p2)
{
	const post_index_t *pi1 = p1, *pi2 = p2;
	return pi1->id - pi2->id;
}

static int post_index_record_check(post_index_record_t *pir, post_id_t id,
		record_perm_e rdonly)
{
	post_id_t base = (id - 1) / POST_INDEX_PER_FILE * POST_INDEX_PER_FILE + 1;
	if (pir->base > 0 && pir->base == base && (rdonly || !pir->rdonly))
		return 0;

	if (pir->base > 0)
		record_close(&pir->record);

	pir->base = base;
	pir->rdonly = rdonly;

	char file[HOMELEN];
	snprintf(file, sizeof(file), "index/%"PRIdPID,
			(id - 1) / POST_INDEX_PER_FILE);
	return record_open(file, post_index_record_cmp, sizeof(post_index_t),
			rdonly, &pir->record);
}

int post_index_record_read(post_index_record_t *pir, post_id_t id,
		post_index_t *pi)
{
	if (post_index_record_check(pir, id, RECORD_READ) >= 0) {
		return record_read_after(&pir->record, pi, 1, id - pir->base);
	}
	memset(pi, 0, sizeof(*pi));
	return 0;
}

int post_index_record_update(post_index_record_t *pir, const post_index_t *pi)
{
	if (post_index_record_check(pir, pi->id, RECORD_WRITE) < 0)
		return 0;
	return record_write(&pir->record, pi, 1, pi->id - pir->base);
}

void post_index_record_get_title(post_index_record_t *pir, post_id_t id,
		char *buf, size_t size)
{
	if (post_index_record_check(pir, id, RECORD_READ) < 0) {
		buf[0] = '\0';
	} else {
		post_index_t pi;
		post_index_record_read(pir, id, &pi);
		strlcpy(buf, pi.utf8_title, size);
	}
}

static int post_index_record_lock(post_index_record_t *pir, record_lock_e lock,
		post_id_t id)
{
	return record_lock(&pir->record, lock, id - pir->base, RECORD_SET, 1);
}

void post_index_record_close(post_index_record_t *pir)
{
	if (pir->base >= 0)
		record_close(&pir->record);
	pir->base = 0;
	pir->rdonly = RECORD_READ;
}

enum {
	POST_CONTENT_PER_FILE = 10000,
};

typedef struct {
	uint32_t offset;
	uint32_t length;
} post_content_header_t;

static char *post_content_file_name(post_id_t id, char *file, size_t size)
{
	snprintf(file, size, "post/%"PRIdPID, (id - 1) / POST_CONTENT_PER_FILE);
	return file;
}

enum {
	POST_CONTENT_NOT_EXIST = 0,
	POST_CONTENT_NEED_LOCK = 1,
	POST_CONTENT_READ_ERROR = 2,
};

static char *post_content_try_get(int fd, post_id_t id, char *buf, size_t size)
{
	int relative_id = (id - 1) % POST_CONTENT_PER_FILE;

	post_content_header_t header;
	lseek(fd, relative_id * sizeof(header), SEEK_SET);
	file_read(fd, &header, sizeof(header));

	if (!header.offset) {
		buf[0] = POST_CONTENT_NOT_EXIST;
		return NULL;
	}

	if (!header.length) {
		buf[0] = '\0';
		return buf;
	}

	char *sbuf = buf;
	if (header.length >= size) {
		sbuf = malloc(header.length + 1);
	}

	char hdr[3];
	struct iovec vec[] = {
		{ .iov_base = hdr, .iov_len = sizeof(hdr) },
		{ .iov_base = sbuf, .iov_len = header.length + 1 },
	};
	lseek(fd, header.offset, SEEK_SET);
	int ret = readv(fd, vec, ARRAY_SIZE(vec));

	if (ret != sizeof(hdr) + header.length + 1) {
		if (sbuf != buf)
			free(sbuf);
		buf[0] = POST_CONTENT_READ_ERROR;
		return NULL;
	}

	uint16_t rel = *(uint16_t *) (hdr + 1);
	if (hdr[0] != '\n' || rel != relative_id || sbuf[header.length] != '\0') {
		if (sbuf != buf)
			free(sbuf);
		buf[0] = POST_CONTENT_NEED_LOCK;
		return NULL;
	}
	return sbuf;
}

char *post_content_get(post_id_t id, char *buf, size_t size)
{
	char file[HOMELEN];
	post_content_file_name(id, file, sizeof(file));

	int fd = open(file, O_RDONLY);
	if (fd < 0)
		return NULL;

	char *ptr = post_content_try_get(fd, id, buf, size);
	if (!ptr && buf[0] == POST_CONTENT_NEED_LOCK) {
		file_lock(fd, FILE_WRLCK, 0, FILE_SET, 0);
		ptr = post_content_try_get(fd, id, buf, size);
		file_lock(fd, FILE_UNLCK, 0, FILE_SET, 0);
	}

	close(fd);
	return ptr;
}

int post_content_write(post_id_t id, char *str, size_t size)
{
	char file[HOMELEN];
	post_content_file_name(id, file, sizeof(file));

	int fd = open(file, O_WRONLY | O_CREAT);
	if (fd < 0)
		return -1;

	struct stat st;
	if (file_lock(fd, FILE_WRLCK, 0, FILE_SET, 0) < 0) {
		close(fd);
		return -1;
	}

	if (fstat(fd, &st) < 0) {
		file_lock(fd, FILE_UNLCK, 0, FILE_SET, 0);
		close(fd);
		return -1;
	}

	uint32_t offset = st.st_size;
	if (offset < sizeof(uint32_t) * POST_CONTENT_PER_FILE)
		offset = sizeof(uint32_t) * POST_CONTENT_PER_FILE;

	post_id_t base = 1 + (id - 1) / POST_CONTENT_PER_FILE
			* POST_CONTENT_PER_FILE;
	uint16_t rel = id - base;
	lseek(fd, rel * sizeof(post_content_header_t), SEEK_SET);
	post_content_header_t header = { .offset = offset, .length = size };
	file_write(fd, &header, sizeof(header));

	char buf[3] = { '\n' };
	memcpy(buf + 1, &rel, sizeof(rel));
	struct iovec vec[] = {
		{ .iov_base = buf, .iov_len = sizeof(buf) },
		{ .iov_base = str, .iov_len = size + 1 },
	};
	lseek(fd, offset, SEEK_SET);
	int ret = writev(fd, vec, ARRAY_SIZE(vec));

	file_lock(fd, FILE_UNLCK, 0, FILE_SET, 0);
	close(fd);
	return ret;
}

static int post_sticky_filter(const void *ptr, void *fargs, int offset)
{
	const post_index_board_t *pib = ptr;
	post_id_t *id = fargs;
	return pib->id != *id;
}

int post_remove_sticky(int bid, post_id_t id)
{
	record_t record;
	if (post_index_board_open_sticky(bid, RECORD_WRITE, &record) < 0)
		return 0;
	int r = record_delete(&record, NULL, 0, post_sticky_filter, &id,
			NULL, NULL);
	record_close(&record);
	return r;
}

int post_add_sticky(int bid, const post_info_t *pi)
{
	record_t record;
	if (post_index_board_open_sticky(bid, RECORD_WRITE, &record) < 0)
		return 0;

	post_index_board_t pib = {
		.id = pi->id,
		.reid_delta = pi->id - pi->reid,
		.tid_delta = pi->id - pi->tid,
		.uid = pi->uid,
		.flag = pi->flag | POST_FLAG_STICKY,
	};

	record_lock_all(&record, RECORD_WRLCK);
	int r = 0;
	int count = record_count(&record);
	if (count < MAX_NOTICE) {
		if (record_search(&record, post_sticky_filter, &pib.id, -1, false) < 0)
			r = record_append(&record, &pib, 1);
	}
	record_lock_all(&record, RECORD_UNLCK);

	record_close(&record);
	return r;
}

const char *pid_to_base32(post_id_t pid, char *s, size_t size)
{
	if (!pid) {
		return "a";
	} else {
		char buf[PID_BUF_LEN];
		char *p = buf, *r = s;
		while (pid) {
			char c = pid & 0x1f;
			if (c < 26)
				c += 'a';
			else
				c += '2' - 26;

			*p++ = c;
			pid >>= 5;
		}

		--p;
		if (p > buf + size - 2)
			p = buf + size - 2;
		for (; p >= buf; --p) {
			*s++ = *p;
		}
		*s = '\0';
		return r;
	}
}

post_id_t base32_to_pid(const char *s)
{
	post_id_t pid = 0;
	for (char c = *s; (c = *s); ++s) {
		char d = 0;
		if (c >= 'a' && c <= 'z')
			d = c - 'a';
		else if (c >= '2' && c <= '7')
			d = c - '2' + 26;
		else
			return 0;

		pid <<= 5;
		pid += d;
	}
	return pid;
}

/**
 * Creates a new file in specific location.
 * @param[in] dir The directory.
 * @param[in] pfx Prefix of the file.
 * @param[in, out] fname The resulting filename.
 * @param[in] size The size of fname.
 * @return Filename and stream on success, NULL on error.
 * @see ::date_to_fname.
 */
static FILE *get_fname(const char *dir, const char *pfx,
		char *fname, size_t size)
{
	if (dir == NULL || pfx == NULL)
		return NULL;
	const char c[] = "ZYXWVUTSRQPONMLKJIHGFEDCBA";
	int t = (int)time(NULL);
	int count = snprintf(fname, size, "%s%s%d. ", dir, pfx, t);
	if (count < 0 || count >= size)
		return NULL;
	int fd;
	for (int i = sizeof(c) - 2; i >= 0; ++i) {
		fname[count - 1] = c[i];
		if ((fd = open(fname, O_CREAT | O_RDWR | O_EXCL, 0644)) > 0) {
			FILE *fp = fdopen(fd, "w+");
			if (fp) {
				return fp;
			} else {
				close(fd);
				return NULL;
			}
		}
	}
	return NULL;
}

char *convert_file_to_utf8_content(const char *file)
{
	char *utf8_content = NULL;
	mmap_t m = { .oflag = O_RDONLY };
	if (mmap_open(file, &m) == 0) {
		utf8_content = malloc(m.size * 2);
		convert(env_g2u, m.ptr, CONVERT_ALL, utf8_content, m.size * 2,
				NULL, NULL);
		mmap_close(&m);
	}
	return utf8_content;
}

static char *generate_content(const post_request_t *pr, const char *uname,
		const char *nick, const char *ip, bool anony)
{
	char dir[HOMELEN];
	snprintf(dir, sizeof(dir), "boards/%s/", pr->board->name);
	const char *pfx = "M.";

	char fname[HOMELEN];
	FILE *fptr;
	if ((fptr = get_fname(dir, pfx, fname, sizeof(fname))) == NULL)
		return NULL;

	//% "发信人: %s (%s), 信区: %s\n标  题: %s\n发信站: %s (%s)\n\n"
	fprintf(fptr, "\xb7\xa2\xd0\xc5\xc8\xcb: %s (%s), \xd0\xc5\xc7\xf8: %s\n\xb1\xea  \xcc\xe2: %s\n\xb7\xa2\xd0\xc5\xd5\xbe: %s (%s)\n\n",
			uname, nick, pr->board->name, pr->title, BBSNAME,
			getdatestring(time(NULL), DATE_ZH));

	if (pr->cp)
		convert_to_file(pr->cp, pr->content, CONVERT_ALL, fptr);
	else
		fputs(pr->content, fptr);

	if (!anony && pr->sig > 0)
		add_signature(fptr, uname, pr->sig);
	else
		fputs("\n--", fptr);

	if (ip) {
		char buf[2];
		fseek(fptr, -1, SEEK_END);
		fread(buf, 1, 1, fptr);
		if (buf[0] != '\n')
			fputs("\n", fptr);

		//% "\033[m\033[1;%2dm※ %s:·"
		fprintf(fptr, "\033[m\033[1;%2dm\xa1\xf9 %s:\xa1\xa4"BBSNAME" "BBSHOST
			//% "·HTTP [FROM: %s]\033[m\n"
			"\xa1\xa4HTTP [FROM: %s]\033[m\n", 31 + rand() % 7,
			//% "转载" "来源"
			pr->crosspost ? "\xd7\xaa\xd4\xd8" : "\xc0\xb4\xd4\xb4", ip);
	}

	fclose(fptr);

	return convert_file_to_utf8_content(fname);
}

#define LAST_FAKE_ID_KEY "last_fake_id"

int get_last_fake_pid(int bid)
{
	return mdb_integer(0, "HGET", LAST_FAKE_ID_KEY " %d", bid);
}

int incr_last_fake_pid(int bid, int delta)
{
	return mdb_integer(0, "HINCRBY", LAST_FAKE_ID_KEY " %d %d", bid, delta);
}

const char *post_recent_table(int bid)
{
	static char table[24];
	if (bid) {
		snprintf(table, sizeof(table), "posts.recent_%d", bid);
		return table;
	}
	return "posts.recent";
}

static post_id_t next_post_id(void)
{
	return mdb_integer(0, "INCR", "post_id_seq");
}

static post_id_t insert_post(const post_request_t *pr, const char *uname,
		const char *content)
{
	post_index_t pi = { .id = next_post_id(), };
	if (pi.id) {
		pi.reid_delta = pr->reid ? pi.id - pr->reid : 0;
		pi.tid_delta = pr->tid ? pi.id - pr->tid : 0;
		pi.stamp = time(NULL);
		pi.uid = get_user_id(uname);
		pi.flag = (pr->marked ? POST_FLAG_MARKED : 0)
				| (pr->locked ? POST_FLAG_LOCKED : 0);
		pi.bid = pr->board->id;
		strlcpy(pi.owner, uname, sizeof(pi.owner));
		convert_g2u(pr->title, pi.utf8_title);

		post_index_record_t pir;
		post_index_record_open(&pir);
		if (!post_index_record_update(&pir, &pi))
			pi.id = 0;
		post_index_record_close(&pir);
	}
	if (pi.id) {
		post_index_board_t pib = {
			.id = pi.id, .reid_delta = pi.reid_delta, .tid_delta = pi.tid_delta,
			.uid = pi.uid, .flag = pi.flag,
		};

		record_t record;
		post_index_board_open(pi.bid, RECORD_WRITE, &record);
		record_lock_all(&record, RECORD_WRLCK);
		if (record_append(&record, &pib, 1) < 0)
			pi.id = 0;
		record_lock_all(&record, RECORD_UNLCK);
		record_close(&record);
	}

	return pi.id;
}

/**
 * Publish a post.
 * @param pr The post request.
 * @return file id on success, -1 on error.
 */
post_id_t publish_post(const post_request_t *pr)
{
	if (!pr || !pr->title || (!pr->content && !pr->gbk_file) || !pr->board)
		return 0;

	bool anony = pr->anony && (pr->board->flag & BOARD_ANONY_FLAG);
	const char *uname = NULL, *nick = NULL, *ip = pr->ip;
	if (anony) {
		uname = ANONYMOUS_ACCOUNT;
		nick = ANONYMOUS_NICK;
		ip = ANONYMOUS_SOURCE;
	} else if (pr->user) {
		uname = pr->user->userid;
		nick = pr->user->username;
	} else if (pr->autopost) {
		uname = pr->uname;
		nick = pr->nick;
	}
	if (!uname || !nick)
		return 0;

	char *content;
	if (pr->gbk_file)
		content = convert_file_to_utf8_content(pr->gbk_file);
	else
		content = generate_content(pr, uname, nick, ip, anony);

	post_id_t pid = insert_post(pr, uname, content);
	free(content);

	if (pid) {
		set_last_post_id(pr->board->id, pid);

		if (!pr->autopost) {
			brc_fcgi_init(uname, pr->board->name);
			brc_mark_as_read(pid);
			brc_update(uname, pr->board->name);
		}
	}
	return pid;
}

enum {
	MAX_QUOTED_LINES = 5,     ///< Maximum quoted lines (for QUOTE_AUTO).
	/** A line will be truncated at this width (78 for quoted line) */
	TRUNCATE_WIDTH = 76,
};

/**
 * Find newline in [begin, end), truncate at TRUNCATE_WIDTH.
 * @param begin The head pointer.
 * @param end The off-the-end pointer.
 * @return Off-the-end pointer to the first (truncated) line.
 */
static const char *get_truncated_line(const char *begin, const char *end)
{
	const char *code = "[0123456789;";
	bool ansi = false;

	int width = TRUNCATE_WIDTH;
	if (end - begin >= 2 && *begin == ':' && *(begin + 1) == ' ')
		width += 2;

	for (const char *s = begin; s < end; ++s) {
		if (*s == '\n')
			return s + 1;

		if (*s == '\033') {
			ansi = true;
			continue;
		}

		if (ansi) {
			if (!memchr(code, *s, sizeof(code) - 1))
				ansi = false;
			continue;
		}

		if (*s & 0x80) {
			width -= 2;
			if (width < 0)
				return s;
			++s;
			if (width == 0)
				return (s + 1 > end ? end : s + 1);
		} else {
			if (--width == 0)
				return s + 1;
		}
	}
	return end;
}

/**
 * Tell if a line is meaningless.
 * @param begin The beginning of the line.
 * @param end The off-the-end pointer.
 * @return True if str is quotation of a quotation or contains only white
           spaces, false otherwise.
 */
static bool qualify_quotation(const char *begin, const char *end)
{
	const char *s = begin;
	if (end - s > 2 && (*s == ':' || *s == '>') && *(s + 1) == ' ') {
		s += 2;
		if (end - s > 2 && (*s == ':' || *s == '>') && *(s + 1) == ' ')
			return false;
	}

	while (s < end && (*s == ' ' || *s == '\t' || *s == '\r'))
		++s;

	return (s < end && *s != '\n');
}

typedef size_t (*filter_t)(const char *, size_t, FILE *);

static size_t default_filter(const char *s, size_t size, FILE *fp)
{
	return fwrite(s, size, 1, fp);
}

static const char *get_newline(const char *begin, const char *end)
{
	while (begin < end) {
		if (*begin++ == '\n')
			return begin;
	}
	return begin;
}

#define PRINT_CONST_STRING(s)  (*filter)(s, sizeof(s) - 1, fp)

static void quote_author(const char *begin, const char *lend, bool mail,
		FILE *fp, filter_t filter)
{
	const char *quser = begin, *ptr = lend;
	while (quser < lend) {
		if (*quser++ == ' ')
			break;
	}
	while (--ptr >= begin) {
		if (*ptr == ')')
			break;
	}
	++ptr;

	//% "\n【 在 "
	PRINT_CONST_STRING("\n\xa1\xbe \xd4\xda ");
	if (ptr > quser)
		(*filter)(quser, ptr - quser, fp);
	//% " 的"
	PRINT_CONST_STRING(" \xb5\xc4");
	if (mail)
		//% "来信"
		PRINT_CONST_STRING("\xc0\xb4\xd0\xc5");
	else
		//% "大作"
		PRINT_CONST_STRING("\xb4\xf3\xd7\xf7");
	//% "中提到: 】\n"
	PRINT_CONST_STRING("\xd6\xd0\xcc\xe1\xb5\xbd: \xa1\xbf\n");
}

/**
 * Make quotation from a string.
 * @param str String to be quoted.
 * @param size Size of the string.
 * @param output Output file. If NULL, will output to stdout (web).
 * @param mode Quotation mode. See QUOTE_* enums.
 * @param mail Whether the referenced post is a mail.
 * @param filter Output filter function.
 */
void quote_string(const char *str, size_t size, const char *output, int mode,
		bool mail, filter_t filter)
{
	FILE *fp = NULL;
	if (output) {
		if (!(fp = fopen(output, "w")))
			return;
	}

	if (!filter)
		filter = default_filter;

	const char *begin = str, *end = str + size;
	const char *lend = get_newline(begin, end);
	quote_author(begin, lend, mail, fp, filter);

	bool header = true, tail = false;
	size_t lines = 0;
	const char *ptr;
	while (1) {
		ptr = lend;
		if (ptr >= end)
			break;

		lend = get_truncated_line(ptr, end);
		if (header && *ptr == '\n') {
			header = false;
			continue;
		}

		if (lend - ptr == 3 && !memcmp(ptr, "--\n", 3)) {
			tail = true;
			if (mode == QUOTE_LONG || mode == QUOTE_AUTO)
				break;
		}

		if (!header || mode == QUOTE_ALL) {
			if ((mode == QUOTE_LONG || mode == QUOTE_AUTO)
					&& !qualify_quotation(ptr, lend)) {
				if (*(lend - 1) != '\n')
					lend = get_newline(lend, end);
				continue;
			}

			//% "※ 来源:·"
			if (mode == QUOTE_SOURCE && lend - ptr > 10 + sizeof("\xa1\xf9 \xc0\xb4\xd4\xb4:\xa1\xa4")
					//% "※ 来源:·" "※ 来源:·"
					&& !memcmp(ptr + 10, "\xa1\xf9 \xc0\xb4\xd4\xb4:\xa1\xa4", sizeof("\xa1\xf9 \xc0\xb4\xd4\xb4:\xa1\xa4"))) {
				break;
			}

			if (mode == QUOTE_AUTO) {
				if (++lines > MAX_QUOTED_LINES) {
					//% ": .................（以下省略）"
					PRINT_CONST_STRING(": .................\xa3\xa8\xd2\xd4\xcf\xc2\xca\xa1\xc2\xd4\xa3\xa9");
					break;
				}
			}

			if (mode != QUOTE_SOURCE)
				PRINT_CONST_STRING(": ");
			(*filter)(ptr, lend - ptr, fp);
			if (*(lend - 1) != '\n')
				PRINT_CONST_STRING("\n");
		}
	}
	if (fp)
		fclose(fp);
}

void quote_file_(const char *orig, const char *output, int mode, bool mail,
		filter_t filter)
{
	if (mode != QUOTE_NOTHING) {
		mmap_t m = { .oflag = O_RDONLY };
		if (mmap_open(orig, &m) == 0) {
			quote_string(m.ptr, m.size, output, mode, mail, filter);
			mmap_close(&m);
		}
	}
}

typedef struct {
	bool set;
	bool toggle;
	post_flag_e flag;
} post_index_board_update_flag_t;

static int match_filter(const post_index_board_t *pib,
		post_index_record_t *pir, const post_filter_t *filter, int offset)
{
	bool match = true;
	if (filter->uid)
		match &= pib->uid == filter->uid;
	if (filter->min)
		match &= pib->id >= filter->min;
	if (filter->max)
		match &= pib->id <= filter->max;
	if (filter->tid)
		match &= pib->id - pib->tid_delta == filter->tid;
	if (filter->flag)
		match &= (pib->flag & filter->flag) == filter->flag;
	if (filter->fake_id_min)
		match &= offset >= filter->fake_id_min - 1;
	if (filter->fake_id_max)
		match &= offset < filter->fake_id_max;
	if (filter->type == POST_LIST_TOPIC)
		match &= !pib->tid_delta;
	if (*filter->utf8_keyword) {
		UTF8_BUFFER(title, POST_TITLE_CCHARS);
		post_index_record_get_title(pir, pib->id, utf8_title,
				sizeof(utf8_title));
		match &= (bool) strcasestr(utf8_title, filter->utf8_keyword);
	}
	return match;
}

int post_index_board_filter(const void *pib, void *fargs, int offset)
{
	const post_index_board_filter_t *pibf = fargs;
	return match_filter(pib, pibf->pir, pibf->filter, offset) ? 0 : -1;
}

static int post_index_board_update_flag(void *ptr, void *uargs)
{
	post_index_board_t *pib = ptr;
	post_index_board_update_flag_t *pibuf = uargs;
	if (pibuf->toggle)
		pibuf->set = !(pib->flag & pibuf->flag);
	if (pibuf->set)
		pib->flag |= pibuf->flag;
	else
		pib->flag &= ~pibuf->flag;
	return 0;
}

int set_post_flag(record_t *rec, post_index_record_t *pir,
		post_filter_t *filter, post_flag_e flag, bool set, bool toggle)
{
	post_index_board_filter_t pibf = { .pir = pir, .filter = filter };
	post_index_board_update_flag_t pibuf = {
		.set = set, .toggle = toggle, .flag = flag,
	};
	return record_update(rec, NULL, 0, post_index_board_filter, &pibf,
			post_index_board_update_flag, &pibuf);
}

static int post_index_board_filter_one(const void *ptr, void *fargs,
		int offset)
{
	return post_index_cmp(ptr, fargs);
}

int set_post_flag_one(record_t *rec, post_index_board_t *pib, int offset,
		post_flag_e flag, bool set, bool toggle)
{
	post_index_board_update_flag_t pibuf = {
		.set = set, .toggle = toggle, .flag = flag,
	};
	return record_update(rec, pib, offset, post_index_board_filter_one, pib,
			post_index_board_update_flag, &pibuf);
}

int count_sticky_posts(int bid)
{
	db_res_t *r = db_query("SELECT count(*) FROM posts.recent"
			" WHERE board = %d AND sticky", bid);
	int rows = r ? db_get_bigint(r, 0, 0) : 0;
	db_clear(r);
	return rows;
}

void res_to_post_info(db_res_t *r, int i, bool archive, post_info_t *p)
{
	bool deleted = streq(db_field_name(r, 0), "did");
	p->id = db_get_post_id(r, i, 0);
	p->reid = db_get_post_id(r, i, 1);
	p->tid = db_get_post_id(r, i, 2);
	p->fake_id = db_get_is_null(r, i, 3) ? 0 : db_get_integer(r, i, 3);
	p->bid = db_get_integer(r, i, 4);
	p->uid = db_get_is_null(r, i, 5) ? 0 : db_get_user_id(r, i, 5);
	strlcpy(p->owner, db_get_value(r, i, 6), sizeof(p->owner));
	p->stamp = db_get_time(r, i, 7);
	p->flag = (db_get_bool(r, i, 8) ? POST_FLAG_DIGEST : 0)
			| (db_get_bool(r, i, 9) ? POST_FLAG_MARKED : 0)
			| (db_get_bool(r, i, 10) ? POST_FLAG_WATER : 0)
			| (db_get_bool(r, i, 11) ? POST_FLAG_LOCKED : 0)
			| (db_get_bool(r, i, 12) ? POST_FLAG_IMPORT : 0)
			| (deleted ? POST_FLAG_DELETED : 0)
			| (archive ? POST_FLAG_ARCHIVE : 0);
	p->replies = db_get_integer(r, i, 13);
	p->comments = db_get_integer(r, i, 14);
	p->score = db_get_integer(r, i, 15);
	strlcpy(p->utf8_title, db_get_value(r, i, 16), sizeof(p->utf8_title));

	if (deleted) {
		strlcpy(p->ename, db_get_value(r, i, 17), sizeof(p->ename));
		p->estamp = db_get_time(r, i, 18);
		p->flag |= db_get_bool(r, i, 19) ? POST_FLAG_JUNK : 0;
	}
}

int load_sticky_posts(int bid, post_info_t **posts)
{
	if (!*posts)
		*posts = malloc(sizeof(**posts) * MAX_NOTICE);

	db_res_t *r = db_query("SELECT " POST_LIST_FIELDS " FROM posts.recent"
			" WHERE board = %d AND sticky ORDER BY id DESC", bid);
	if (r) {
		int count = db_res_rows(r);
		for (int i = 0; i < count; ++i) {
			res_to_post_info(r, i, 0, *posts + i);
//			set_post_flag_local(*posts + i, POST_FLAG_STICKY, true);
		}
		db_clear(r);
		return count;
	}
	return 0;
}

bool is_deleted(post_list_type_e type)
{
	return type == POST_LIST_TRASH || type == POST_LIST_JUNK;
}

post_list_type_e post_list_type(const post_info_t *ip)
{
	return (ip->flag & POST_FLAG_DELETED) ? POST_LIST_TRASH : POST_LIST_NORMAL;
}

const char *post_archive_table(const post_filter_t *filter)
{
	static char table[24];
	if (filter->bid) {
		snprintf(table, sizeof(table), "posts.archive_%d", filter->bid);
		return table;
	}
	return "posts.archives";
}

/**
 * Get name of the database table by filter.
 * @param filter The post filter.
 * @return The table name.
 * @note This function will directly return partition table to the caller
 *       instead of parent table because of a 2x performance gain (as of
 *       PostgreSQL 9.2).
 */
const char *post_table_name(const post_filter_t *filter)
{
	if (filter->archive)
		return post_archive_table(filter);

	if (is_deleted(filter->type))
		return "posts.deleted";
	else
		return post_recent_table(filter->bid);
}

const char *post_table_index(const post_filter_t *filter)
{
	if (is_deleted(filter->type))
		return "did";
	else
		return "id";
}

/**
 * Generate post query for a given filter.
 * @param[out] q The query.
 * @param[in] f The post filter.
 * @param[in] asc Generate proper ORDER BY clause, or nothing if left NULL.
 */
void build_post_filter(query_t *q, const post_filter_t *f, const bool *asc)
{
	query_where(q, "TRUE");
	if (f->bid && is_deleted(f->type))
		query_and(q, "board = %d", f->bid);
	if (f->flag & POST_FLAG_DIGEST)
		query_and(q, "digest");
	if (f->flag & POST_FLAG_MARKED)
		query_and(q, "marked");
	if (f->flag & POST_FLAG_WATER)
		query_and(q, "water");
	if (f->uid)
		query_and(q, "owner = %"DBIdUID, f->uid);
	if (*f->utf8_keyword)
		query_and(q, "title ILIKE '%%' || %s || '%%'", f->utf8_keyword);
	if (f->type == POST_LIST_TOPIC)
		query_and(q, "id = tid");

	if (f->type == POST_LIST_THREAD) {
		if (f->min && f->tid) {
			query_and(q, "(tid = %"DBIdPID" AND id >= %"DBIdPID
					" OR tid > %"DBIdPID")", f->tid, f->min, f->tid);
		}
		if (f->max && f->tid) {
			query_and(q, "(tid = %"DBIdPID" AND id <= %"DBIdPID
					" OR tid < %"DBIdPID")", f->tid, f->max, f->tid);
		}
	} else {
		if (f->min) {
			query_and(q, post_table_index(f));
			query_append(q, ">= %"DBIdPID, f->min);
		}
		if (f->max) {
			query_and(q, post_table_index(f));
			query_append(q, "<= %"DBIdPID, f->max);
		}
		if (f->tid)
			query_and(q, "tid = %"DBIdPID, f->tid);
		if (f->fake_id_min)
			query_and(q, "fake_id >= %d", f->fake_id_min);
		if (f->fake_id_max)
			query_and(q, "fake_id <= %d", f->fake_id_max);
	}

	if (f->type == POST_LIST_TRASH)
		query_and(q, "AND", "bm_visible");
	if (f->type == POST_LIST_JUNK)
		query_and(q, "NOT bm_visible");

	if (asc) {
		query_append(q, "ORDER BY");
		if (f->type == POST_LIST_THREAD) {
			if (*asc)
				query_append(q, "tid,");
			else
				query_append(q, "tid DESC,");
		}
		query_append(q, post_table_index(f));
		if (!*asc)
			query_append(q, "DESC");
	}
}

static const char *post_list_fields(const post_filter_t *filter)
{
	if (is_deleted(filter->type))
		return "d"POST_LIST_FIELDS",ename,deleted,junk";
	return POST_LIST_FIELDS;
}

query_t *build_post_query(const post_filter_t *filter, bool asc, int limit)
{
	query_t *q = query_new(0);
	query_select(q, post_list_fields(filter));
	query_from(q, post_table_name(filter));
	build_post_filter(q, filter, &asc);
	query_limit(q, limit);
	return q;
}

void res_to_post_info_full(db_res_t *res, int row, bool archive,
		post_info_full_t *p)
{
	res_to_post_info(res, row, archive, &p->p);
	p->res = res;
	p->content = db_get_value(res, row, 17);
	p->length = db_get_length(res, row, 17);
}

void free_post_info_full(post_info_full_t *p)
{
	db_clear(p->res);
}

int dump_content_to_gbk_file(const char *utf8_str, size_t length, char *file,
		size_t size)
{
	snprintf(file, size, "tmp/gbk_dump.%d", getpid());
	FILE *fp = fopen(file, "w");
	if (!fp)
		return -1;
	convert_to_file(env_u2g, utf8_str, length, fp);
	fclose(fp);
	return 0;
}

#define LAST_POST_KEY  "last_post"

bool set_last_post_id(int bid, post_id_t pid)
{
	mdb_res_t *res = mdb_cmd("HSET", LAST_POST_KEY " %d %"PRIdPID, bid, pid);
	mdb_clear(res);
	return res;
}

post_id_t get_last_post_id(int bid)
{
	return mdb_integer(0, "HGET", LAST_POST_KEY " %d", bid);
}

static void adjust_user_post_count(const char *uname, int delta)
{
	struct userec urec;
	int unum = searchuser(uname);
	getuserbyuid(&urec, unum);
	urec.numposts += delta;
	substitut_record(NULL, &urec, sizeof(urec), unum);
}

typedef struct {
	post_index_record_t *pir;
	bool delete_;
} post_deletion_callback_t;

static int post_deletion_callback(void *ptr, void *args)
{
	post_index_trash_t *pit = ptr;
	post_deletion_callback_t *pdc = args;
	post_index_t pi;

	post_index_record_lock(pdc->pir, RECORD_WRLCK, pit->id);
	post_index_record_read(pdc->pir, pit->id, &pi);
	if (pdc->delete_)
		pi.flag |= POST_FLAG_DELETED;
	else
		pi.flag &= ~POST_FLAG_DELETED;
	post_index_record_update(pdc->pir, &pi);
	post_index_record_lock(pdc->pir, RECORD_UNLCK, pit->id);

	if (pit->flag & POST_FLAG_JUNK)
		adjust_user_post_count(pi.owner, pdc->delete_ ? -1 : 1);
	return 0;
}

static int post_deletion_trigger(record_t *trash, const post_filter_t *filter,
		post_index_record_t *pir, bool deletion)
{
	post_index_board_filter_t pibf = { .pir = pir, .filter = filter, };
	post_deletion_callback_t pdc = { .pir = pir, .delete_ = deletion, };
	int rows = record_update(trash, NULL, 0, post_index_board_filter, &pibf,
			post_deletion_callback, &pdc);
	return rows;
}

typedef struct {
	record_t *trash;
	const char *ename;
	fb_time_t estamp;
	bool junk;
	bool decrease;
} post_index_trash_insert_t;

static int post_index_trash_insert(void *rec, void *cargs)
{
	const post_index_board_t *pib = rec;
	post_index_trash_insert_t *piti = cargs;
	post_index_trash_t pit = {
		.id = pib->id,
		.reid_delta = pib->reid_delta,
		.tid_delta = pib->tid_delta,
		.uid = pib->uid,
		.flag = pib->flag,
		.estamp = piti->estamp,
	};
	if (piti->decrease && (piti->junk || (pib->flag & POST_FLAG_WATER)))
		pit.flag |= POST_FLAG_JUNK;
	strlcpy(pit.ename, piti->ename, sizeof(pit.ename));
	return record_append(piti->trash, &pit, 1);
}

static int post_index_board_filter_unmarked(const void *ptr, void *fargs,
		int offset)
{
	const post_index_board_t *pib = ptr;
	const post_index_board_filter_t *pibf = fargs;
	return !(pib->flag & POST_FLAG_MARKED)
			&& match_filter(pib, pibf->pir, pibf->filter, offset);
}

int post_index_board_delete(const post_filter_t *filter, void *ptr, int offset,
		bool junk, bool bm_visible, bool force)
{
	if (!filter->bid)
		return 0;

	bool decrease = true;
	board_t board;
	if (filter->bid && get_board_by_bid(filter->bid, &board)
			&& is_junk_board(&board)) {
		decrease = false;
	}

	record_t record, trash;
	post_index_board_open(filter->bid, RECORD_WRITE, &record);
	post_index_trash_open(filter->bid,
			bm_visible ? POST_INDEX_TRASH : POST_INDEX_JUNK, &trash);
	post_index_record_t pir;
	post_index_record_open(&pir);

	post_index_board_filter_t pibf = { .pir = &pir, .filter = filter };
	post_index_trash_insert_t piti = {
		.trash = &trash, .ename = currentuser.userid,
		.estamp = time(NULL), .junk = junk, .decrease = decrease,
	};

	record_lock_all(&trash, RECORD_WRLCK);
	int current = record_seek(&trash, 0, RECORD_CUR);

	record_lock_all(&record, RECORD_WRLCK);
	int deleted = record_delete(&record, ptr, offset,
			force ? post_index_board_filter : post_index_board_filter_unmarked,
			&pibf, post_index_trash_insert, &piti);
	record_lock_all(&record, RECORD_UNLCK);

	post_filter_t filter2 = { .fake_id_min = current + 1 };
	post_deletion_trigger(&trash, &filter2, &pir, true);

	record_lock_all(&trash, RECORD_UNLCK);
	return deleted;
}

typedef struct {
	post_index_board_t *buf;
	int count;
	int max;
} post_undeletion_callback_t;

static int post_undeletion_callback(void *ptr, void *args)
{
	const post_index_trash_t *pit = ptr;
	post_undeletion_callback_t *puc = args;
	if (puc->count >= puc->max)
		return -1;

	post_index_board_t *pib = puc->buf + puc->count;
	pib->id = pit->id;
	pib->reid_delta = pit->reid_delta;
	pib->tid_delta = pit->tid_delta;
	pib->uid = pit->uid;
	pib->flag = pit->flag;
	return ++puc->count;
}

int post_index_board_undelete(const post_filter_t *filter, void *ptr,
		int offset, bool bm_visible)
{
	if (!filter->bid)
		return 0;

	record_t record, trash;
	post_index_board_open(filter->bid, RECORD_WRITE, &record);
	post_index_trash_open(filter->bid,
			bm_visible ? POST_INDEX_TRASH : POST_INDEX_JUNK, &trash);
	post_index_record_t pir;
	post_index_record_open(&pir);

	post_index_board_filter_t pibf = { .pir = &pir, .filter = filter };

	record_lock_all(&trash, RECORD_WRLCK);
	int undeleted = post_deletion_trigger(&trash, filter, &pir, false);
	post_index_board_t *buf = malloc(undeleted * sizeof(post_index_board_t));
	post_undeletion_callback_t puc = {
		.buf = buf, .count = 0, .max = undeleted,
	};

	record_lock_all(&record, RECORD_WRLCK);
	record_delete(&trash, NULL, 0, post_index_board_filter, &pibf,
			post_undeletion_callback, &puc);
	record_merge(&record, buf, undeleted);
	record_lock_all(&record, RECORD_UNLCK);

	record_lock_all(&trash, RECORD_UNLCK);
	free(buf);
	return undeleted;
}

db_res_t *query_post_by_pid(const post_filter_t *filter, const char *fields)
{
	query_t *q = query_new(0);
	query_select(q, fields);
	query_from(q, post_table_name(filter));
	query_where(q, post_table_index(filter));
	query_append(q, "= %"DBIdPID, filter->min);

	db_res_t *res = query_exec(q);
	return res;
}

static char *replace_content_title(const char *content, size_t len,
		const char *title)
{
	const char *end = content + len;
	const char *l1_end = get_line_end(content, end);
	const char *l2_end = get_line_end(l1_end, end);

	// sizeof("标  题: ") in UTF-8 is 10
	const char *begin = l1_end + 10;
	int orig_title_len = l2_end - begin - 1; // exclude '\n'
	if (orig_title_len < 0)
		return NULL;

	int new_title_len = strlen(title);
	len += new_title_len - orig_title_len;
	char *s = malloc(len + 1);
	char *p = s;
	size_t l = begin - content;
	memcpy(p, content, l);
	p += l;
	memcpy(p, title, new_title_len);
	p += new_title_len;
	*p++ = '\n';
	l = end - l2_end;
	memcpy(p, l2_end, end - l2_end);
	s[len] = '\0';
	return s;
}

bool alter_title(post_index_record_t *pir, const post_info_t *pi)
{
	if (post_index_record_check(pir, pi->id, RECORD_WRITE) < 0)
		return false;

	post_index_t tmp;
	post_index_record_lock(pir, RECORD_WRLCK, pi->id);
	post_index_record_read(pir, pi->id, &tmp);
	strlcpy(tmp.utf8_title, pi->utf8_title, sizeof(tmp.utf8_title));
	post_index_record_update(pir, &tmp);
	post_index_record_lock(pir, RECORD_UNLCK, pi->id);

	char buf[4096];
	char *content = post_content_get(pi->id, buf, sizeof(buf));
	if (!content)
		return false;
	char *new_content =
		replace_content_title(content, strlen(content), pi->utf8_title);
	post_content_write(pi->id, new_content, strlen(new_content));
	free(new_content);
	if (content != buf)
		free(content);
	return true;
}

int get_post_mark_raw(post_id_t id, int flag)
{
	int mark = ' ';

	if (flag & POST_FLAG_DIGEST) {
		if (flag & POST_FLAG_MARKED)
			mark = 'b';
		else
			mark = 'g';
	} else if (flag & POST_FLAG_MARKED) {
		mark = 'm';
	}

	if (mark == ' ' && (flag & POST_FLAG_WATER))
		mark = 'w';

	if (brc_unread(id)) {
		if (mark == ' ')
			mark = DEFINE(DEF_NOT_N_MASK) ? '+' : 'N';
		else
			mark = toupper(mark);
	}

	return mark;
}

int get_post_mark(const post_info_t *p)
{
	return get_post_mark_raw(p->id, p->flag);
}
