/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "bbs.h"
#include "screen.h"             /* Leeward 98.06.05 */

time_t calltime = 0;
static int stuffmode = 0;

static int mem_show(char *ptr, int size, int row, int numlines, char *fn);

/*typedef (void  generate_attach_link_t)(char* buf,int buf_len,char* attachname,long attachpos,long attachsize);
*/
static generate_attach_link_t current_attach_link;
static void* current_attach_link_arg;
void register_attach_link(generate_attach_link_t fn,void* arg)
{
    current_attach_link=fn;
    current_attach_link_arg=arg;
}

/*extern int isblank(int c);*/


/*struct FILESHM
{
        char line[FILE_MAXLINE][FILE_BUFSIZE];
        int  fileline;
        int  max;
        time_t  update;
};
struct  FILESHM   *goodbyeshm;
struct  FILESHM   *issueshm;
*/
struct ACSHM {
    char line[ACBOARD_MAXLINE][ACBOARD_BUFSIZE];
    int movielines;
    time_t update;
};
struct ACSHM *movieshm;

int nnline = 0, xxxline = 0;
int more_size, more_num;
int displayflag = 0, shownflag = 1;

static int mem_more(char *ptr, int size, int quit, char *keystr, char *fn, char *title);

int NNread_init()
{
    FILE *fffd;
    char *ptr;
    char buf[ACBOARD_BUFSIZE];
    struct stat st;
    time_t ftime, now;
    int iscreate;

    now = time(0);
    if (stat("etc/movie", &st) < 0) {
        return 0;
    }
    ftime = st.st_mtime;
    if (movieshm == NULL) {
        movieshm = (void *) attach_shm("ACBOARD_SHMKEY", 4123, sizeof(*movieshm), &iscreate);
    }
    if (abs(now - movieshm->update) < 12 * 60 * 60 && ftime < movieshm->update) {
        return 1;
    }
    /*---	modified by period	2000-10-20	---*
            if ((fffd = fopen( "etc/movie" , "r" )) == NULL) {
                    return 0;
            }         
     ---*/
    nnline = 0;
    xxxline = 0;
    if (!DEFINE(currentuser, DEF_ACBOARD)) {
        nnline = 1;
        xxxline = 1;
        return 1;
    }
    /*---	原有程序顺序有误, !DEFINE --> return后没close	---*/
    if ((fffd = fopen("etc/movie", "r")) == NULL)
        return 0;
    /*---	---*/
    while ((xxxline < ACBOARD_MAXLINE) && (fgets(buf, ACBOARD_BUFSIZE, fffd) != NULL)) {
        ptr = movieshm->line[xxxline];
        memcpy(ptr, buf, sizeof(buf));
        xxxline++;
    }
    sprintf(buf, "%79.79s\n", " ");
    movieshm->movielines = xxxline;
    while (xxxline % MAXnettyLN != 0) {
        ptr = movieshm->line[xxxline];
        memcpy(ptr, buf, sizeof(buf));
        xxxline++;
    }
    movieshm->movielines = xxxline;
    movieshm->update = time(0);
    sprintf(buf, "%d 行 活动看板 更新", xxxline);
    bbslog("user", "%s", buf);
    fclose(fffd);
    return 1;
}

void check_calltime()
{
    int line;

    if (time(0) >= calltime && calltime != 0) {
        /*
         * if (uinfo.mode != MMENU)
         * {
         * bell();
         * move(0,0);
         * clrtoeol();
         * prints("请到主选单看备忘录......");
         * return;
         * }
         * set_alarm(0,0, NULL,NULL);
         * showusernote();
         * pressreturn();
         * R_monitor(NULL);
         */
        if (uinfo.mode == TALK)
            line = t_lines / 2 - 1;
        else
            line = 0;
        saveline(line, 0, NULL);        /* restore line */
        bell();
        bell();
        bell();
        good_move(line, 0);
        clrtoeol();
        prints("[44m[32mBBS 系统通告: [37m%-65s[m", "系统闹钟 铃～～～～～～");
        igetkey();
        good_move(line, 0);
        clrtoeol();
        saveline(line, 1, NULL);
        calltime = 0;
    }
}

void setcalltime()
{
    char ans[6];
    int ttt;

    good_move(1, 0);
    clrtoeol();
    getdata(1, 0, "几分钟后要系统提醒你: ", ans, 3, DOECHO, NULL, true);
    if (!isdigit(ans[0]))
        return;
    ttt = atoi(ans);
    if (ttt <= 0)
        return;
    calltime = time(0) + ttt * 60;

}

