/*
 * smtp_ser.h
 *
 *  Created on: 2012-8-11
 *      Author: root
 */

#ifndef SMTP_SER_H_
#define SMTP_SER_H_
#include "tcp.h"

#define SMTP_READY  "220 welcome to JChen's smtp server!\r\n"
#define SMTP_SUC    "250 JChen\r\n"
#define SMTP_CON    "354 end with .\r\n"
#define SMTP_FAL    "500 unknown command\r\n"
#define SMTP_QUIT   "221 JChen\r\n"

#define CMD_HELO "HELO"
#define CMD_EHLO "EHLO"
#define CMD_AUTH "AUTH"
#define CMD_MAIL "MAIL"
#define CMD_RCPT "RCPT"
#define CMD_RSET "RSET"
#define CMD_DATA "DATA"
#define CMD_DOT  "."
#define CMD_QUIT "QUIT"


////LOG需要C99的支持，可以正式支持“可变参数宏”
//#define DEBUG
//#ifdef DEBUG
//#define LOG(...) printf(__VA_ARGS__)
//#else
//#define LOG(...) ;
//#endif


#define DEBUG
#ifdef DEBUG
#define LOG(...) \
do\
{\
        char format_msg[1024]={0};\
        int i = sprintf(format_msg,"[%s %s:%d]:",__TIME__,__FILE__,__LINE__);\
        sprintf(format_msg+i,__VA_ARGS__);\
        elog(format_msg);\
}while(0)
#else
#define LOG(...) ;
#endif



typedef struct mail{
	char mail_from[64];   // <root@ch.com>
	char mail_to[64];	   // <chen@ch.com>
	char domain[128];    // ch.com
	char ip[32];       //ch.com的ip
	struct mail *next;
}MailLink;

int elog(char* msg);

void sig_wait(int p);
int read_line(int clientfd, char *buf, int size);
void sep_buf(char *buf, char *key, char *val);


int do_helo(int fd, char *param);
int do_ehlo(int fd, char *param);
int auth_pass(char *username, char *password);
int do_auth(int fd, char *param);
int do_mail(int fd, char *param);
int get_uname(char *param);
void get_ip_by_domain_from_db(char *domain, char *ip);
void insert_back(char* name, char* domain);
int do_rcpt(int fd, char *param);
void *mail_send(void *p);
void * mail_handler(void *p);
void get_now_time(char *ntm);
int do_data(int fd, char *param);
int do_reset(int fd, char *param);
int do_quit(int fd, char *param);
int do_other(int fd, char *param);

int process(int clientfd);
int smtp_ser();

int shm_init();
int shm_del();


#endif /* SMTP_SER_H_ */
