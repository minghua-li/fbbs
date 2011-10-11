// Lists all boards.

#include "libweb.h"
#include "fbbs/fileio.h"
#include "fbbs/web.h"

int bbsall_main(void)
{
	struct boardheader *x;
	int i;
	xml_header(NULL);
	printf("<bbsall>");
	print_session();
	for (i = 0; i < MAXBOARD; i++) {
		x = &(bcache[i]);
		if (x->filename[0] <= 0x20 || x->filename[0] > 'z')
			continue;
		if (!hasreadperm(&currentuser, x))
			continue;
		printf("<brd dir='%d' title='%s' cate='%6.6s' desc='%s' bm='%s' />",
				x->flag & BOARD_DIR_FLAG ? 1 : 0, x->filename, x->title + 1,
				get_board_desc(x), x->BM);
	}
	printf("</bbsall>");
	return 0;
}

// TODO: unify with telnet
static int filenum(char *board)
{
	char file[HOMELEN];
	struct stat st;
	sprintf(file, "boards/%s/.DIR", board);
	if (stat(file, &st) < 0)
		return 0;
	return st.st_size / sizeof(struct fileheader);
}

int bbsboa_main(void)
{
	int sector = (int)strtol(get_param("s"), NULL, 16);
	if (sector < 0 || sector >= SECNUM)
		return BBS_EINVAL;
	char *cgi;
	if (strtol(get_param("my_def_mode"), NULL, 10) != 0)
		cgi = "tdoc";
	else
		cgi = "doc";
	
	xml_header(NULL);
	printf("<bbsboa link='%s' ", cgi);

	struct boardheader *parent = NULL;
    int parent_bid = 0;
	const char *parent_name = get_param("board");
    if (*parent_name) {
        parent = getbcache(parent_name);
        parent_bid = getbnum(parent_name, &currentuser);
	} else {
		parent_bid = strtol(get_param("bid"), NULL, 10);
		parent = getbcache2(parent_bid);
	}
	if (parent == NULL || parent_bid <= 0 || !(parent->flag & BOARD_DIR_FLAG)) {
		parent = NULL;
		parent_bid = 0;
	}

	if (parent == NULL) {
		char path[HOMELEN];
		sprintf(path, "%s/info/egroup%d/icon.jpg", BBSHOME, sector);
		if (dashf(path))
			printf(" icon='%s'", path);
		printf(" title='%s'>", secname[sector][0]);
	} else {
		printf(" dir= '1' title='%s'>", get_board_desc(parent));
	}

	// TODO: Marquee BBSHOME/info/egroup(sector)/headline.txt
	struct boardheader *x;
	int i;
	for (i = 0; i < MAXBOARD; i++) {
		x = &(bcache[i]);
		if (x->filename[0] <= 0x20 || x->filename[0] > 'z')
			continue;
		if (!hasreadperm(&currentuser, x))
			continue;
		if (parent != NULL) {
			if (x->group != parent_bid) // directory listing
				continue;
		} else {  // section listing
			if (!strchr(seccode[sector], x->title[0]))
				continue;
		}
		printf("<brd dir='%d' title='%s' cate='%.6s' desc='%s' bm='%s' "
				"read='%d' count='%d' />",
				x->flag & BOARD_DIR_FLAG ? 1 : 0, x->filename,
				x->title + 1, get_board_desc(x), x->BM,
				brc_board_unread(currentuser.userid, x), filenum(x->filename));
	}
	print_session();
	printf("</bbsboa>");
	return 0;
}