int readln(int fd, char *buf, char *more_buf)
{
    int len, bytes, in_esc, ch;

    len = bytes = in_esc = 0;
    while (1) {
        if (more_num >= more_size) {
            more_size = read(fd, more_buf, MORE_BUFSIZE);
            if (more_size == 0) {
                break;
            }
            more_num = 0;
        }
        ch = more_buf[more_num++];
        bytes++;
        if (ch == '\n' || bytes > 255) {
            break;
        } else if (ch == '\t') {
            do {
                len++, *buf++ = ' ';
            } while ((len % 8) != 0);
        } else if (ch == '') {
            if (showansi)
                *buf++ = ch;
            in_esc = 1;
        } else if (in_esc) {
            if (showansi)
                *buf++ = ch;
            if (strchr("[0123456789;,", ch) == NULL) {
                in_esc = 0;
            }
        } else if (isprint2(ch)) {
            /*
             * if(len>79)
             * break; 
             */
            len++, *buf++ = ch;
        }
    }
    *buf++ = ch;
    *buf = '\0';
    return bytes;
}

int morekey()
{
    while (1) {
        switch (igetkey()) {
        case Ctrl('Y'):
            return Ctrl('Y');
        case Ctrl('Z'):
            return 'M';         /* Leeward 98.07.30 support msgX */
        case '!':
            return '!';         /*Haohmaru 98.09.24 */
        case 'q':
        case KEY_LEFT:
        case EOF:
            return KEY_LEFT;
        case ' ':
        case KEY_RIGHT:
        case KEY_PGDN:
        case Ctrl('F'):
            return KEY_RIGHT;
        case KEY_PGUP:
        case Ctrl('B'):
            return KEY_PGUP;
        case '\r':
        case KEY_DOWN:
        case 'j':
            return KEY_DOWN;
        case 'k':
        case KEY_UP:
            return KEY_UP;
            /*************** 新增加阅读时的热键 Luzi 1997.11.1 ****************/
        case 'h':
        case '?':
            return 'H';
        case 'o':
        case 'O':
            return 'O';
        case 'l':
        case 'L':
            return 'L';
        case 'w':
        case 'W':
            return 'W';
        case 'H':
            return 'M';
        case 'X':              /* Leeward 98.06.05 */
            return 'X';
        case 'u':              /*Haohmaru 99.11.29 */
            return 'u';
        case KEY_REFRESH:
            return KEY_REFRESH;
        default:;
        }
    }
}

int seek_nth_line(int fd, int no, char *more_buf)
{
    int n_read, line_count, viewed;
    char *p, *end;

    lseek(fd, 0, SEEK_SET);
    line_count = viewed = 0;
    if (no > 0)
        while (1) {
            n_read = read(fd, more_buf, MORE_BUFSIZE);
            p = more_buf;
            end = p + n_read;
            for (; p < end && line_count < no; p++)
                if (*p == '\n')
                    line_count++;
            if (line_count >= no) {
                viewed += (p - more_buf);
                lseek(fd, viewed, SEEK_SET);
                break;
            } else
                viewed += n_read;
        }

    more_num = MORE_BUFSIZE + 1;        /* invalidate the readln()'s buffer */
    return viewed;
}

/*Add by SmallPig*/
int countln(fname)
char *fname;
{
    FILE *fp;
    char tmp[256];
    int count = 0;

    if ((fp = fopen(fname, "r")) == NULL)
        return 0;

    while (fgets(tmp, sizeof(tmp), fp) != NULL)
        count++;
    fclose(fp);
    return count;
}

                           /*
                            * below added by netty  
                                                       *//*
                                                       * Rewrite by SmallPig 
                                                       */
void netty_more()
{
    char buf[256];
    int ne_row = 1;
    int x, y;
    time_t thetime = time(0);

    if (!DEFINE(currentuser, DEF_ACBOARD)) {
        update_endline();
        return;
    }

    nnline = ((thetime / 10) % (movieshm->movielines / MAXnettyLN)) * MAXnettyLN;

    getyx(&y, &x);
    update_endline();
    good_move(3, 0);
    while ((nnline < movieshm->movielines) /*&&DEFINE(currentuser,DEF_ACBOARD) */ ) {
        good_move(2 + ne_row, 0);
        clrtoeol();

        strcpy(buf, movieshm->line[nnline]);
        showstuff(buf);
        nnline = nnline + 1;
        ne_row = ne_row + 1;
        if (nnline == movieshm->movielines) {
            nnline = 0;
            break;
        }
        if (ne_row > MAXnettyLN) {
            break;
        }
    }
    good_move(y, x);
}

