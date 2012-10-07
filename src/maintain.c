#include "bbs.h"
#include "record.h"
#include "fbbs/helper.h"
#include "fbbs/mail.h"
#include "fbbs/session.h"
#include "fbbs/terminal.h"

#ifndef DLM
#undef  ALLOWGAME
#endif

char    cexplain[STRLEN];
char    lookgrp[30];
FILE   *cleanlog;

//保存用户近期信息
static int getuinfo(FILE *fn)
{
	int num;
	char buf[40];
	fprintf(fn, "\n\n他的代号     : %s\n", currentuser.userid);
	fprintf(fn, "他的昵称     : %s\n", currentuser.username);
	fprintf(fn, "电子邮件信箱 : %s\n", currentuser.email);
	fprintf(fn, "帐号建立日期 : %s\n", getdatestring(currentuser.firstlogin, DATE_ZH));
	fprintf(fn, "最近光临日期 : %s\n", getdatestring(currentuser.lastlogin, DATE_ZH));
	fprintf(fn, "最近光临机器 : %s\n", currentuser.lasthost);
	fprintf(fn, "上站次数     : %d 次\n", currentuser.numlogins);
	fprintf(fn, "文章数目     : %d\n", currentuser.numposts);
	fprintf(fn, "上站总时数   : %d 小时 %d 分钟\n", currentuser.stay / 3600,
			(currentuser.stay / 60) % 60);
	strcpy(buf, "ltmprbBOCAMURS#@XLEast0123456789");
	for (num = 0; num < 30; num++)
		if (!(currentuser.userlevel & (1 << num)))
			buf[num] = '-';
	buf[num] = '\0';
	fprintf(fn, "使用者权限   : %s\n\n", buf);
	return 0;
}

//	系统安全记录,自动发送到syssecurity版
//  mode == 0		syssecurity
//	mode == 1		boardsecurity
//  mode == 2		bmsecurity
//  mode == 3		usersecurity
void securityreport(char *str, int save, int mode)
{
	FILE*	se;
	char    fname[STRLEN];
	int     savemode;
	savemode = session.status;
	report(str, currentuser.userid);
	sprintf(fname, "tmp/security.%s.%05d", currentuser.userid, session.pid);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "系统安全记录\n[1m原因：%s[m\n", str);
		if (save){
			fprintf(se, "以下是个人资料:");
			getuinfo(se);
		}
		fclose(se);
		if (mode == 0){
			Postfile(fname, "syssecurity", str, 2);
		} else if (mode == 1){
			Postfile(fname, "boardsecurity", str, 2);
		} else if (mode == 2){
		    Postfile(fname, "bmsecurity", str, 2);
		} else if (mode == 3){
		    Postfile(fname, "usersecurity", str, 2);
		} else if (mode == 4){
		    Postfile(fname, "vote", str, 2);
		}
		unlink(fname);
		set_user_status(savemode);
	}
}

//	核对系统密码
int	check_systempasswd(void)
{
	FILE*	pass;
	char    passbuf[20], prepass[STRLEN];
	clear();
	if ((pass = fopen("etc/.syspasswd", "r")) != NULL) {
		fgets(prepass, STRLEN, pass);
		fclose(pass);
		prepass[strlen(prepass) - 1] = '\0';
		getdata(1, 0, "请输入系统密码: ", passbuf, 19, NOECHO, YEA);
		if (passbuf[0] == '\0' || passbuf[0] == '\n')
			return NA;
		if (!passwd_match(prepass, passbuf)) {
			move(2, 0);
			prints("错误的系统密码...");
			securityreport("系统密码输入错误...", 0, 0);
			pressanykey();
			return NA;
		}
	}
	return YEA;
}

//	自动发送到版面
//			title		标题
//			str			内容
//			uname		发送到的用户名,为null则不发送.
void autoreport(const char *board, const char *title, const char *str,
		const char *uname, int mode)
{
    report(title, currentuser.userid);

	if (uname)
		do_mail_file(uname, title, NULL, str, strlen(str), NULL);

	if (board)
		Poststring(str, board, title, mode);
}

// 清屏,并在第一行显示title
void	stand_title(char   *title)
{
	clear();
	standout();
	prints("%s",title);
	standend();
}

char    curruser[IDLEN + 2];
extern int delmsgs[];
extern int delcnt;

void
domailclean(fhdrp)
struct fileheader *fhdrp;
{
	static int newcnt, savecnt, deleted, idc;
	char    buf[STRLEN];
	if (fhdrp == NULL) {
		fprintf(cleanlog, "new = %d, saved = %d, deleted = %d\n",
			newcnt, savecnt, deleted);
		newcnt = savecnt = deleted = idc = 0;
		if (delcnt) {
			sprintf(buf, "mail/%c/%s/%s", toupper(curruser[0]), curruser, DOT_DIR);
			while (delcnt--)
				delete_record(buf, sizeof(struct fileheader), delmsgs[delcnt],NULL,NULL);
		}
		delcnt = 0;
		return;
	}
	idc++;
	if (!(fhdrp->accessed[0] & FILE_READ))
		newcnt++;
	else if (fhdrp->accessed[0] & FILE_MARKED)
		savecnt++;
	else {
		deleted++;
		sprintf(buf, "mail/%c/%s/%s", toupper(curruser[0]), curruser, fhdrp->filename);
		unlink(buf);
		delmsgs[delcnt++] = idc;
	}
}
