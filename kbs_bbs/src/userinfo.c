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

extern time_t   login_start_time;
char *genpasswd() ;
char    *sysconf_str();

void
disply_userinfo( u, real )
struct userec *u ;
int     real;
{
    struct stat st;
    int         num, diff;
    int         exp;

    move(real==1?2:3,0);
    clrtobot();
    prints("您的代号     : %s\n", u->userid);
    prints("您的昵称     : %s\n", u->username);
    prints("真实姓名     : %s\n", u->realname);
    prints("居住住址     : %s\n", u->address);
    prints("电子邮件信箱 : %s\n", u->email);
    if( real ) {
        prints("真实 E-mail  : %s\n", u->termtype + 16 );
        prints("Ident 资料   : %s\n", u->ident );
    }
    prints("终端机形态   : %s\n", u->termtype );
    prints("注册日期     : %s", ctime( &u->firstlogin));
    prints("最近光临日期 : %s", ctime( &u->lastlogin));
    if( real ) {
        prints("最近光临机器 : %s\n", u->lasthost );
    }
/*---	added by period		hide posts/logins	2000-11-02	---*/
/*    if(HAS_PERM(PERM_ADMINMENU)) {*/ /* removed to let user can see his own data */
        prints("上站次数     : %d 次\n", u->numlogins);
	if(real) prints("文章数目     : %d 篇\n", u->numposts);
 /*       if( real ) {   
            prints("文章数目     : %d / %d (Board/1Discuss)\n",
               u->numposts, post_in_tin( u->userid ));
        }  removed by stephen 2000-11-02*/
/*    }*/
 /* move these things, alex , 97.6 
    exp=countexp(u);
    prints("经验值       : %d(%s)\n",exp,cexp(exp));
    exp=countperf(u);
    prints("表现值       : %d(%s)\n",exp,cperf(exp));  */
    prints("上站总时数   : %d 小时 %d 分钟\n",u->stay/3600,(u->stay/60)%60);
        setmailfile(genbuf, u->userid, DOT_DIR);
    if( stat( genbuf, &st ) >= 0 )
        num = st.st_size / (sizeof( struct fileheader ));
    else
        num = 0;
    prints("私人信箱     : %d 封\n", num );

    if( real ) {
        strcpy( genbuf, "bTCPRp#@XWBA$VS!DEM1234567890%" );
        for( num = 0; num < strlen(genbuf); num++ )
            if( !(u->userlevel & (1 << num)) )
                genbuf[num] = '-';
        genbuf[num] = '\0';
        prints("使用者权限   : %s\n", genbuf );
    } else {
        diff = (time(0) - login_start_time) / 60;
        prints("停留期间     : %d 小时 %02d 分\n", diff / 60, diff % 60 );
        prints("屏幕大小     : %dx%d\n", t_lines, t_columns );
    }
    prints("\n");
    if( u->userlevel & PERM_LOGINOK ) {
        prints("  您的注册程序已经完成, 欢迎加入本站.\n");
    } else if( u->lastlogin - u->firstlogin < 3 * 86400 ) {
        prints("  新手上路, 请阅读 Announce 讨论区.\n" );
    } else {
        prints("  注册尚未成功, 请参考本站进站画面说明.\n");
    }
}