void printacbar()
{
    int x, y;

    getyx(&y, &x);

    good_move(2, 0);
    if (DEFINE(currentuser, DEF_HIGHCOLOR))
        prints("[1;35m┌——————————————┤[37m活  动  看  版[35m├——————————————┐ [m\n");
    else
        prints("[35m┌——————————————┤[37m活  动  看  版[35m├——————————————┐ [m\n");
    good_move(3 + MAXnettyLN, 0);
    if (DEFINE(currentuser, DEF_HIGHCOLOR))
        prints("[1;35m└——————————————┤[36m" FOOTER_MOVIE "[35m├——————————————┘ [m\n");
    else
        prints("[35m└——————————————┤[36m" FOOTER_MOVIE "[35m├——————————————┘ [m\n");
    good_move(y, x);
}

extern int idle_count;
void R_monitor(void *data)
{
    if (!DEFINE(currentuser, DEF_ACBOARD))
        return;

    if (uinfo.mode != MMENU)
        return;
    netty_more();
    printacbar();
    idle_count++;
    set_alarm(10 * idle_count, 0, R_monitor, NULL);
    UNUSED_ARG(data);
}

struct MemMoreLines {
    char *ptr;
    int size;
    char *line[100];
    char ty[100];               /* 0: 普通, 有回车; 1: 普通, 无回车; 2: 引文, 有回车; 3: 引文, 无回车 */
    int len[100];
    int s[100];
    int start;                  /* this->line[start%100]是检索的行号最小的行，行号为 start */
    int num;                    /* 共检索了row到row+num-1这么多行 */
    int curr_line;              /* 当前游标位置 */
    char *curr;                 /* 当前游标的行 */
    char currty;
    int currlen;
    int total;
};

int measure_line(char *p0, int size, int *l, int *s, char oldty, char *ty)
{
    int i, w, in_esc = 0, db = 0, lastspace = 0, asciiart = 0;
    char *p = p0;

    if (size == 0)
        return -1;
    if (oldty!=100) {
    for (i = 0, w = 0; i < size; i++, p++) {
        if (*p == '\n') {
            *l = i;
            *s = i + 1;
            break;
        }
        if (*p == '\0') {
            *l = i;
            *s = i;
            break;
        }
        if (asciiart) {
            continue;
        } else if (*p == '\t') {
            db = 0;
            w = (w + 8) / 8 * 8;
            lastspace = i;
        } else if (*p == '\033') {
            db = 0;
            in_esc = 1;
            lastspace = i - 1;
        } else if (in_esc) {
            if (strchr("suHMfL@PABCDJK", *p) != NULL) {
                asciiart = 1;
                continue;
            }
            if (strchr("[0123456789;,", *p) == NULL)
                in_esc = 0;
        } else if (isprint2(*p)) {
            if (!db) {
                if ((unsigned char) *p >= 128)
                    db = 1;
                else if (isblank((int) *p))
                    lastspace = i;
            } else {
                db = 0;
                lastspace = i;
            }
            w++;
        }
    }
    if (i >= size) {
        *l = size;
        *s = size;
    }
    if (*s > 0 && ((p0[*s - 1] == '\n') || (p0[*s - 1] == '\0'))) {
        switch (oldty) {
        case 1:
            *ty = 0;
            break;
        case 3:
            *ty = 2;
            break;
        default:
            if (*l < 2 || strncmp(p0, ": ", 2))
                *ty = 0;
            else
                *ty = 2;
        }
    } else {
        switch (oldty) {
        case 1:
            *ty = 1;
            break;
        case 3:
            *ty = 3;
            break;
        default:
            if (*l < 2 || strncmp(p0, ": ", 2))
                *ty = 1;
            else
                *ty = 3;
        }
    }
    if (*s == size)
        return 0;
    }
    if ( oldty==100 || (size > ATTACHMENT_SIZE
        && !memcmp(p0, ATTACHMENT_PAD, ATTACHMENT_SIZE))) {
        long attach_len;

        *ty = 100;
        p = p0;
	if (oldty==100) p--;
        p += ATTACHMENT_SIZE;
        if ((p = (char *) memchr(p, '\0', size - (ATTACHMENT_SIZE))) == NULL) {
            return 0;
        }
        p++;
        *s = ntohl(*(unsigned long *) p) + p - p0 + sizeof(unsigned long);
	if (oldty==100) {
          *ty=101;
          *s--;
	}
	else {
          if (*s>size)
              *s=size;
	  *s=1;
	}
    } else {
        if (p0[*s-1]=='\0')
		*s++;
    }

    /*
    if ((oldty == 100) && (*ty != 100)) {
        *l = 0;
        *s = 0;
        *ty = 104;
    }*/
    return 0;
}

