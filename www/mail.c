#include "libweb.h"
#include "mmap.h"
#include "record.h"
#include "fbbs/string.h"
#include "fbbs/mail.h"
#include "fbbs/web.h"

static bool _is_mail_read(const struct fileheader *fp)
{
	return (fp->accessed[0] & FILE_READ);
}

static int _get_mail_mark(const struct fileheader *fp)
{
	int mark = ' ';
	if (fp->accessed[0] & MAIL_REPLY)
		mark = 'r';
	if (fp->accessed[0] & FILE_MARKED) {
		if (mark == 'r')
			mark = 'b';
		else
			mark = 'm';
	}
	if (!_is_mail_read(fp)) {
		if (mark == ' ')
			mark = '+';
		else
			mark = toupper(mark);
	}
	return mark;
}

int bbsmail_main(web_ctx_t *ctx)
{
	if (!loginok)
		return BBS_ELGNREQ;

	int start = strtol(get_param(ctx->r, "start"), NULL, 10);
	int page = strtol(get_param(ctx->r, "page"), NULL, 10);
	if (page <= 0 || page > TLINES)
		page = TLINES;
	
	char buf[HOMELEN];
	setmdir(buf, currentuser.userid);
	mmap_t m = { .oflag = O_RDONLY };
	if (mmap_open(buf, &m) < 0)
		return BBS_ENOFILE;

	int total = m.size / sizeof(struct fileheader);
	if (start <= 0 || start > total - page + 1)
		start = total - page + 1;
	if (start < 1)
		start = 1;

	const struct fileheader *begin = m.ptr, *end = begin + total;
	const struct fileheader *fp = begin + start + page - 2;
	if (fp >= end)
		fp = end - 1;

	xml_header(NULL);
	printf("<bbsmail start='%d' total='%d' page='%d' dpage='%d'>",
			start, total, page, TLINES);
	print_session(ctx);

	for (int i = 0; i < page && fp >= begin; ++i, --fp) {
		printf("\n<mail r='%d' m='%c' from='%s' date='%s' name='%s'>",
				_is_mail_read(fp), _get_mail_mark(fp), fp->owner,
				getdatestring(getfiletime(fp), DATE_XML), fp->filename);
		xml_fputs2(fp->title, check_gbk(fp->title) - fp->title, stdout);
		printf("</mail>");
	}

	mmap_close(&m);
	printf("</bbsmail>");
	return 0;
}

int print_new_mail(void *buf, int count, void *args)
{
	struct fileheader *fp = buf;
	time_t limit = *(time_t *)args;
	time_t date = getfiletime(fp);
	if (date < limit)
		return QUIT;
	if (!(fp->accessed[0] & FILE_READ)) {
		printf("<mail from='%s' date='%s' name='%s' n='%d'>", fp->owner, 
				getdatestring(date, DATE_XML), fp->filename, count);
		xml_fputs2(fp->title, check_gbk(fp->title) - fp->title, stdout);
		printf("</mail>");
	}
	return 0;
}

int bbsnewmail_main(web_ctx_t *ctx)
{
	if (!loginok)
		return BBS_ELGNREQ;
	xml_header(NULL);
	printf("<bbsnewmail>");
	print_session(ctx);
	char file[HOMELEN];
	setmdir(file, currentuser.userid);
	time_t limit = time(NULL) - 24 * 60 * 60 * NEWMAIL_EXPIRE;
	apply_record(file, print_new_mail, sizeof(struct fileheader), &limit,
			false, true, true);
	printf("</bbsnewmail>");
	return 0;
}

int bbsmailcon_main(web_ctx_t *ctx)
{
	if (!loginok)
		return BBS_ELGNREQ;

	char file[40];
	strlcpy(file, get_param(ctx->r, "f"), sizeof(file));
	if (!valid_mailname(file))
		return BBS_EINVAL;

	char buf[HOMELEN];
	mmap_t m = { .oflag = O_RDWR };
	// deal with index
	setmdir(buf, currentuser.userid);
	if (mmap_open(buf, &m) < 0)
		return BBS_ENOFILE;

	int total = m.size / sizeof(struct fileheader);
	struct fileheader *fh = bbsmail_search(m.ptr, m.size, file);
	if (!fh) {
		mmap_close(&m);
		return BBS_ENOFILE;
	}

	bool newmail = false;
	if (!(fh->accessed[0] & FILE_READ)) {
		newmail = true;
		fh->accessed[0] |= FILE_READ;
	}

	xml_header(NULL);
	printf("<bbsmailcon total='%d'", total);
	struct fileheader *prev = fh - 1;
	if (prev >= (struct fileheader *)m.ptr)
		printf(" prev='%s'", prev->filename);
	struct fileheader *next = fh + 1;
	if (next < (struct fileheader *)m.ptr + m.size / sizeof(*next))
		printf(" next='%s'", next->filename);
	if (newmail)
		printf(" new='1'");
	printf("><t>");
	xml_fputs2(fh->title, check_gbk(fh->title) - fh->title, stdout);
	printf("</t>");

	mmap_close(&m);

	// show mail content.
	if (file[0] == 's') // shared mail
		strlcpy(buf, file, sizeof(buf));
	else
		setmfile(buf, currentuser.userid, file);

	m.oflag = O_RDONLY;
	if (mmap_open(buf, &m) < 0)
		return BBS_ENOFILE;
	printf("<mail f='%s' n='%s'>", file, get_param(ctx->r, "n"));
	xml_fputs((char *)m.ptr, stdout);
	fputs("</mail>\n", stdout);
	mmap_close(&m);

	print_session(ctx);
	printf("</bbsmailcon>");
	return 0;
}