int
uinfo_query( u, real, unum )
struct userec *u ;
int     real, unum;
{
    struct userec       newinfo;
    char        ans[3], buf[ STRLEN ],*emailfile,genbuf[STRLEN];
    int         i, fail = 0 ,netty_check  = 0;
    FILE        *fin, *fout,*dp;
    time_t      code;

    memcpy( &newinfo, u, sizeof(currentuser));
    getdata( t_lines-1, 0, real ?
        "请选择 (0)结束 (1)修改资料 (2)设定密码 (3) 改 ID ==> [0]" :
        "请选择 (0)结束 (1)修改资料 (2)设定密码 ==> [0]",
        ans, 2, DOECHO, NULL,YEA);
    clear();
    refresh();

    i = 3;
    move( i++, 0 );
    if(ans[0]!='3'||real)
        prints("使用者代号: %s\n", u->userid );

    switch( ans[0] ) {
        case '1':
            move( 1, 0 );
            prints("请逐项修改,直接按 <ENTER> 代表使用 [] 内的资料。\n");

            sprintf( genbuf, "昵称 [%s]: ", u->username );
            getdata( i++, 0, genbuf, buf, NAMELEN, DOECHO, NULL ,YEA);
            if( buf[0] ) strncpy( newinfo.username, buf, NAMELEN );
            if(!real && buf[0]) strncpy(uinfo.username,buf,40);            

            sprintf( genbuf, "真实姓名 [%s]: ", u->realname );
            getdata( i++, 0, genbuf, buf, NAMELEN, DOECHO, NULL,YEA);
            if( buf[0] ) strncpy( newinfo.realname, buf, NAMELEN );

            sprintf( genbuf, "居住地址 [%s]: ", u->address );
            getdata( i++, 0, genbuf, buf, STRLEN, DOECHO, NULL,YEA);
            if( buf[0] ) strncpy( newinfo.address, buf, NAMELEN );

            sprintf( genbuf, "电子信箱 [%s]: ", u->email );
            getdata( i++, 0, genbuf, buf, STRLEN, DOECHO, NULL,YEA);
            if ( buf[0] )
            { 
                 /*netty_check = 1;*/
                 /* 取消email 认证, alex , 97.7 */
                 strncpy( newinfo.email, buf, STRLEN );
                 
            }
            sprintf( genbuf, "终端机形态 [%s]: ", u->termtype );
            getdata( i++, 0, genbuf, buf, 16, DOECHO, NULL ,YEA);
            if( buf[0] ) strncpy( newinfo.termtype, buf, 16 );

        if( real ) {
            sprintf( genbuf, "真实Email[%s]: ", u->termtype+16 );
            getdata( i++, 0, genbuf, buf, STRLEN, DOECHO, NULL ,YEA);
            if( buf[0] ) strncpy( newinfo.termtype+16, buf, STRLEN-16 );

            sprintf( genbuf, "上线次数 [%d]: ", u->numlogins );
            getdata( i++, 0, genbuf, buf, 16, DOECHO, NULL ,YEA);
            if( atoi( buf ) > 0 ) newinfo.numlogins = atoi( buf );

            sprintf( genbuf, "文章数目 [%d]: ", u->numposts );
            getdata( i++, 0, genbuf, buf, 16, DOECHO, NULL ,YEA);
            {
                int lres;
                lres = atoi(buf);
                if(lres > 0 || ('\0' == buf[1] && '0' == *buf))
                    newinfo.numposts = lres;
            }
/*            if( atoi( buf ) > 0 ) newinfo.numposts = atoi( buf );*/
         
            sprintf( genbuf, "将注册日期提前三天 [Y/N]");
            getdata( i++, 0, genbuf, buf, 16, DOECHO, NULL, YEA);
            if(buf[0]=='y'||buf[0]=='Y') newinfo.firstlogin-=3*86400;

            sprintf( genbuf, "将最近光临日期设置为今天吗？[Y/N]");
            getdata( i++, 0, genbuf, buf, 16, DOECHO, NULL, YEA);
            if(buf[0]=='y'||buf[0]=='Y') newinfo.lastlogin=time(0);

        }

            break;
        case '2':
            if( ! real ) {
                getdata(i++,0,"请输入原密码: ",buf,PASSLEN,NOECHO,NULL,YEA);
                if( *buf == '\0' || !checkpasswd( u->passwd, buf )) {
                    prints("\n\n很抱歉, 您输入的密码不正确。\n");
                    fail++;
                    break;
                }
            }
            getdata(i++,0,"请设定新密码: ",buf,PASSLEN,NOECHO,NULL,YEA);
            if( buf[0] == '\0' ) {
                prints("\n\n密码设定取消, 继续使用旧密码\n");
                fail++;
                break;
            }
            strncpy(genbuf,buf,PASSLEN) ;

            getdata(i++,0,"请重新输入新密码: ",buf,PASSLEN,NOECHO,NULL,YEA);
            if(strncmp(buf,genbuf,PASSLEN)) {
                prints("\n\n新密码确认失败, 无法设定新密码。\n");
                fail++;
                break;
            }
            buf[8] = '\0';
/*	Added by cityhunter to deny others to modify SYSOP's passwd */
	    if( real && (strcmp(u->userid,"SYSOP") ==0) )
            {
		prints("\n\n错误!系统禁止修改SYSOP的密码,警察正在来的路上 :)");
		pressreturn();
        	clear();
        	return 0;
	    }
/* end of this addin */
            strncpy( newinfo.passwd, genpasswd( buf ), PASSLEN );
            break;
        case '3':
            if( ! real ) {
                    clear();
                        return 0;
            }
/* Bigman 2000.10.2 修改使用者ID位数不够 */
            getdata(i++,0,"新的使用者代号: ",genbuf,IDLEN+1,DOECHO,NULL,YEA);
            if(*genbuf != '\0') {
                if(getuser(genbuf)) {
                    prints("\n错误! 已经有同样 ID 的使用者\n") ;
                    fail++;
                } else {
                    strncpy(newinfo.userid, genbuf,IDLEN+2) ;
                }
            }
            break;
        default:
            clear();
            return 0;
    }
    if( fail != 0 ) {
        pressreturn();
        clear();
        return 0;
    }
    for(;;)
     {
    getdata(t_lines-1,0,"确定要改变吗?  (Yes or No): ",ans,2,DOECHO,NULL,YEA);
    if (*ans=='n'||*ans=='N') break;
    if( *ans == 'y' || *ans == 'Y' ) {
       if(real)
       {
        char        secu[STRLEN];
        sprintf(secu,"修改 %s 的基本资料或密码。",u->userid);
	if(strcmp(u->userid, newinfo.userid ))
		sprintf(secu,"%s 的 ID 被 %s 改为 %s",u->userid,currentuser.userid,newinfo.userid);/*Haohmaru.99.5.6*/
        securityreport(secu);
       }
        if( strcmp( u->userid, newinfo.userid ) ) {
            char src[ STRLEN ], dst[ STRLEN ];

                        setmailpath( src, u->userid );
            setmailpath( dst, newinfo.userid );
	    sprintf(genbuf,"mv %s %s",src, dst);
	    system(genbuf);/*
            rename( src, dst );*/
            sethomepath( src, u->userid );
            sethomepath( dst, newinfo.userid );
	    sprintf(genbuf,"mv %s %s",src ,dst);
	    system(genbuf);/*
            rename( src, dst );*/
            sprintf(src,"tmp/email_%s",u->userid);
            unlink(src);
            setuserid( unum, newinfo.userid );
        }
/* added by netty to automatically send a mail to new user. */

if ((netty_check == 1))
{
 if((strchr( newinfo.email, '@' ) != NULL ) &&
    (!strstr( newinfo.email, "@firebird.cs") ) &&
    (!strstr( newinfo.email, "@bbs.") ) &&
        (!invalidaddr(newinfo.email) ) &&
    (!strstr( newinfo.email, ".bbs@") )) {
if( (emailfile = sysconf_str( "EMAILFILE" )) != NULL )
{
           code=(time(0)/2)+(rand()/10);
                   sethomefile(genbuf, u->userid, "mailcheck"); 
           if((dp=fopen(genbuf,"w"))==NULL)
           {
                fclose(dp);
                return;
           }
           fprintf(dp,"%9.9d\n",code);
           fclose(dp);
sprintf( genbuf, "/usr/lib/sendmail -f SYSOP.bbs@%s %s ", 
email_domain(), newinfo.email );
fout = popen( genbuf, "w" );
fin  = fopen( emailfile, "r" );
if (fin == NULL || fout == NULL) return -1;
fprintf( fout, "Reply-To: SYSOP.bbs@%s\n", email_domain());
fprintf( fout, "From: SYSOP.bbs@%s\n",  email_domain() ); 
fprintf( fout, "To: %s\n", newinfo.email);
fprintf( fout, "Subject: @%s@[-%9.9d-]firebird mail check.\n", u->userid,code );
fprintf( fout, "X-Forwarded-By: SYSOP \n" );
fprintf( fout, "X-Disclaimer: None\n");
fprintf( fout, "\n");
fprintf(fout,"您的基本资料如下：\n",u->userid);
fprintf(fout,"使用者代号：%s (%s)\n",u->userid,u->username);
fprintf(fout,"姓      名：%s\n",u->realname);
fprintf(fout,"上站位置  ：%s\n",u->lasthost);
fprintf(fout,"电子邮件  ：%s\n\n",u->email);
fprintf(fout,"亲爱的 %s(%s):\n",u->userid,u->username);

while (fgets( genbuf, 255, fin ) != NULL ) {
        if (genbuf[0] == '.' && genbuf[ 1 ] == '\n')
                fputs( ". \n", fout );
        else fputs( genbuf, fout );
}
fprintf(fout, ".\n");                                    
fclose( fin );
pclose( fout );                                     
}
}else
{
   if(sysconf_str( "EMAILFILE" )!=NULL)
   {
        move(t_lines-5,0);
        prints("\n你的电子邮件地址 【[33m%s[m】\n",newinfo.email);
        prints("并非 Unix 帐号，系统不会投递身份确认信，请到[32m工具箱[m中修改..\n");
        pressanykey();
   }
}
}
        memcpy( u, &newinfo, sizeof(newinfo) );
        set_safe_record();
        substitute_record( PASSFILE, &newinfo, sizeof(newinfo), unum );
	break;/*Haohmaru.98.01.10.faint...Luzi加个for循环也不break!*/
   }
 }clear();
    return 0;
}