int effectiveline;              /*有效行数, 只计算前面的部分, 头部不含, 空行不含, 签名档不含, 引言不含 */
void init_MemMoreLines(struct MemMoreLines *l, char *ptr, int size)
{
    int i, s, u;
    char *p0, oldty = 0;

    l->ptr = ptr;
    l->size = size;
    l->start = 0;
    l->num = 0;
    l->total = 0;
    effectiveline = 0;
    for (i = 0, p0 = ptr, s = size; i < 50 && s > 0; i++) {
        u = (l->start + l->num) % 100;
        l->line[u] = p0;
        if (measure_line(p0, s, &l->len[u], &l->s[u], oldty, &l->ty[u])
            < 0) {
            break;
        }
        oldty = l->ty[u];
        s -= l->s[u];
        p0 = l->line[u] + l->s[u];
        l->num++;
        if (effectiveline >= 0) {
            if (l->len[u] >= 2 && strncmp(l->line[u], "--", 2) == 0)
                effectiveline = -effectiveline;
            else if (l->num > 3 && l->len[u] >= 2 && l->ty[u] < 2)
                effectiveline++;
        }
    }
    if (effectiveline < 0)
        effectiveline = -effectiveline;
    if (s == 0)
        l->total = l->num;
    l->curr_line = 0;
    l->curr = l->line[0];
    l->currlen = l->len[0];
    l->currty = l->ty[0];
}

int next_MemMoreLines(struct MemMoreLines *l)
{
    int n;
    char *p0;

    if (l->curr_line + 1 >= l->start + l->num) {
        char oldty;

        n = (l->start + l->num - 1) % 100;
        if (l->ptr + l->size == (l->line[n] + l->s[n])) {
            return -1;
        }
        if (l->num == 100) {
            l->start++;
            l->num--;
        }
        oldty = l->ty[n];
        p0 = l->line[n] + l->s[n];
        n = (l->start + l->num) % 100;
        l->line[n] = p0;
        measure_line(p0, l->size - (p0 - l->ptr), &l->len[n], &l->s[n], oldty, &l->ty[n]);
        l->num++;
        if (l->size - (p0 - l->ptr) == l->s[n]) {
            l->total = l->start + l->num;
        }
    }
    l->curr_line++;
    l->curr = l->line[l->curr_line % 100];
    l->currlen = l->len[l->curr_line % 100];
    l->currty = l->ty[l->curr_line % 100];
    return l->curr_line;
}

int seek_MemMoreLines(struct MemMoreLines *l, int n)
{
    int i;

    if (n < 0) {
        seek_MemMoreLines(l, 0);
        return -1;
    }
    if (n < l->start) {
        i = l->total;
        init_MemMoreLines(l, l->ptr, l->size);
        l->total = i;
    }
    if (n < l->start + l->num) {
        l->curr_line = n;
        l->curr = l->line[l->curr_line % 100];
        l->currlen = l->len[l->curr_line % 100];
        l->currty = l->ty[l->curr_line % 100];
        return l->curr_line;
    }
    while (l->curr_line != n)
        if (next_MemMoreLines(l) < 0)
            return -1;
    return 0;
}

#include <sys/mman.h>

int mmap_show(char *fn, int row, int numlines)
{
    char *ptr;
    int size, retv;

    BBS_TRY {
        if (safe_mmapfile(fn, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &ptr, &size, NULL) == 0)
            BBS_RETURN(0);
        retv = mem_show(ptr, size, row, numlines, fn);
    }
    BBS_CATCH {
    }
    BBS_END end_mmapfile((void *) ptr, size, -1);

    return retv;
}