int bbsdelmail_main(web_ctx_t *ctx)
{
	if (!loginok)
		return BBS_ELGNREQ;
	char file[40];
	strlcpy(file, get_param(ctx->r, "f"), sizeof(file));
	if (!valid_mailname(file))
		return BBS_EINVAL;
	char buf[HOMELEN];
	mmap_t m;
	setmdir(buf, currentuser.userid);
	m.oflag = O_RDWR;
	if (mmap_open(buf, &m) < 0)
		return BBS_EINTNL;
	struct fileheader *fh = bbsmail_search(m.ptr, m.size, file);
	if (fh == NULL) {
		mmap_close(&m);
		return BBS_ENOFILE;
	}
	struct fileheader *end = 
			(struct fileheader *)m.ptr + m.size / sizeof(*fh);
	if (fh < end) {
		memmove(fh, fh + 1, (end - fh) * sizeof(*fh));
		mmap_truncate(&m, m.size - sizeof(*fh));
	}
	mmap_close(&m);
	if (file[0] != 's') {// not shared mail
		setmfile(buf, currentuser.userid, file);
		unlink(buf);
	}
	xml_header("bbsdelmail");
	printf("<bbsdelmail></bbsdelmail>\n");
	return 0;
}

extern int web_quotation(const char *str, size_t size, const char *owner, bool ismail);

int bbspstmail_main(web_ctx_t *ctx)
{
	if (!loginok)
		return BBS_ELGNREQ;
	if (!HAS_PERM2(PERM_MAIL, &currentuser))
		return BBS_EACCES;
	// TODO: mail quota check, signature
	int num = 0;
	mmap_t m;
	m.oflag = O_RDONLY;
	char file[HOMELEN];
	const char *str = get_param(ctx->r, "n"); // 1-based
	const struct fileheader *fh = NULL;
	if (*str != '\0') {
		num = strtol(str, NULL, 10);
		if (num <= 0)
			return BBS_EINVAL;
		setmdir(file, currentuser.userid);
		if (mmap_open(file, &m) < 0)
			return BBS_EINTNL;
		int size = m.size / sizeof(*fh);
		if (num > size) {
			mmap_close(&m);
			return BBS_ENOFILE;
		}
		fh = (struct fileheader *)m.ptr + num - 1;
	}
	xml_header(NULL);
	printf("<bbspstmail ");

	printf(" ref='");
	const char *ref = get_referer();
	if (*ref == '\0')
		ref = "pstmail";
	xml_fputs(ref, stdout);

	printf("' recv='%s'>", fh == NULL ? get_param(ctx->r, "recv") : fh->owner);

	if (fh != NULL) {
		printf("<t>");
		xml_fputs(fh->title, stdout);
		printf("</t><m>");
		mmap_t m2;
		m2.oflag = O_RDONLY;
		setmfile(file, currentuser.userid, fh->filename);
		if (mmap_open(file, &m2) == 0) {
			web_quotation(m2.ptr, m2.size, fh->owner, true);
		}
		mmap_close(&m2);
		printf("</m>");
	}
	mmap_close(&m);
	print_session(ctx);
	printf("</bbspstmail>");
	return 0;
}

int bbssndmail_main(web_ctx_t *ctx)
{
	if (!loginok)
		return BBS_ELGNREQ;
	if (!HAS_PERM2(PERM_MAIL, &currentuser))
		return BBS_EACCES;
	if (parse_post_data(ctx->r) < 0)
		return BBS_EINVAL;

	const char *recv = get_param(ctx->r, "recv");
	if (*recv == '\0')
		return BBS_EINVAL;

	char title[STRLEN];
	strlcpy(title, get_param(ctx->r, "title"), sizeof(title));
	printable_filter(title);
	valid_title(title);
	if (*title == '\0')
		strlcpy(title, "û����", sizeof(title));

	const char *text = get_param(ctx->r, "text");
	int len = strlen(text);
	char header[320];
	snprintf(header, sizeof(header), "������: %s (%s)\n��  ��: %s\n����վ: "
			BBSNAME" (%s)\n��  Դ: %s\n\n", currentuser.userid,
			currentuser.username, title, getdatestring(time(NULL), DATE_ZH),
			mask_host(fromhost));
	// TODO: signature, error code
	if (do_mail_file(recv, title, header, text, len, NULL) < 0)
		return BBS_EINVAL;
	if (*get_param(ctx->r, "backup") != '\0') {
		char title2[STRLEN];
		snprintf(title2, sizeof(title2), "{%s} %s", recv, title);
		do_mail_file(currentuser.userid, title2, header, text, len, NULL);
	}
	const char *ref = get_param(ctx->r, "ref");
	http_header();
	refreshto(1, ref);
	printf("</head>\n<body>�����ɹ���1���Ӻ��Զ�ת��<a href='%s'>ԭҳ��</a>\n"
			"</body>\n</html>\n", ref);
	return 0;
}