void
x_info()
{
    modify_user_mode( GMENU ); 
    disply_userinfo( &currentuser, 1 );
    if (!strcmp("guest", currentuser.userid)) {
        pressreturn();
        return;
    }
    uinfo_query( &currentuser, 0, usernum );
}

void
getfield( line, info, desc, buf, len )
int     line, len;
char    *info, *desc, *buf;
{
    char        prompt[ STRLEN ];

/*    sprintf( genbuf, "  原先设定: %-46.46s (%s)", buf, info ); */
    sprintf( genbuf, "  原先设定: %-20.20s (%s)", buf, info ); 
    move( line, 0 );
    prints( genbuf );
    sprintf( prompt, "  %s: ", desc );
    getdata( line+1, 0, prompt, genbuf, len, DOECHO, NULL ,YEA);
    if( genbuf[0] != '\0' ) {
        strncpy( buf, genbuf, len );
    }
    move( line, 0 );
    clrtoeol();
    prints( "  %s: %s\n", desc, buf );
    clrtoeol();
}

void
x_fillform()
{
    char        rname[ NAMELEN ], addr[ STRLEN ];
    char        phone[ STRLEN ], career[ STRLEN ],birth[ STRLEN];
    char        ans[5], *mesg, *ptr;
    FILE        *fn;
    time_t      now;

    modify_user_mode(NEW);
    move( 3, 0 );
    clrtobot();

    if (!strcmp("guest", currentuser.userid)) {
        prints( "抱歉, 请用 new 申请一个新帐号後再填申请表." );
        pressreturn();
        return;
    }
    if( currentuser.userlevel & PERM_LOGINOK ) {
        prints( "您的身份确认已经成功, 欢迎加入本站的行列." );
        pressreturn();
        return;
    }
    if ((time(0)-currentuser.firstlogin) < 3*86400) 
    {
        prints( "您首次登入本站未满3天(72个小时)..." );
        prints( "请先四处熟悉一下，在满3天以后再填写注册单。");
        pressreturn();
        return;
    }

    /*prints("因转站不接收新注册用户填写注册单; 8 号恢复本服务。请等候。");
    pressreturn();
    return;*/


    if( (fn = fopen( "new_register", "r" )) != NULL ) {
        while( fgets( genbuf, STRLEN, fn ) != NULL ) {
            if( (ptr = strchr( genbuf, '\n' )) != NULL )
                *ptr = '\0';
            if( strncmp( genbuf, "userid: ", 8 ) == 0 &&
                strcmp( genbuf + 8, currentuser.userid ) == 0 ) {
                fclose( fn );
                prints( "站长尚未处理您的注册申请单, 请耐心等候." );
                pressreturn();
                return;
            }
        }
        fclose( fn );
    }
/* added by KCN 1999.10.25 */
    ansimore("etc/register.note",NA);
    getdata(t_lines-1,8,"您确定要填写注册单吗 (Y/N)? [N]: ",ans,3,DOECHO,NULL,YEA);
    if( ans[0] != 'Y' && ans[0] != 'y' )
        return;
    strncpy( rname, currentuser.realname, NAMELEN );
    strncpy( addr,  currentuser.address,  STRLEN  );
    career[0] = phone[0] = birth[0]='\0';
    clear();
    while( 1 ) {
        move( 3, 0 );
        clrtoeol();
        prints( "%s 您好, 请据实填写以下的资料(请使用中文):\n", currentuser.userid ); 
	genbuf[0] = '\0';/*Haohmaru.99.09.17.以下内容不得过短*/
	while ( strlen( genbuf ) < 3 ) {
        getfield(  6, "请用中文,不能输入的汉字请用拼音", "真实姓名", rname, NAMELEN ); }
	genbuf[0] = '\0';
	while ( strlen( genbuf ) < 2 ) {
        getfield(  8, "学校系级或单位全称", "服务单位", career,STRLEN ); }
	genbuf[0] = '\0';
	while ( strlen( genbuf ) < 6 ) {
        getfield( 10, "请具体到寝室或门牌号码", "目前住址", addr,  STRLEN ); }
	genbuf[0] = '\0';
	while ( strlen( genbuf ) < 2 ) {
        getfield( 12, "包括可连络时间,若无可用呼机或Email地址代替",     "连络电话", phone, STRLEN ); }
        getfield( 14, "年.月.日(公元)(选择填写)",     "出生年月", birth, STRLEN );
        mesg = "以上资料是否正确, 按 Q 放弃注册 (Y/N/Quit)? [N]: ";
        getdata(t_lines-1,0,mesg,ans,3,DOECHO,NULL,YEA);
        if( ans[0] == 'Q' || ans[0] == 'q' )
            return;
        if( ans[0] == 'Y' || ans[0] == 'y' )
            break;
    }
    strncpy( currentuser.realname, rname,  NAMELEN );
    strncpy( currentuser.address,  addr,   STRLEN  );
    if( (fn = fopen( "new_register", "a" )) != NULL ) {
        now = time( NULL );
        fprintf( fn, "usernum: %d, %s", usernum, ctime( &now ) );
        fprintf( fn, "userid: %s\n",    currentuser.userid );
        fprintf( fn, "realname: %s\n",  rname );
        fprintf( fn, "career: %s\n",    career );
        fprintf( fn, "addr: %s\n",      addr );
        fprintf( fn, "phone: %s\n",     phone );
        fprintf( fn, "birth: %s\n",     birth);
        fprintf( fn, "----\n" );
        fclose( fn );
    }
}