int mmap_more(char *fn, int quit, char *keystr, char *title)
{
    char *ptr;
    int size, retv;

    BBS_TRY {
        if (safe_mmapfile(fn, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &ptr, &size, NULL) == 0)
            BBS_RETURN(-1);
        retv = mem_more(ptr, size, quit, keystr, fn, title);
    }
    BBS_CATCH {
    }
    BBS_END end_mmapfile((void *) ptr, size, -1);

    return retv;
}

void mem_printline(struct MemMoreLines *l, char *fn,char* begin)
{
    char* ptr=l->curr;
    int len=l->currlen;
    int ty=l->currty;
    if (stuffmode) {
        char buf[256];

        memcpy(buf, ptr, (len >= 256) ? 255 : len);
        buf[(len >= 256) ? 255 : len] = 0;
        showstuff(buf);
        prints("\n");
        return;
    }
    if (!strncmp(ptr, "□ 引用", 7) || !strncmp(ptr, "==>", 3)
        || !strncmp(ptr, "【 在", 5) || !strncmp(ptr, "※ 引述", 7)) {
        outns("\033[1;33m", 7);
        outns(ptr, len);
        outns("\033[m\n", 4);
        return;
    } else if (ty == 100) {
        char attachname[41], *p;
        strncpy(attachname, ptr + ATTACHMENT_SIZE, 40);
        p = strrchr(attachname, '.');
        if (p != NULL && (!strcasecmp(p, ".bmp") || !strcasecmp(p, ".jpg")
                          || !strcasecmp(p, ".gif") || !strcasecmp(p, ".jpeg")))
            prints("\033[m附图: %s 链接:\n", attachname,link);
        else
            prints("\033[m附件: %s 链接:\n", attachname,link);
	return;
    } else if (ty == 101) {
        char link[256];

        if (current_attach_link)
            (*current_attach_link)(link,255,ptr-begin+ATTACHMENT_SIZE-1,current_attach_link_arg);
        else
            strcpy(link,"(用www方式阅读本文可以下载此附件)");
	prints("\033[4m%s\033[m\n",link);
        return;
    } else if (ty >= 2) {
        outns("\033[36m", 5);
        outns(ptr, len);
        outns("\033[m\n", 4);
        return;
    }
    outns(ptr, len);
//    outns("\n", 1);
}

static int mem_show(char *ptr, int size, int row, int numlines, char *fn)
{
    extern int t_lines;
    struct MemMoreLines l;
    int i, curr_line;

    init_MemMoreLines(&l, ptr, size);
    good_move(row, 0);
    clrtobot();
    prints("\033[m");
    curr_line = l.curr_line;
    for (i = 0; i < t_lines - 1 - row && i < numlines; i++) {
        good_move(row+i, 0);
        mem_printline(&l, fn, ptr);
        if (next_MemMoreLines(&l) < 0)
            break;
    }
    return 0;
}

void mem_printbotline(int l1, int l2, int total, int read, int size)
{
    extern int t_lines;

/*	static int n = 0;
	char *(s[4]) = {
		"结束 ← q | ↑ ↓ PgUp PgDn 移动",
		"s 开头 | e 末尾 | b f 向前后翻页",
		"g 跳到指定行 | ? / 向上下搜索字符串",
		"n下一篇 | l上一篇 | R 回文 | E 评价"
	};
	n++;
	if (uinfo.mode == READING)
		n %= 4;
	else
		n %= 3;*/
    good_move(t_lines - 1, 0);
/*	prints
	    ("\033[1;44;32m%s (%d%%) 第(%d-%d)行 \033[33m| %s | h 辅助说明\033[m",
	     (read >= size) ? "看到末尾啦" : "下面还有喔",
	     total ? (100 * l2 / total) : (100 * read / size), l1, l2, s[n]);*/
    if (currentuser != NULL && DEFINE(currentuser, DEF_HIGHCOLOR))
        prints("[1;44m[32m下面还有喔 (%d%%)[33m   │ 结束 ← <q> │ ↑/↓/PgUp/PgDn 移动 │ ? 辅助说明 │     [m", total ? (100 * l2 / total) : (100 * read / size));
    else
        prints("[44m[32m下面还有喔 (%d%%)[33m   │ 结束 ← <q> │ ↑/↓/PgUp/PgDn 移动 │ ? 辅助说明 │     [m", total ? (100 * l2 / total) : (100 * read / size));
}

