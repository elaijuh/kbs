/*
 * Pirate Bulletin Board System Copyright (C) 1990, Edward Luke,
 * lush@Athena.EE.MsState.EDU Eagles Bulletin Board System Copyright (C)
 * 1992, Raymond Rocker, rocker@rock.b11.ingr.com Guy Vega,
 * gtvega@seabass.st.usm.edu Dominic Tynes, dbtynes@seabass.st.usm.edu
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 1, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */

#include "bbs.h"

char cexplain[STRLEN];
char *Ctime();
static int sysoppassed = 0;

/* modified by wwj, 2001/5/7, for new md5 passwd */
void igenpass(const char *passwd, const char *userid, unsigned char md5passwd[]);

int check_systempasswd()
{
    FILE *pass;
    char passbuf[40], prepass[STRLEN];

    if ((sysoppassed) && (time(NULL) - sysoppassed < 60 * 60))
        return true;
    clear();
    if ((pass = fopen("etc/systempassword", "rb")) != NULL) {
        fgets(prepass, STRLEN, pass);
        prepass[strlen(prepass) - 1] = '\0';
        if (!strcmp(prepass, "md5")) {
            fread(&prepass[16], 1, 16, pass);
        }
        fclose(pass);

        getdata(1, 0, "请输入系统密码: ", passbuf, 39, NOECHO, NULL, true);
        if (passbuf[0] == '\0' || passbuf[0] == '\n')
            return false;


        if (!strcmp(prepass, "md5")) {
            igenpass(passbuf, "[system]", (unsigned char *) prepass);
            passbuf[0] = (char) !memcmp(prepass, &prepass[16], 16);
        } else {
            passbuf[0] = (char) checkpasswd(prepass, passbuf);
        }
        if (!passbuf[0]) {
            move(2, 0);
            prints("系统密码输入错误...");
            securityreport("系统密码输入错误...", NULL, NULL);
            pressanykey();
            return false;
        }
    }
    sysoppassed = time(NULL);
    return true;
}

int setsystempasswd()
{
    FILE *pass;
    char passbuf[40], prepass[40];

    modify_user_mode(ADMIN);
    if (strcmp(currentuser->userid, "SYSOP"))
        return -1;
    if (!check_systempasswd())
        return -1;
    getdata(2, 0, "请输入新的系统密码: ", passbuf, 39, NOECHO, NULL, true);
    getdata(3, 0, "确认新的系统密码: ", prepass, 39, NOECHO, NULL, true);
    if (strcmp(passbuf, prepass))
        return -1;
    if ((pass = fopen("etc/systempassword", "w")) == NULL) {
        move(4, 0);
        prints("系统密码无法设定....");
        pressanykey();
        return -1;
    }
    fwrite("md5\n", 4, 1, pass);

    igenpass(passbuf, "[system]", (unsigned char *) prepass);
    fwrite(prepass, 16, 1, pass);

    fclose(pass);
    move(4, 0);
    prints("系统密码设定完成....");
    pressanykey();
    return 0;
}



void securityreport(char *str, struct userec *lookupuser, char fdata[7][STRLEN])
{                               /* Leeward: 1997.12.02 */
    FILE *se;
    char fname[STRLEN];
    int savemode;
    char *ptr;

    savemode = uinfo.mode;
    sprintf(fname, "tmp/security.%d", getpid());
    if ((se = fopen(fname, "w")) != NULL) {
        if (lookupuser) {
            if (strstr(str, "让") && strstr(str, "通过身份确认")) {
                fprintf(se, "系统安全记录系统\n[32m原因：%s[m\n", str);
                fprintf(se, "以下是通过者个人资料");
                /*
                 * getuinfo(se, lookupuser); 
                 */
                /*
                 * Haohmaru.99.4.15.把被注册的资料列得更详细,同时去掉注册者的资料 
                 */
                fprintf(se, "\n\n您的代号     : %s\n", fdata[1]);
                fprintf(se, "您的昵称     : %s\n", lookupuser->username);
                fprintf(se, "真实姓名     : %s\n", fdata[2]);
                fprintf(se, "电子邮件信箱 : %s\n", lookupuser->email);
                fprintf(se, "真实 E-mail  : %s$%s@%s\n", fdata[3], fdata[5], currentuser->userid);
                fprintf(se, "服务单位     : %s\n", fdata[3]);
                fprintf(se, "目前住址     : %s\n", fdata[4]);
                fprintf(se, "连络电话     : %s\n", fdata[5]);
                fprintf(se, "注册日期     : %s", ctime(&lookupuser->firstlogin));
                fprintf(se, "最近光临日期 : %s", ctime(&lookupuser->lastlogin));
                fprintf(se, "最近光临机器 : %s\n", lookupuser->lasthost);
                fprintf(se, "上站次数     : %d 次\n", lookupuser->numlogins);
                fprintf(se, "文章数目     : %d(Board)\n", lookupuser->numposts);
                fprintf(se, "生    日     : %s\n", fdata[6]);
                /*
                 * fprintf(se, "\n[33m以下是认证者个人资料[35m");
                 * getuinfo(se, currentuser);rem by Haohmaru.99.4.16 
                 */
                fclose(se);
                post_file(currentuser, "", fname, "Registry", str, 0, 2);
            } else if (strstr(str, "删除使用者：")) {
                fprintf(se, "系统安全记录系统\n[32m原因：%s[m\n", str);
                fprintf(se, "以下是被删者个人资料");
                getuinfo(se, lookupuser);
                fprintf(se, "\n以下是删除者个人资料");
                getuinfo(se, currentuser);
                fclose(se);
                post_file(currentuser, "", fname, "syssecurity", str, 0, 2);
            } else if ((ptr = strstr(str, "的权限XPERM")) != NULL) {
                int oldXPERM, newXPERM;
                int num;
                char XPERM[48];

                sscanf(ptr + strlen("的权限XPERM"), "%d %d", &oldXPERM, &newXPERM);
                *(ptr + strlen("的权限")) = 0;

                fprintf(se, "系统安全记录系统\n[32m原因：%s[m\n", str);

                strcpy(XPERM, XPERMSTR);
                for (num = 0; num < (int) strlen(XPERM); num++)
                    if (!(oldXPERM & (1 << num)))
                        XPERM[num] = ' ';
                XPERM[num] = '\0';
                fprintf(se, "以下是被改者原来的权限\n\033[1m\033[33m%s", XPERM);

                strcpy(XPERM, XPERMSTR);
                for (num = 0; num < (int) strlen(XPERM); num++)
                    if (!(newXPERM & (1 << num)))
                        XPERM[num] = ' ';
                XPERM[num] = '\0';
                fprintf(se, "\n%s\033[0m\n以上是被改者现在的权限\n", XPERM);

                fprintf(se, "\n"
                        "\033[1m\033[33mb\033[0m基本权力 \033[1m\033[33mT\033[0m进聊天室 \033[1m\033[33mC\033[0m呼叫聊天 \033[1m\033[33mP\033[0m发文章 \033[1m\033[33mR\033[0m资料正确 \033[1m\033[33mp\033[0m被禁发文 \033[1m\033[33m#\033[0m可隐身 \033[1m\033[33m@\033[0m可见隐身\n"
                        "\033[1m\033[33mX\033[0m长期帐号 \033[1m\033[33mW\033[0m编辑系统档案 \033[1m\033[33mB\033[0m版主 \033[1m\033[33mA\033[0m帐号管理 \033[1m\033[33m$\033[0m智囊团 \033[1m\033[33mV\033[0m投票管理 \033[1m\033[33mS\033[0m系统维护\n"
                        "\033[1m\033[33m!\033[0mRead/Post限制 \033[1m\033[33mD\033[0m精华区总管 \033[1m\033[33mE\033[0m讨论区总管 \033[1m\033[33mM\033[0m活动看版总管 \033[1m\033[33m1\033[0m不能ZAP \033[1m\033[33m2\033[0m聊天室OP\n"
                        "\033[1m\033[33m3\033[0m系统总管理员 \033[1m\033[33m4\033[0m荣誉帐号 \033[1m\033[33m5->9\033[0m 特殊权限5->9 \033[1m\033[33m0\033[0m看系统讨论版 \033[1m\033[33m%%\033[0m封禁Mail"
                        "\n");

                fprintf(se, "\n以下是被改者个人资料");
                getuinfo(se, lookupuser);
                fprintf(se, "\n以下是修改者个人资料");
                getuinfo(se, currentuser);
                fclose(se);
                post_file(currentuser, "", fname, "syssecurity", str, 0, 2);
            } else {            /* Modified for change id by Bigman 2001.5.25 */

                fprintf(se, "系统安全记录系统\0x1b[32m原因：%s\x1b[m\n", str);
                fprintf(se, "以下是个人资料");
                getuinfo(se, currentuser);
                fclose(se);
                post_file(currentuser, "", fname, "syssecurity", str, 0, 2);
            }
        } else {
            fprintf(se, "系统安全记录系统\n[32m原因：%s[m\n", str);
            fprintf(se, "以下是个人资料");
            getuinfo(se, currentuser);
            fclose(se);
            if (strstr(str, "设定使用者注册资料"))      /* Leeward 98.03.29 */
                post_file(currentuser, "", fname, "Registry", str, 0, 2);
            else
                post_file(currentuser, "", fname, "syssecurity", str, 0, 2);
        }
        unlink(fname);
        modify_user_mode(savemode);
    }
}

