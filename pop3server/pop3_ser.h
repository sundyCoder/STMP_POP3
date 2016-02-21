/*
 * pop3_ser.h
 *
 *  Created on: 2012-8-12
 *      Author: root
 */

#ifndef POP3_SER_H_
#define POP3_SER_H_
#include <stdio.h>
#include "tcp.h"

#define SERVER_READY "+OK welcome to JChen's pop3 server!\r\n"
#define RESP_SUC     "+OK JChen\r\n"
#define RESP_ERR     "-ERR not valid\r\n"
#define RESP_QUIT    "+OK byebye\r\n"
#define RESP_TIMEOUT "-ERR,time out!\r\n"
#define RESP_BUSY    "-ERR,server is busy now!\r\n"
#define DB_BUSY      "-ERR,database is busy now!\r\n"

#define CMD_USER "USER"  //username
#define CMD_PASS "PASS"  //password
#define CMD_STAT "STAT"  //获取邮件的总数和总大小
#define CMD_LIST "LIST"  //列出邮件的列表和大小
#define CMD_RETR "RETR"  //查阅邮件
#define CMD_DELE "DELE"  //删除邮件
#define CMD_RSET "RSET"  //清楚收件人的数据，包括缓存数据
#define CMD_UIDL "UIDL"  //会话的标识号
#define CMD_TOP "TOP"    //返回第n个邮件的第n行内容
#define CMD_DOT  "."
#define CMD_QUIT "QUIT"

#define DEBUG
#ifdef DEBUG
#define LOG(...)\
	do\
	{\
	char reco_msg[1024] = {'\0'};\
	int n = sprintf(reco_msg,"[%s%s:%d]",_TIME_,_FILE_,_LINE_);\
	sprintf(reco_msg+i,_VA_ARGS_);\
	elog(reco_msg);\
 }while(0);
#else
#define LOG(...);

#endif

int elog(char *msg);
int shm_init();
int shm_del();
void sig_clean(int signo);
void sig_wait(int signo);
int read_line(int fd, char *buf, int size);
int sep_buf(char *buf, char *key, char *val);
void do_user(int fd, char *param);
void do_pass(int fd, char *param);
int do_stat(int fd);
int do_list(int fd, char *param);
int do_retr(int fd, char *param);
int do_dele(int fd, char *param);
int do_rset(int fd, char *param);
int do_uidl(int fd, char *param);
int do_top(int fd, char *param);
int do_dot(int fd, char *param);
int do_quit(int fd, char *param);
int do_other(int fd);
int timeout(int fd);
int process(int clientfd);
int pop3_ser();

#endif /* POP3_SER_H_ */