int mem_more(char *ptr, int size, int quit, char *keystr, char *fn, char *title)
{
    extern int t_lines;
    struct MemMoreLines l;
    int i, ch = 0, curr_line, last_line, change;

    displayflag = 0;
    shownflag = 1;
    init_MemMoreLines(&l, ptr, size);

    prints("\033[m");
    while (1) {
        good_move(0, 0);
        clear();
        curr_line = l.curr_line;
        for (i = 0;;) {
            if (shownflag) {
                displayflag = 0;
            }
            move(i, 0);
            mem_printline(&l, fn, ptr);
            i++;
            if (i >= t_lines - 1)
                break;
            if (next_MemMoreLines(&l) < 0)
                break;
        }
        move(i, 0);
        last_line = l.curr_line;
        if (l.total && l.total <= t_lines - 1)
            return 0;
        if (l.line[last_line % 100] - ptr + l.s[last_line % 100] == size && (ch == KEY_RIGHT || ch == KEY_PGDN || ch == ' ' || ch == Ctrl('f'))) {
            good_move(t_lines - 1, 0);
            clrtobot();
            return 0;
        }
        change = 0;
        while (change == 0) {
            mem_printbotline(curr_line + 1, last_line + 1, l.total, l.line[last_line % 100] - ptr + l.s[last_line % 100], size);
            ch = morekey();
            good_move(t_lines - 1, 0);
            clrtoeol();
            switch (ch) {
            case KEY_UP:
                change = -1;
                break;
            case KEY_DOWN:
            case 'd':
            case 'j':
            case '\n':
                change = 1;
                break;
            case 'b':
            case Ctrl('b'):
            case KEY_PGUP:
                change = -t_lines + 2;
                break;
            case ' ':
            case 'f':
            case Ctrl('f'):
            case KEY_PGDN:
            case KEY_RIGHT:
                if (!l.total)
                    seek_MemMoreLines(&l, last_line + t_lines);
                change = t_lines - 2;
                if (l.total && last_line < l.total && curr_line + change + t_lines - 1 > l.total)
                    change = l.total - curr_line - t_lines + 1;
                break;
            case 's':
                change = -curr_line;
                break;
            case 'e':
                if (!l.total) {
                    while (next_MemMoreLines(&l) >= 0);
                    curr_line = l.curr_line;
                } else
                    curr_line = l.total - 1;
                change = -t_lines + 2;
                break;
            case 'g':
            case KEY_LEFT:
            case 'q':
                return 0;
            case '!':
                Goodbye();
                curr_line += t_lines - 1;
                change = 1 - t_lines;
                break;
            case 'n':
                return KEY_DOWN;
            case 'l':
                return KEY_UP;
            case KEY_REFRESH:
                curr_line += t_lines - 1;
                change = 1 - t_lines;
                break;
            case 'L':
                if (uinfo.mode != LOOKMSGS) {
                    show_allmsgs();
                    curr_line += t_lines - 1;
                    change = 1 - t_lines;
                }
                break;
            case 'M':
                r_lastmsg();
                clear();
                curr_line += t_lines - 1;
                change = 1 - t_lines;
                break;
            case 'W':
                if (HAS_PERM(currentuser, PERM_PAGE)) {
                    s_msg();
                    curr_line += t_lines - 1;
                    change = 1 - t_lines;
                }
                break;
            case 'u':{
                    int oldmode = uinfo.mode;

                    clear();
                    modify_user_mode(QUERY);
                    t_query(NULL);
                    clear();
                    modify_user_mode(oldmode);
                    curr_line += t_lines - 1;
                    change = 1 - t_lines;
                }
                break;
            case 'H':
                show_help("help/morehelp");
                curr_line += t_lines - 1;
                change = 1 - t_lines;
                break;
            case Ctrl('Y'):
                if (title) {
                    zsend_file(fn, title);
                    curr_line += t_lines - 1;
                    change = 1 - t_lines;
                }
                break;
            default:
                if (keystr != NULL && strchr(keystr, ch) != NULL)
                    return ch;
            }
            if (change < 0 && curr_line == 0) {
                if (quit)
                    return KEY_UP;
                change = 0;
            }
            if (change == 1) {
                if (seek_MemMoreLines(&l, curr_line + t_lines - 1) >= 0) {
                    curr_line++;
                    last_line++;
                    scroll();
                    good_move(t_lines - 2, 0);
                    mem_printline(&l, fn, ptr);
                    if ((ch == KEY_PGDN || ch == ' ' || ch == Ctrl('f')
                         || ch == KEY_RIGHT || ch == KEY_DOWN || ch == 'j' || ch == '\n')
                        && l.line[last_line % 100] - ptr + l.s[last_line % 100] == size) {
                        good_move(t_lines - 1, 0);
                        clrtoeol();
                        return 0;
                    }
                } else
                    return 0;
                change = 0;
            }
            if (change == -1) {
                if (seek_MemMoreLines(&l, curr_line - 1) >= 0) {
                    curr_line--;
                    last_line--;
                    rscroll();
                    good_move(0, 0);
                    mem_printline(&l, fn, ptr);
                }
                change = 0;
            }
            if (!change)
                mem_printbotline(curr_line + 1, last_line + 1, l.total, l.line[last_line % 100] - ptr + l.s[last_line % 100], size);
        }

        seek_MemMoreLines(&l, curr_line + change);
    }
}