void stand_title(title)
char *title;
{
    clear();
    standout();
    prints(title);
    standend();
}

int m_info()
{
    struct userec uinfo;
    int id;
    struct userec *lookupuser;


    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {        /* Haohmaru.98.12.19 */
        return -1;
    }
    clear();
    stand_title("修改使用者代号");
    move(1, 0);
    usercomplete("请输入使用者代号: ", genbuf);
    if (*genbuf == '\0') {
        clear();
        return -1;
    }
    if (!(id = getuser(genbuf, &lookupuser))) {
        move(3, 0);
        prints(MSG_ERR_USERID);
        clrtoeol();
        pressreturn();
        clear();
        return -1;
    }
    uinfo = *lookupuser;

    move(1, 0);
    clrtobot();
    disply_userinfo(&uinfo, 1);
    uinfo_query(&uinfo, 1, id);
    return 0;
}

extern int cmpbnames();

int valid_brdname(brd)
char *brd;
{
    char ch;

    ch = *brd++;
    if (!isalnum(ch) && ch != '_')
        return 0;
    while ((ch = *brd++) != '\0') {
        if (!isalnum(ch) && ch != '_' && ch != '.')
            return 0;
    }
    return 1;
}

char *chgrp()
{
    int i, ch;
    char buf[STRLEN], ans[6];

    /*
     * static char    *explain[] = {
     * "本站系统",
     * "休闲娱乐",
     * "电脑技术",
     * "学术科学",
     * "体育健身",
     * "谈天说地",
     * "校园信息",
     * "艺术文化",
     * "人文社会",
     * "网络信息",
     * "清华大学",
     * "兄弟院校",
     * "其  他",
     * NULL
     * };
     * 
     * static char    *groups[] = {
     * "system.faq",
     * "rec.faq",
     * "comp.faq",
     * "sci.faq",
     * "sport.faq",
     * "talk.faq",
     * "campus.faq",
     * "literal.faq",
     * "soc.faq",
     * "network.faq",
     * "thu.faq",
     * "univ.faq",
     * "other.faq",
     * NULL
     * };
     */

    clear();
    move(2, 0);
    prints("选择精华区的目录\n");
    oflush();

    for (i = 0;; i++) {
        if (explain[i] == NULL || groups[i] == NULL)
            break;
        prints("[32m%2d[m. %-20s%-20s\n", i, explain[i], groups[i]);
    }
    sprintf(buf, "请输入你的选择(0~%d): ", i - 1);
    while (1) {
        getdata(i + 3, 0, buf, ans, 4, DOECHO, NULL, true);
        if (!isdigit(ans[0]))
            continue;
        ch = atoi(ans);
        if (ch < 0 || ch >= i || ans[0] == '\r' || ans[0] == '\0')
            continue;
        else
            break;
    }
    sprintf(cexplain, "%s", explain[ch]);

    return groups[ch];
}


int m_newbrd()
{
    struct boardheader newboard;
    char ans[5];
    char vbuf[100];
    char *group;


    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {
        return -1;
    }
    clear();
    memset(&newboard, 0, sizeof(newboard));
    prints("开启新讨论区:");
    while (1) {
        getdata(3, 0, "讨论区名称:   ", newboard.filename, 18, DOECHO, NULL, true);
        if (newboard.filename[0] == '\0')
            return -1;
        if (valid_brdname(newboard.filename))
            break;
        prints("不合法名称...");
    }
    getdata(4, 0, "讨论区说明:   ", newboard.title, 60, DOECHO, NULL, true);
    strcpy(vbuf, "vote/");
    strcat(vbuf, newboard.filename);
    setbpath(genbuf, newboard.filename);
    if (getbnum(newboard.filename) > 0 || mkdir(genbuf, 0755) == -1 || mkdir(vbuf, 0755) == -1) {
        prints("\n错误：错误的讨论区名称\n");
        pressreturn();
        clear();
        return -1;
    }
    newboard.flag = 0;
    getdata(5, 0, "讨论区管理员: ", newboard.BM, BM_LEN - 1, DOECHO, NULL, true);
    getdata(6, 0, "是否限制存取权力 (Y/N)? [N]: ", ans, 4, DOECHO, NULL, true);
    if (*ans == 'y' || *ans == 'Y') {
        getdata(6, 0, "限制 Read/Post? [R]: ", ans, 4, DOECHO, NULL, true);
        if (*ans == 'P' || *ans == 'p')
            newboard.level = PERM_POSTMASK;
        else
            newboard.level = 0;
        move(1, 0);
        clrtobot();
        move(2, 0);
        prints("设定 %s 权力. 讨论区: '%s'\n", (newboard.level & PERM_POSTMASK ? "POST" : "READ"), newboard.filename);
        newboard.level = setperms(newboard.level, 0, "权限", NUMPERMS, showperminfo, NULL);
        clear();
    } else
        newboard.level = 0;
    getdata(7, 0, "是否加入匿名版 (Y/N)? [N]: ", ans, 4, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {
        newboard.flag |= BOARD_ANNONY;
        addtofile("etc/anonymous", newboard.filename);
    }
    getdata(8, 0, "是否不记文章数(Y/N)? [N]: ", ans, 4, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y')
        newboard.flag |= BOARD_JUNK;
    getdata(9, 0, "是否可向外转信(Y/N)? [N]: ", ans, 4, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y')
        newboard.flag |= BOARD_OUTFLAG;
    if (add_board(&newboard) == -1) {
        move(t_lines - 1, 0);
        outs("加入讨论区失败!\n");
        pressreturn();
        clear();
        return -1;
    }
    group = chgrp();
    if (group != NULL) {
        if (newboard.BM[0] != '\0')
            sprintf(vbuf, "%-38.38s(BM: %s)", newboard.title + 13, newboard.BM);
        else
            sprintf(vbuf, "%-38.38s", newboard.title + 13);

        if (add_grp(group, newboard.filename, vbuf, cexplain) == -1)
            prints("\n成立精华区失败....\n");
        else
            prints("已经置入精华区...\n");
    }
    prints("\n新讨论区成立\n");
    sprintf(genbuf, "add brd %s", newboard.filename);
    report(genbuf);
    {
        char secu[STRLEN];

        sprintf(secu, "成立新版：%s", newboard.filename);
        securityreport(secu, NULL, NULL);
    }
    pressreturn();
    clear();
    return 0;
}

int m_editbrd()
{
    char bname[STRLEN], buf[STRLEN], oldtitle[STRLEN], vbuf[256], *group;
    char oldpath[STRLEN], newpath[STRLEN], tmp_grp[30];
    int pos, noidboard, a_mv;
    struct boardheader fh, newfh;

    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {
        return -1;
    }
    clear();
    stand_title("修改讨论区资讯");
    move(1, 0);
    make_blist();
    namecomplete("输入讨论区名称: ", bname);
    if (*bname == '\0') {
        move(2, 0);
        prints("错误的讨论区名称");
        pressreturn();
        clear();
        return -1;
    }
    pos = getboardnum(bname, &fh);
    if (!pos) {
        move(2, 0);
        prints("错误的讨论区名称");
        pressreturn();
        clear();
        return -1;
    }
    noidboard = anonymousboard(bname);
    move(2, 0);
    memcpy(&newfh, &fh, sizeof(newfh));
    prints("讨论区名称:   %s\n", fh.filename);
    prints("讨论区说明:   %s\n", fh.title);
    prints("讨论区管理员: %s\n", fh.BM);
    prints("匿名讨论区:   %s\n", (noidboard) ? "Yes" : "No");
    prints("不记文章数:   %s\n", (fh.flag & BOARD_JUNK) ? "Yes" : "No");
    prints("可向外转信:   %s\n", (fh.flag & BOARD_OUTFLAG) ? "Yes" : "No");
    strcpy(oldtitle, fh.title);
    prints("限制 %s 权力: %s", (fh.level & PERM_POSTMASK) ? "POST" : "READ", (fh.level & ~PERM_POSTMASK) == 0 ? "不设限" : "有设限");
    getdata(9, 0, "是否更改以上资讯? (Yes or No) [N]: ", genbuf, 4, DOECHO, NULL, true);
    if (*genbuf == 'y' || *genbuf == 'Y') {
        move(8, 0);
        prints("直接按 <Return> 不修改此栏资讯\n");
      enterbname:
        getdata(9, 0, "新讨论区名称: ", genbuf, 18, DOECHO, NULL, true);
        if (*genbuf != 0) {
            if (getboardnum(genbuf, NULL) > 0) {
                move(3, 0);
                prints("错误! 此讨论区已经存在\n");
                move(10, 0);
                clrtobot();
                goto enterbname;
            }
            strncpy(newfh.filename, genbuf, sizeof(newfh.filename));
            strcpy(bname, genbuf);
        }
        getdata(10, 0, "新讨论区说明: ", genbuf, 60, DOECHO, NULL, true);
        if (*genbuf != 0)
            strncpy(newfh.title, genbuf, sizeof(newfh.title));
        getdata(11, 0, "讨论区管理员: ", genbuf, 60, DOECHO, NULL, true);
        if (*genbuf != 0)
            strncpy(newfh.BM, genbuf, sizeof(newfh.BM));
        if (*genbuf == ' ')
            strncpy(newfh.BM, "\0", sizeof(newfh.BM));
        /*
         * newfh.BM[ BM_LEN - 1 ]=fh.BM[ BM_LEN - 1 ]; 
         */
        sprintf(buf, "匿名版 (Y/N)? [%c]: ", (noidboard) ? 'Y' : 'N');
        getdata(12, 0, buf, genbuf, 4, DOECHO, NULL, true);
        if (*genbuf == 'y' || *genbuf == 'Y' || *genbuf == 'N' || *genbuf == 'n') {
            if (*genbuf == 'y' || *genbuf == 'Y')
                noidboard = 1;
            else
                noidboard = 0;
        }
        sprintf(buf, "不记文章数 (Y/N)? [%c]: ", (newfh.flag & BOARD_JUNK) ? 'Y' : 'N');
        getdata(13, 0, buf, genbuf, 4, DOECHO, NULL, true);
        if (*genbuf == 'y' || *genbuf == 'Y' || *genbuf == 'N' || *genbuf == 'n') {
            if (*genbuf == 'y' || *genbuf == 'Y')
                newfh.flag |= BOARD_JUNK;
            else
                newfh.flag &= ~BOARD_JUNK;
        };
        sprintf(buf, "可向外转信 (Y/N)? [%c]: ", (newfh.flag & BOARD_OUTFLAG) ? 'Y' : 'N');
        getdata(14, 0, buf, genbuf, 4, DOECHO, NULL, true);
        if (*genbuf == 'y' || *genbuf == 'Y' || *genbuf == 'N' || *genbuf == 'n') {
            if (*genbuf == 'y' || *genbuf == 'Y')
                newfh.flag |= BOARD_OUTFLAG;
            else
                newfh.flag &= ~BOARD_OUTFLAG;
        };
        getdata(15, 0, "是否移动精华区的位置 (Y/N)? [N]: ", genbuf, 4, DOECHO, NULL, true);
        if (*genbuf == 'Y' || *genbuf == 'y')
            a_mv = 2;
        else
            a_mv = 0;
        getdata(16, 0, "是否更改存取权限 (Y/N)? [N]: ", genbuf, 4, DOECHO, NULL, true);
        if (*genbuf == 'Y' || *genbuf == 'y') {
            char ans[5];

            sprintf(genbuf, "限制 (R)阅读 或 (P)张贴 文章 [%c]: ", (newfh.level & PERM_POSTMASK ? 'P' : 'R'));
            getdata(17, 0, genbuf, ans, 4, DOECHO, NULL, true);
            if ((newfh.level & PERM_POSTMASK) && (*ans == 'R' || *ans == 'r'))
                newfh.level &= ~PERM_POSTMASK;
            else if (!(newfh.level & PERM_POSTMASK) && (*ans == 'P' || *ans == 'p'))
                newfh.level |= PERM_POSTMASK;
            move(1, 0);
            clrtobot();
            move(2, 0);
            prints("设定 %s '%s' 讨论区的权限\n", newfh.level & PERM_POSTMASK ? "张贴" : "阅读", newfh.filename);
            newfh.level = setperms(newfh.level, 0, "权限", NUMPERMS, showperminfo, NULL);
            clear();
            getdata(0, 0, "确定要更改吗? (Y/N) [N]: ", genbuf, 4, DOECHO, NULL, true);
        } else {
            getdata(17, 0, "确定要更改吗? (Y/N) [N]: ", genbuf, 4, DOECHO, NULL, true);
        }
        if (*genbuf == 'Y' || *genbuf == 'y') {
            char lookgrp[30];

            {
                char secu[STRLEN];

                sprintf(secu, "修改讨论区：%s(%s)", fh.filename, newfh.filename);
                securityreport(secu, NULL, NULL);
            }
            if (strcmp(fh.filename, newfh.filename)) {
                char old[256], tar[256];

                a_mv = 1;
                setbpath(old, fh.filename);
                setbpath(tar, newfh.filename);
                f_mv(old, tar);
                sprintf(old, "vote/%s", fh.filename);
                sprintf(tar, "vote/%s", newfh.filename);
                f_mv(old, tar);
            }
            if (newfh.BM[0] != '\0')
                sprintf(vbuf, "%-38.38s(BM: %s)", newfh.title + 13, newfh.BM);
            else
                sprintf(vbuf, "%-38.38s", newfh.title + 13);
            ann_get_board(lookgrp, fh.filename, 29);
            edit_grp(fh.filename, lookgrp, oldtitle + 13, vbuf);
            if (a_mv >= 1) {
                group = chgrp();
                ann_get_board(lookgrp, fh.filename, 29);
                strcpy(tmp_grp, lookgrp);
                if (strcmp(tmp_grp, group) || a_mv != 2) {
                    del_from_file("0Announce/.Search", fh.filename);
                    if (group != NULL) {
                        if (newfh.BM[0] != '\0')
                            sprintf(vbuf, "%-38.38s(BM: %s)", newfh.title + 13, newfh.BM);
                        else
                            sprintf(vbuf, "%-38.38s", newfh.title + 13);

                        if (add_grp(group, newfh.filename, vbuf, cexplain) == -1)
                            prints("\n成立精华区失败....\n");
                        else
                            prints("已经置入精华区...\n");
                        sprintf(newpath, "0Announce/groups/%s/%s", group, newfh.filename);
                        sprintf(oldpath, "0Announce/groups/%s/%s", tmp_grp, fh.filename);
                        if (dashd(oldpath)) {
                            /*
                             * sprintf(genbuf, "/bin/rm -fr %s", newpath);
                             */
                            f_rm(newpath);
                        }
                        f_mv(oldpath, newpath);
                        del_grp(tmp_grp, fh.filename, fh.title + 13);
                    }
                }
            }
            if (noidboard == 1 && !anonymousboard(newfh.filename)) {
                newfh.flag |= BOARD_ANNONY;
                addtofile("etc/anonymous", newfh.filename);
            } else if (noidboard == 0) {
                newfh.flag &= ~BOARD_ANNONY;
                del_from_file("etc/anonymous", newfh.filename);
            }
            set_board(pos, &newfh);
            sprintf(genbuf, "更改讨论区 %s 的资料 --> %s", fh.filename, newfh.filename);
            report(genbuf);
        }
    }
    clear();
    return 0;
}

int searchtrace()
{
    int id;
    char tmp_command[80], tmp_id[20];
    char buf[8192];

    if (check_systempasswd() == false)
        return -1;
    modify_user_mode(ADMIN);
    clear();
    stand_title("查询使用者发文记录");
    move(1, 0);
    usercomplete("请输入使用者帐号:", genbuf);
    strcpy(tmp_id, genbuf);
    if (tmp_id[0] == '\0') {
        clear();
        return -1;
    }

    if (!(id = getuser(genbuf, NULL))) {
        move(3, 0);
        prints("不正确的使用者代号\n");
        clrtoeol();
        pressreturn();
        clear();
        return -1;
    }

    sprintf(tmp_command, "grep -a -w %s user.log | grep posted > tmp/searchresult.%d", tmp_id, getpid());
    system(tmp_command);
    sprintf(tmp_command, "tmp/searchresult.%d", getpid());
    mail_file(currentuser->userid, tmp_command, currentuser->userid, "系统查询结果", 1);

    sprintf(buf, "查询用户 %s 的发文情况", tmp_id);
    securityreport(buf, NULL, NULL);    /*写入syssecurity版, stephen 2000.12.21 */
    sprintf(buf, "Search the posts by %s in the trace", tmp_id);
    report(buf);                /*写入trace, stephen 2000.12.21 */

    move(3, 0);
    prints("查询结果已经寄到您的信箱！ \n");
    pressreturn();
    clear();
    return 0;
}                               /* stephen 2000.12.15 let sysop search in trace */


/*
char curruser[IDLEN + 2];
extern int delmsgs[];
extern int delcnt;

void domailclean(struct fileheader *fhdrp, char *arg)
{
    static int newcnt, savecnt, deleted, idc;
    char buf[STRLEN];

    if (fhdrp == NULL) {
        bbslog("clean", "new = %d, saved = %d, deleted = %d", newcnt, savecnt, deleted);
        newcnt = savecnt = deleted = idc = 0;
        if (delcnt) {
            setmailfile(buf, curruser, DOT_DIR);
            while (delcnt--)
                delete_record(buf, sizeof(struct fileheader), delmsgs[delcnt], NULL, NULL);
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
        setmailfile(buf, curruser, fhdrp->filename);
        unlink(buf);
        delmsgs[delcnt++] = idc;
    }
}

int cleanmail(struct userec *urec, char *arg)
{
    struct stat statb;

    if (urec->userid[0] == '\0' || !strcmp(urec->userid, "new"))
        return 0;
    setmailfile(genbuf, urec->userid, DOT_DIR);
    if (stat(genbuf, &statb) == -1) {
        bbslog("clean", "%s no mail", urec->userid);
    } else {
        if (statb.st_size == 0) {
            bbslog("clean", "%s no mail", urec->userid);
        } else {
            strcpy(curruser, urec->userid);
            delcnt = 0;
            apply_record(genbuf, (RECORD_FUNC_ARG) domailclean, sizeof(struct fileheader), 0, 1);
            domailclean(NULL, 0);
        }
    }
    return 0;
}

int m_mclean()
{
    char ans[5];

    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {
        return -1;
    }
    clear();
    stand_title("清除私人信件");
    move(1, 0);
    prints("清除所有已读且未 mark 的信件\n");
    getdata(2, 0, "确定吗 (Y/N)? [N]: ", ans, 3, DOECHO, NULL, true);
    if (ans[0] != 'Y' && ans[0] != 'y') {
        clear();
        return 0;
    }
    {
        char secu[STRLEN];

        sprintf(secu, "清除所有使用者已读信件。");
        securityreport(secu, NULL, NULL);
    }

    move(3, 0);
    prints("请耐心等候.\n");
    refresh();
    apply_users(cleanmail, 0);
    move(4, 0);
    prints("清除完成! 请查看日志文件.\n");
    report("Mail Clean");
    pressreturn();
    clear();
    return 0;
}
*/

void trace_state(flag, name, size)
int flag, size;
char *name;
{
    char buf[STRLEN];

    if (flag != -1) {
        sprintf(buf, "ON (size = %d)", size);
    } else {
        strcpy(buf, "OFF");
    }
    prints("%s记录 %s\n", name, buf);
}

int touchfile(filename)
char *filename;
{
    int fd;

    if ((fd = open(filename, O_RDWR | O_CREAT, 0600)) > 0) {
        close(fd);
    }
    return fd;
}

int m_trace()
{
    struct stat ostatb, cstatb;
    int otflag, ctflag, done = 0;
    char ans[3];
    char *msg;

    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {
        return -1;
    }
    clear();
    stand_title("Set Trace Options");
    while (!done) {
        move(2, 0);
        otflag = stat("trace", &ostatb);
        ctflag = stat("trace.chatd", &cstatb);
        prints("目前设定:\n");
        trace_state(otflag, "一般", ostatb.st_size);
        trace_state(ctflag, "聊天", cstatb.st_size);
        move(9, 0);
        prints("<1> 切换一般记录\n");
        prints("<2> 切换聊天记录\n");
        getdata(12, 0, "请选择 (1/2/Exit) [E]: ", ans, 2, DOECHO, NULL, true);

        switch (ans[0]) {
        case '1':
            if (otflag) {
                touchfile("trace");
                msg = "一般记录 ON";
            } else {
                f_mv("trace", "trace.old");
                msg = "一般记录 OFF";
            }
            break;
        case '2':
            if (ctflag) {
                touchfile("trace.chatd");
                msg = "聊天记录 ON";
            } else {
                f_mv("trace.chatd", "trace.chatd.old");
                msg = "聊天记录 OFF";
            }
            break;
        default:
            msg = NULL;
            done = 1;
        }
        move(t_lines - 2, 0);
        if (msg) {
            prints("%s\n", msg);
            report(msg);
        }
    }
    clear();
    return 0;
}

int valid_userid(ident)         /* check the user has registed, added by dong, 1999.4.18 */
char *ident;
{
    if (strchr(ident, '@') && valid_ident(ident))
        return 1;
    return 0;
}

int check_proxy_IP(ip, buf)
                                /*
                                 * added for rejection of register from proxy,
                                 * Bigman, 2001.11.9 
                                 */
 /*
  * 与bbsd_single里面得local_check_ban_IP基本一样，可以考虑共用 
  */
char *ip;
char *buf;
{                               /* Leeward 98.07.31
                                 * RETURN:
                                 * - 1: No any banned IP is defined now
                                 * 0: The checked IP is not banned
                                 * other value over 0: The checked IP is banned, the reason is put in buf
                                 */
    FILE *Ban = fopen("etc/proxyIP", "r");
    char IPBan[64];
    int IPX = -1;
    char *ptr;

    if (!Ban)
        return IPX;
    else
        IPX++;

    while (fgets(IPBan, 64, Ban)) {
        if ((ptr = strchr(IPBan, '\n')) != NULL)
            *ptr = 0;
        if ((ptr = strchr(IPBan, ' ')) != NULL) {
            *ptr++ = 0;
            strcpy(buf, ptr);
        }
        IPX = strlen(ip);
        if (!strncmp(ip, IPBan, IPX))
            break;
        IPX = 0;
    }

    fclose(Ban);
    return IPX;
}

int apply_reg(regfile, fname, pid, num)
/* added by Bigman, 2002.5.31 */
/* 申请指定条数注册单 */
char *regfile, *fname;
long pid;
int num;
{
    FILE *in_fn, *out_fn, *tmp_fn;
    char fname1[STRLEN], fname2[STRLEN];
    int sum, fd;
    char *ptr;

    strcpy(fname1, "reg.ctrl");

    if ((in_fn = fopen(regfile, "r+")) == NULL) {
        move(2, 0);
        prints("系统错误, 无法读取注册资料档: %s\n", regfile);
        pressreturn();
        return -1;
    }

    fd = fileno(in_fn);
    flock(fd, LOCK_EX);

    if ((out_fn = fopen(fname, "w")) == NULL) {
        move(2, 0);
        flock(fd, LOCK_UN);
        fclose(in_fn);
        prints("系统错误, 无法写临时注册资料档: %s\n", fname);
        pressreturn();
        return -1;
    }
    sum = 0;

    while (fgets(genbuf, STRLEN, in_fn) != NULL) {
        if ((ptr = (char *) strstr(genbuf, "----")) != NULL)
            sum++;

        fputs(genbuf, out_fn);

        if (sum >= num)
            break;
    }
    fclose(out_fn);

    if (sum >= num) {
        sum = 0;

        sprintf(fname2, "tmp/reg.%ld", pid);

        if ((tmp_fn = fopen(fname2, "w")) == NULL) {
            prints("不能建立临时文件:%s\n", fname2);
            flock(fd, LOCK_UN);
            fclose(in_fn);
            pressreturn();
            return -1;
        }

        while (fgets(genbuf, STRLEN, in_fn) != NULL) {
            if ((ptr = (char *) strstr(genbuf, "userid")) != NULL)
                sum++;
            fputs(genbuf, tmp_fn);

        }

        flock(fd, LOCK_UN);

        fclose(in_fn);
        fclose(tmp_fn);

        if (sum > 0) {
            f_rm(regfile);
            f_mv(fname2, regfile);
        } else
            f_rm(regfile);

        f_rm(fname2);

    }

    else
        f_rm(regfile);

    if ((out_fn = fopen(fname1, "a")) == NULL) {
        move(2, 0);
        prints("系统错误, 无法更改注册控制文件: %s\n", fname1);
        pressreturn();
        return -1;
    }

    fd = fileno(out_fn);

    flock(fd, LOCK_UN);
    fprintf(out_fn, "%ld\n", pid);
    flock(fd, LOCK_UN);
    fclose(out_fn);

    return (0);
}

int check_reg(mod)
int mod;

/* added by Bigman, 2002.5.31 */
/* mod=0 检查reg_control文件 */
/* mod=1 正常退出删除该文件 */
{
    FILE *fn1, *fn2;
    char fname1[STRLEN];
    char fname2[STRLEN];
    long myid;
    int flag = 0, fd;

    strcpy(fname1, "reg.ctrl");

    if ((fn1 = fopen(fname1, "r")) != NULL) {

        fd = fileno(fn1);
        flock(fd, LOCK_EX);

        sprintf(fname2, "tmp/reg.c%ld", getpid());

        if ((fn2 = fopen(fname2, "w")) == NULL) {
            prints("不能建立临时文件:%s\n", fname2);
            flock(fd, LOCK_UN);
            fclose(fn1);
            pressreturn();
            return -1;
        } else {
            while (fgets(genbuf, STRLEN, fn1) != NULL) {

                myid = atol(genbuf);

                if (mod == 0) {
/*    					if (myid==getpid())
					{
					prints("你只能一个进程进行审批帐号");
					pressreturn();
					return -1;
					}
*/

                    if (kill(myid, 0) == -1) {  /*注册中间断线了，恢复 */
                        flag = 1;
                        restore_reg(myid);
                    } else {
                        fprintf(fn2, "%ld\n", myid);
                    }
                } else {
                    flag = 1;
                    if (myid != getpid())
                        fprintf(fn2, "%ld\n", myid);


                }

            }
            fclose(fn2);
        }
        flock(fd, LOCK_UN);
        fclose(fn1);

        if (flag == 1) {
            f_rm(fname1);
            f_mv(fname2, fname1);
        }
        f_rm(fname2);

    }

    return (0);
}

int restore_reg(pid)
long pid;

/* added by Bigman, 2002.5.31 */
/* 恢复断线的注册文件 */
{
    FILE *fn, *freg;
    char *regfile, buf[STRLEN];
    int fd1, fd2;

    regfile = "new_register";

    sprintf(buf, "register.%ld", pid);

    if ((fn = fopen(buf, "r")) != NULL) {
        fd1 = fileno(fn);
        flock(fd1, LOCK_EX);

        if ((freg = fopen(regfile, "a")) != NULL) {
            fd2 = fileno(freg);
            flock(fd2, LOCK_EX);
            while (fgets(genbuf, STRLEN, fn) != NULL)
                fputs(genbuf, freg);
            flock(fd2, LOCK_UN);
            fclose(freg);

        }
        flock(fd1, LOCK_UN);
        fclose(fn);

        f_rm(buf);
    }

    return (0);
}
static const char *field[] = { "usernum", "userid", "realname", "career",
    "addr", "phone", "birth", NULL
};
static const char *reason[] = {
    "请输入真实姓名(国外可用拼音).", "请详填学校科系或工作单位.",
    "请填写完整的住址资料.", "请详填连络电话(若无可用呼机或Email地址代替).",
    "请确实而详细的填写注册申请表.", "请用中文填写申请单.",
    "不允许从穿梭注册", "同一个用户注册了过多ID",
    NULL
};

int scan_register_form(logfile, regfile)
char *logfile, *regfile;
{
    static const char *finfo[] = { "帐号位置", "申请代号", "真实姓名", "服务单位",
        "目前住址", "连络电话", "生    日", NULL
    };
    struct userec uinfo;
    FILE *fn, *fout, *freg;
    char fdata[7][STRLEN];
    char fname[STRLEN], buf[STRLEN], buff;

    /*
     * ^^^^^ Added by Marco 
     */
    char ans[5], *ptr, *uid;
    int n, unum, fd;
    int count, sum, total_num;  /*Haohmaru.2000.3.9.计算还有多少单子没处理 */

    long pid;                   /* Added by Bigman: 2002.5.31 */

    uid = currentuser->userid;
    stand_title("依序设定所有新注册资料");
/*    sprintf(fname, "%s.tmp", regfile);*/

    pid = getpid();
    sprintf(fname, "register.%ld", pid);

    move(2, 0);
    if (dashf(fname)) {
/*        prints("[1m其他 SYSOP 正在使用 telnet 或 WWW 查看注册申请单，请检查使用者状态。\n\n");
        prints("[33m如果没有其他 SYSOP 正在查看注册申请单，则是由于断线造成的无法注册。\n");
        prints("请进 bbsroot 帐户运行一次以下命令：\n");
        prints("                                   [32mcat new_register.tmp >> new_register[33m\n");
        prints("确认上述命令运行成功后，再运行一次以下命令：\n");
        prints("                                            [32mrm new_register.tmp\n[0m");
        pressreturn();
        return -1;*/

        restore_reg(pid);       /* Bigman,2002.5.31:恢复该文件 */
    }
/*    f_mv(regfile, fname);*/
/*申请注册单 added by Bigman, 2002.5.31*/

/*统计总的注册单数 Bigman, 2002.6.2 */
    if ((fn = fopen(regfile, "r")) == NULL) {
        move(2, 0);
        prints("系统错误, 无法读取注册资料档: %s\n", fname);
        pressreturn();
        return -1;
    }

    fd = fileno(fn);
    flock(fd, LOCK_EX);

    total_num = 0;
    while (fgets(genbuf, STRLEN, fn) != NULL) {
        if ((ptr = (char *) strstr(genbuf, "userid")) != NULL)
            total_num++;
    }
    flock(fd, LOCK_UN);
    fclose(fn);

    apply_reg(regfile, fname, pid, 50);

    if ((fn = fopen(fname, "r")) == NULL) {
        move(2, 0);
        prints("系统错误, 无法读取注册资料档: %s\n", fname);
        pressreturn();
        return -1;
    }
    memset(fdata, 0, sizeof(fdata));
    /*
     * Haohmaru.2000.3.9.计算共有多少单子 
     */
    sum = 0;
    while (fgets(genbuf, STRLEN, fn) != NULL) {
        if ((ptr = (char *) strstr(genbuf, "userid")) != NULL)
            sum++;
    }
    fseek(fn, 0, SEEK_SET);
    count = 1;
    while (fgets(genbuf, STRLEN, fn) != NULL) {
        struct userec *lookupuser;

        if ((ptr = (char *) strstr(genbuf, ": ")) != NULL) {
            *ptr = '\0';
            for (n = 0; field[n] != NULL; n++) {
                if (strcmp(genbuf, field[n]) == 0) {
                    strcpy(fdata[n], ptr + 2);
                    if ((ptr = (char *) strchr(fdata[n], '\n')) != NULL)
                        *ptr = '\0';
                }
            }
        } else if ((unum = getuser(fdata[1], &lookupuser)) == 0) {
            move(2, 0);
            clrtobot();
            prints("系统错误, 查无此帐号.\n\n");
            for (n = 0; field[n] != NULL; n++)
                prints("%s     : %s\n", finfo[n], fdata[n]);
            pressreturn();
            memset(fdata, 0, sizeof(fdata));
        } else {
            uinfo = *lookupuser;
            move(1, 0);
            prints("帐号位置     : %d   共有 %d 张注册单，当前为第 %d 张，还剩 %d 张\n", unum, total_num, count++, sum - count + 1);    /*Haohmaru.2000.3.9.计算还有多少单子没处理 */
            disply_userinfo(&uinfo, 1);
            move(15, 0);
            printdash(NULL);
            for (n = 0; field[n] != NULL; n++)
                /*
                 * added for rejection of register from proxy 
                 */
                /*
                 * Bigman, 2001.11.9 
                 */
                if (n == 1) {
                    if (check_proxy_IP(uinfo.lasthost, buf) > 0)
                        prints("%s     : %s \033[33m%s\033[0m\n", finfo[n], fdata[n], buf);
                    else
                        prints("%s     : %s\n", finfo[n], fdata[n]);
                } else
                    prints("%s     : %s\n", finfo[n], fdata[n]);
            /*
             * if (uinfo.userlevel & PERM_LOGINOK) modified by dong, 1999.4.18 
             */
            if ((uinfo.userlevel & PERM_LOGINOK) || valid_userid(uinfo.realemail)) {
                move(t_lines - 1, 0);
                prints("此帐号不需再填写注册单.\n");
                pressanykey();
                ans[0] = 'D';
            } else {
                getdata(t_lines - 1, 0, "是否接受此资料 (Y/N/Q/Del/Skip)? [S]: ", ans, 3, DOECHO, NULL, true);
            }
            move(2, 0);
            clrtobot();
            switch (ans[0]) {
            case 'D':
            case 'd':
                break;
            case 'Y':
            case 'y':
                prints("以下使用者资料已经更新:\n");
                n = strlen(fdata[5]);
                if (n + strlen(fdata[3]) > 60) {
                    if (n > 40)
                        fdata[5][n = 40] = '\0';
                    fdata[3][60 - n] = '\0';
                }
                strncpy(uinfo.realname, fdata[2], NAMELEN);
                strncpy(uinfo.address, fdata[4], NAMELEN);
                sprintf(genbuf, "%s$%s@%s", fdata[3], fdata[5], uid);
                strncpy(uinfo.realemail, genbuf, STRLEN - 16);
                sprintf(buf, "tmp/email/%s", uinfo.userid);
                if ((fout = fopen(buf, "w")) != NULL) {
                    fprintf(fout, "%s\n", genbuf);
                    fclose(fout);
                }

                update_user(&uinfo, unum, 0);
                mail_file(currentuser->userid, "etc/s_fill", uinfo.userid, "恭禧你，你已经完成注册。", 0);
                sprintf(genbuf, "%s 让 %s 通过身份确认.", uid, uinfo.userid);
                securityreport(genbuf, lookupuser, fdata);
                if ((fout = fopen(logfile, "a")) != NULL) {
                    time_t now;

                    for (n = 0; field[n] != NULL; n++)
                        fprintf(fout, "%s: %s\n", field[n], fdata[n]);
                    now = time(NULL);
                    fprintf(fout, "Date: %s\n", Ctime(now));
                    fprintf(fout, "Approved: %s\n", uid);
                    fprintf(fout, "----\n");
                    fclose(fout);
                }
                /*
                 * user_display( &uinfo, 1 ); 
                 */
                /*
                 * pressreturn(); 
                 */

                /*
                 * 增加注册信息记录 2001.11.11 Bigman 
                 */
                sethomefile(buf, uinfo.userid, "/register");
                if ((fout = fopen(buf, "w")) != NULL) {
                    for (n = 0; field[n] != NULL; n++)
                        fprintf(fout, "%s     : %s\n", finfo[n], fdata[n]);
                    fprintf(fout, "您的昵称     : %s\n", uinfo.username);
                    fprintf(fout, "电子邮件信箱 : %s\n", uinfo.email);
                    fprintf(fout, "真实 E-mail  : %s\n", uinfo.realemail);
                    fprintf(fout, "Ident 资料   : %s\n", uinfo.ident);
                    fprintf(fout, "注册日期     : %s\n", ctime(&uinfo.firstlogin));
                    fprintf(fout, "注册时的机器 : %s\n", uinfo.lasthost);
                    fprintf(fout, "Approved: %s\n", uid);
                    fclose(fout);
                }

                break;
            case 'Q':
            case 'q':
                if ((freg = fopen(regfile, "a")) != NULL) {
                    fd = fileno(freg);
                    flock(fd, LOCK_EX);

                    for (n = 0; field[n] != NULL; n++)
                        fprintf(freg, "%s: %s\n", field[n], fdata[n]);
                    fprintf(freg, "----\n");
                    while (fgets(genbuf, STRLEN, fn) != NULL)
                        fputs(genbuf, freg);

                    flock(fd, LOCK_UN);
                    fclose(freg);
                }

                break;
            case 'N':
            case 'n':
                for (n = 0; field[n] != NULL; n++)
                    prints("%s: %s\n", finfo[n], fdata[n]);
                move(9, 0);
                prints("请选择/输入退回申请表原因, 按 <enter> 取消.\n");
                for (n = 0; reason[n] != NULL; n++)
                    prints("%d) %s\n", n, reason[n]);
                getdata(10 + n, 0, "退回原因: ", buf, STRLEN, DOECHO, NULL, true);
                buff = buf[0];  /* Added by Marco */
                if (buf[0] != '\0') {
                    if (buf[0] >= '0' && buf[0] < '0' + n) {
                        strcpy(buf, reason[buf[0] - '0']);
                    }
                    sprintf(genbuf, "<注册失败> - %s", buf);
                    strncpy(uinfo.address, genbuf, NAMELEN);
                    update_user(&uinfo, unum, 0);

                    /*
                     * ------------------- Added by Marco 
                     */
                    switch (buff) {
                    case '0':
                        mail_file(currentuser->userid, "etc/f_fill.realname", uinfo.userid, uinfo.address, 0);
                        break;
                    case '1':
                        mail_file(currentuser->userid, "etc/f_fill.unit", uinfo.userid, uinfo.address, 0);
                        break;
                    case '2':
                        mail_file(currentuser->userid, "etc/f_fill.address", uinfo.userid, uinfo.address, 0);
                        break;
                    case '3':
                        mail_file(currentuser->userid, "etc/f_fill.telephone", uinfo.userid, uinfo.address, 0);
                        break;
                    case '4':
                        mail_file(currentuser->userid, "etc/f_fill.real", uinfo.userid, uinfo.address, 0);
                        break;
                    case '5':
                        mail_file(currentuser->userid, "etc/f_fill.chinese", uinfo.userid, uinfo.address, 0);
                        break;
                    case '6':
                        mail_file(currentuser->userid, "etc/f_fill.proxy", uinfo.userid, uinfo.address, 0);
                        break;
                    case '7':
                        mail_file(currentuser->userid, "etc/f_fill.toomany", uinfo.userid, uinfo.address, 0);
                        break;
                    default:
                        mail_file(currentuser->userid, "etc/f_fill.real", uinfo.userid, uinfo.address, 0);
                        break;
                    }
                    /*
                     * -------------------------------------------------------
                     */
                    /*
                     * user_display( &uinfo, 1 ); 
                     */
                    /*
                     * pressreturn(); 
                     */
                    break;
                }
                move(10, 0);
                clrtobot();
                prints("取消退回此注册申请表.\n");
                /*
                 * run default -- put back to regfile 
                 */
            default:
                if ((freg = fopen(regfile, "a")) != NULL) {
                    fd = fileno(freg);
                    flock(fd, LOCK_EX);

                    for (n = 0; field[n] != NULL; n++)
                        fprintf(freg, "%s: %s\n", field[n], fdata[n]);
                    fprintf(freg, "----\n");

                    flock(fd, LOCK_UN);
                    fclose(freg);
                }
            }
            memset(fdata, 0, sizeof(fdata));
        }
    }

    check_reg(1);               /* Bigman:2002.5.31 */

    fclose(fn);
    unlink(fname);
    return (0);
}

int m_register()
{
    FILE *fn;
    char ans[3], *fname;
    int x, y, wid, len;

    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {
        return -1;
    }
    clear();

    if (check_reg(0) != 0)
        return (-1);            /* added by Bigman, 2002.5.31 */

    stand_title("设定使用者注册资料");
    move(2, 0);

    fname = "new_register";

    if ((fn = fopen(fname, "r")) == NULL) {
        prints("目前并无新注册资料.");
        pressreturn();
    } else {
        y = 2, x = wid = 0;
        while (fgets(genbuf, STRLEN, fn) != NULL && x < 65) {
            if (strncmp(genbuf, "userid: ", 8) == 0) {
                move(y++, x);
                prints(genbuf + 8);
                len = strlen(genbuf + 8);
                if (len > wid)
                    wid = len;
                if (y >= t_lines - 2) {
                    y = 2;
                    x += wid + 2;
                }
            }
        }
        fclose(fn);
        getdata(t_lines - 1, 0, "设定资料吗 (Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
        if (ans[0] == 'Y' || ans[0] == 'y') {
            {
                char secu[STRLEN];

                sprintf(secu, "设定使用者注册资料");
                securityreport(secu, NULL, NULL);
            }
            scan_register_form("register.list", fname);
        }
    }
    clear();
    return 0;
}

int m_stoplogin()
{
    char ans[4];

    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {
        return -1;
    }
    if (!HAS_PERM(currentuser, PERM_ADMIN))
        return -1;
    getdata(t_lines - 1, 0, "禁止登陆吗 (Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {
        if (vedit("NOLOGIN", false) == -1)
            unlink("NOLOGIN");
    }
    return 0;
}

/* czz added 2002.01.15 */
int inn_start()
{
    char ans[4], tmp_command[80];

    getdata(t_lines - 1, 0, "启动转信吗 (Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {
        sprintf(tmp_command, "~bbs/innd/innbbsd");
        system(tmp_command);
    }
    return 0;
}

int inn_reload()
{
    char ans[4], tmp_command[80];

    getdata(t_lines - 1, 0, "重读配置吗 (Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {
        sprintf(tmp_command, "~bbs/innd/ctlinnbbsd reload");
        system(tmp_command);
    }
    return 0;
}

int inn_stop()
{
    char ans[4], tmp_command[80];

    getdata(t_lines - 1, 0, "停止转信吗 (Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {
        sprintf(tmp_command, "~bbs/innd/ctlinnbbsd shutdown");
        system(tmp_command);
    }
    return 0;
}

/* added end */
/* 封禁权限管理*/
int x_deny()
{
    int sel;
    char userid[IDLEN + 1];
    struct userec *lookupuser;
    const int level[] = {
        PERM_BASIC,
        PERM_POST,
        PERM_DENYMAIL,
        PERM_CHAT,
        PERM_PAGE,
        -1
    };
    const int normal_level[] = {
        PERM_BASIC,
        PERM_POST,
        0,
        PERM_CHAT,
        PERM_PAGE,
        -1
    };

    const struct _select_item level_conf[] = {
        {3, 6, -1, SIT_SELECT, (void *) "1)登录权限"},
        {3, 7, -1, SIT_SELECT, (void *) "2)发表文章权限"},
        {3, 8, -1, SIT_SELECT, (void *) "3)发信权限"},
        {3, 9, -1, SIT_SELECT, (void *) "4)进入聊天室权限"},
        {3, 10, -1, SIT_SELECT, (void *) "5)呼叫聊天权限"},
        {3, 11, -1, SIT_SELECT, (void *) "6)换一个ID"},
        {3, 12, -1, SIT_SELECT, (void *) "7)退出"},
        {-1, -1, -1, 0, NULL}
    };

    modify_user_mode(ADMIN);
    if (!check_systempasswd()) {
        return -1;
    }
    move(0, 0);
    clear();

    while (1) {
        int i;
        int basicperm;
        int s[10][2];
        int lcount;

        move(1, 0);

        usercomplete("请输入使用者帐号:", genbuf);
        strncpy(userid, genbuf, IDLEN);
        if (userid[0] == '\0') {
            clear();
            return 0;
        }

        if (!(getuser(userid, &lookupuser))) {
            move(3, 0);
            prints("不正确的使用者代号\n");
            clrtoeol();
            pressreturn();
            clear();
            continue;
        }
        lcount = get_giveupinfo(lookupuser->userid, &basicperm, s);
        move(3, 0);
        clrtobot();

        for (i = 0; level[i] != -1; i++)
            if ((lookupuser->userlevel & level[i]) != normal_level[i]) {
                move(6 + i, 40);
                if (level[i] & basicperm)
                    prints("戒网中");
                else
                    prints("封禁中");
            }
        sel = simple_select_loop(level_conf, SIF_NUMBERKEY|SIF_SINGLE, 0, 6, NULL);
        if (sel == i + 2)
            break;
        if (sel > 0 && sel <= i) {
            char buf[40];
            char reportbuf[120];

            move(40, 0);
            if ((lookupuser->userlevel & level[sel-1]) == normal_level[sel-1]) {
                sprintf(buf, "真的要封禁%s的%s", lookupuser->userid, (char *) level_conf[sel-1].data+2);
                if (askyn(buf, 0) != 0) {
                	sprintf(reportbuf, "封禁%s的%s ", lookupuser->userid, (char *) level_conf[sel-1].data+2);
                    lookupuser->userlevel ^= level[sel-1];
                    securityreport(reportbuf,NULL,NULL);
                }
            } else {
            	if ((basicperm&level[sel-1]) == normal_level[sel-1])  {
                	sprintf(buf, "真的要解开%s的%s 封禁", lookupuser->userid, (char *) level_conf[sel-1].data+2);
                	sprintf(reportbuf, "解开%s的%s 封禁", lookupuser->userid, (char *) level_conf[sel-1].data+2);
            	}
                else {
                	sprintf(buf, "真的要解开%s的%s 戒网", lookupuser->userid, (char *) level_conf[sel-1].data+2);
                	sprintf(reportbuf, "解开%s的%s 戒网", lookupuser->userid, (char *) level_conf[sel-1].data+2);
                }
                if (askyn(buf, 0) != 0) {
                    lookupuser->userlevel ^= level[sel-1];
                    securityreport(reportbuf,NULL,NULL);
                }
            }
	    save_giveupinfo(lookupuser,lcount,s);
        }
    }
    return 0;
}