int ansimore(char *filename, int promptend)
{
    int ch;

    clear();
    ch = mmap_more(filename, 1, "RrEexp", NULL);
    if (promptend)
        pressanykey();
    if (scrint)
    good_move(t_lines - 1, 0);
    prints("[m[m");
    return ch;
}

int ansimore2(filename, promptend, row, numlines)
char *filename;
int promptend;
int row;
int numlines;
{
    int ch;

    if (numlines)
        ch = mmap_show(filename, row, numlines);
    else
        ch = mmap_more(filename, 1, NULL, NULL);
    if (promptend)
        pressanykey();
    return ch;
}

int ansimore_withzmodem(char *filename, int promptend, char *title)
{
    int ch;

    clear();
    ch = mmap_more(filename, 1, "RrEexp", title);
    if (promptend)
        pressanykey();
    if (scrint)
    good_move(t_lines - 1, 0);
    prints("\x1b[m\x1b[m");
    return ch;
}

extern int offsetln;

int draw_content_more(char *ptr, int size, char *fn, struct fileheader *fh)
{
    extern int t_lines;
    struct MemMoreLines l;
    int i, j, ch = 0, curr_line, last_line, change;
    bool header = true;
    char buf[256];

    displayflag = 0;
    shownflag = 1;
    init_MemMoreLines(&l, ptr, size);

    good_move(t_lines / 2, 0);
/*    prints("\033[34m——————————————————预览窗口—————————————————");*/
/*    move(t_lines/2+1, 0);*/
    sprintf(buf, "\033[1;32m\x1b[44m发信人: \033[1;33m%-13.13s\033[1;32m标  题: \033[1;33m%-50.50s\033[1;32m %4.4s\033[m", fh->owner, fh->title, fh->innflag[1] == 'S' ? "[转]" : "");
    outs(buf);
    prints("\n\033[m");
    for(i=t_lines/2+1;i<=t_lines-1;i++) {
        move(i,0);
        clrtoeol();
    }
    curr_line = l.curr_line;
    for (i = 0, j = 0;;) {
        if (shownflag) {
            displayflag = 0;
        }
        if (!header || (!((i == 0) && ((!strncmp(l.curr, "发信人: ", 8) || (!strncmp(l.curr, "寄信人: ", 8))))) &&
                        !((i == 1) && !strncmp(l.curr, "标  题: ", 8)) && !((i == 2) && !strncmp(l.curr, "发信站: ", 8)) && !((i == 3) && (l.currlen == 0)))) {
            move(t_lines/2+1+j, 0);
            offsetln = t_lines/2+1;
            mem_printline(&l, fn, ptr);
            offsetln = 0;
            j++;
            header = false;
        }
        i++;
        if (j >= t_lines - t_lines / 2 - 2)
            break;
        if (next_MemMoreLines(&l) < 0)
            break;
    }
    last_line = l.curr_line;
    if (l.total && l.total <= t_lines - t_lines / 2 - 2)
        return 0;
}

int draw_content(char *fn, struct fileheader *fh)
{
    char *ptr;
    int size, retv;

    BBS_TRY {
        if (safe_mmapfile(fn, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &ptr, &size, NULL) == 0)
            BBS_RETURN(-1);
        retv = draw_content_more(ptr, size, fn, fh);
    }
    BBS_CATCH {
    }
    BBS_END end_mmapfile((void *) ptr, size, -1);

    return retv;
}
