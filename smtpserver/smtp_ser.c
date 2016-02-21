/*
 * smtp_ser.c
 *
 *  Created on: 2012-8-11
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sqlite3.h>
#include <time.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "smtp_ser.h"

#define POET 25

#define MAIL_BOX "/opt/mailbox"

char content[10 * 1024 * 1024];
pthread_t tid;
char mailfrom[64];

sqlite3 *pdb = NULL;
MailLink *head = NULL;

//生成日志
int elog(char* msg)
{
        FILE* fp = fopen("debug.log","a+");
        if (fp == NULL){
                perror("In elog:fopen debug.log error!");
                return 1;
        }
        fputs(msg,fp);
        fclose(fp);
        return 0;
}

void sig_wait(int p) {
	wait(NULL);
	LOG("catch %d\n", p);
	return;
}

void sig_clean(int p){
	shm_del();
	LOG("catch %d\n",p);
	return ;
}

int read_line(int clientfd, char *buf, int size) {
	int i = 0;
	for (i = 0; i < size - 2; ++i) {
		int n = recv(clientfd, buf + i, 1, 0); //一次只读一个字节
		if (1 == n) {
			if (buf[i] == '\n')
				break;
		} else {
			perror("readLine");
			return -1;
		}
	}
	buf[++i] = '\0';
	return i;
}

void sep_buf(char *buf, char *key, char *val) {
	char *p = strchr(buf, ' ');
	if (NULL != p) {
		*p++ = '\0';
		char *q = strstr(p, "\r\n");
		if (NULL != q)
			*q = '\0';
	} else {
		p = strstr(buf, "\r\n");
		if (NULL != p)
			*p = '\0';
	}
	strcpy(key, buf);
	strcpy(val, p);
	return;
}

int do_helo(int fd, char *param) {
	send(fd, SMTP_SUC, strlen(SMTP_SUC), 0);
	return 0;
}

int do_ehlo(int fd, char *param) {
	char res[512] = { '\0' };
	strcat(res, "250-ch.com\r\n");
	strcat(res, "250-mail\r\n");
	strcat(res, "250-PIPELINING\r\n");
	strcat(res, "250-8BITMIME\r\n");
	strcat(res, "250-AUTH printfIN PLAIN\r\n");
	strcat(res, "250-AUTH=printfIN PLAIN\r\n");
	strcat(res, "250 STARTTLS cc35dfcd-7df9-48a2-afd2-c129da541a0f\r\n");
	send(fd, res, strlen(res), 0);
	return 0;
}

int auth_pass(char *username, char *password) {
	int ret = sqlite3_open("smtp.db3", &pdb);
	if (SQLITE_OK != ret) {
		perror("open smtp.db3");
		exit(1);
	}

	char sql[128] = { '\0' };
	sprintf(sql, "select * from auth");
	sqlite3_stmt *stmt;
	ret = sqlite3_prepare(pdb, sql, strlen(sql), &stmt, NULL);
	if (SQLITE_OK != ret) {
		perror("sqlite3_prepare\n");
		exit(1);
	}

	while (SQLITE_ROW == sqlite3_step(stmt)) {
		int i = -1;
		char *uname = (char *) sqlite3_column_text(stmt, ++i);
		char *pwd = (char *) sqlite3_column_text(stmt, ++i);
		LOG("uname = %s,pwd = %s\n", uname, pwd);
		if (strncmp(uname, username, strlen(username) - 2) == 0 && strncmp(pwd,
				password, strlen(password) - 2) == 0) {
			return 1;
		}
	}
	return 0;
}

int do_auth(int fd, char *param) {
	while (1) {
		char username[128], password[128]; //登录到smtp服务器的username和password
		char buf[128] = "334 VXNlcm5hbWU6\r\n"; //username:经过base64加密之后为:VXNlcm5hbWU6
		send(fd, buf, strlen(buf), 0);
		read_line(fd, username, sizeof(username));
		strcpy(buf, "334 UGFzc3dvcmQ6\r\n"); //password：经过base64加密之后为:UGFzc3dvcmQ6
		send(fd, buf, strlen(buf), 0);
		read_line(fd, password, sizeof(password));
		LOG("%d:username=%s,%d:password=%s\n", strlen(username), username,
				strlen(password), password);
		int ret = auth_pass(username, password);
		if (ret) {
			strcpy(buf, "235 OK, go ahead\r\n");
			send(fd, buf, strlen(buf), 0);
			return 1;
		} else {
			strcpy(buf, "500 ERR,input again!\r\n");
			send(fd, buf, strlen(buf), 0);
			continue;
		}
	}
}

int do_mail(int fd, char *param) {
	strcpy(mailfrom, param); //from: <chen@ch.com>
	send(fd, SMTP_SUC, strlen(SMTP_SUC), 0);
	return 0;
}

int get_uname(char *param) {
	char *p = strchr(param, '<');
	if (NULL != p) {
		*p++ = '\0';
		char *q = strchr(p, '>');
		if (NULL != q) {
			*q = '\0';
		}
	}
	strcpy(param, p);
	return 0;
}

void get_ip_by_domain_from_db(char *domain, char *ip) {
	int ret = sqlite3_open("smtp.db3", &pdb);
	if (SQLITE_OK != ret) {
		perror("sqlite3_open error!");
		return;
	} else {
		LOG("open smtp.db3 successful!\n");
	}

	char sql[128] = { '\0' };
	sprintf(sql, "select * from domain");//where name='%s'", username
	sqlite3_stmt *stmt;
	ret = sqlite3_prepare(pdb, sql, strlen(sql), &stmt, NULL);
	if (SQLITE_OK != ret) {
		perror("sqlite3_prepare error!\n");
		return;
	}

	while (SQLITE_ROW == sqlite3_step(stmt)) {
		int i = -1;
		char *name = (char *) sqlite3_column_text(stmt, ++i);
		char *ipstr = (char *) sqlite3_column_text(stmt, ++i);
		LOG("name = %s,ipstr = %s\n", name, ipstr);
		if (strcmp(ip, ipstr) == 0 && strcmp(domain, name)) {
			strcpy(ip, ipstr);
			break;
		}
	}
	sqlite3_finalize(stmt);
	sqlite3_close(pdb);
}

int size() {
	MailLink *p = head;
	int n = 0;
	while (NULL != p) {
		++n;
		p = p->next;
	}
	return n;
}

MailLink *getPointer(int pos) {
	MailLink *ml = head;
	int i = 0;
	while (NULL != ml) {
		++i;
		if (i == pos)
			return ml;
		ml = ml->next;
	}
	return NULL;
}

void insert_back(char* name, char* domain) {

	MailLink* ml = malloc(sizeof(MailLink));

	strcpy(ml->mail_from, mailfrom);
	strcpy(ml->mail_to, name);
	strcpy(ml->domain, domain);
	char ip[32] = { '\0' };
	//get_ip_by_domain(domain, ip);
	get_ip_by_domain_from_db(domain, ip);
	strcpy(ml->ip, ip);
	ml->next = NULL;
	if (head == NULL) {
		head = ml;
		return;
	}
	MailLink* p = head;
	while (p->next != NULL)
		p = p->next;

	p->next = ml;

}

int do_rcpt(int fd, char *param) {
	send(fd, SMTP_SUC, strlen(SMTP_SUC), 0); //to:<hong@ch.com>
	char uname[32] = { '\0' };
	strcpy(uname, param);
	get_uname(uname); //uname = hong@ch.com

	char *p, *q;
	p = strchr(param, '<');
	if (p) {
		++p;
		q = strchr(p, '@'); //p = hong
		if (q) {
			*q++ = '\0';
			char *r = strchr(q, '>'); //q = ch.com
			if (r)
				*r = '\0';
		}
	}
	//printf("%s%s", uname, q);
	if (mailfrom != NULL) {
		insert_back(uname, q);
	} else {
		char emsg[32] = "Please input your own mail!\n";
		send(fd, emsg, strlen(emsg), 0);
	}
	return 0;
}

void mail_transfer(void* p) {
	MailLink* ml = p;

	LOG("\n================transfer...=================\n");

	int sockfd = socket_();

	struct sockaddr_in ser_addr;
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = inet_addr("192.168.1.115");
	ser_addr.sin_port = htons(25);
	bzero(ser_addr.sin_zero, sizeof(ser_addr.sin_zero));
	connect_(&sockfd, &ser_addr);

	char buf[128] = { '\0' };
	read_line(sockfd, buf, sizeof(buf));
	LOG("recv1:%s\n", buf);
	if (strncasecmp(buf, "220", 3) != 0)
		goto err;
	char req[128] = "HELO ch\r\n";
	send(sockfd, req, strlen(req), 0);
	read_line(sockfd, buf, sizeof(buf));
	LOG("recv2:%s\n", buf);
	if (strncasecmp(buf, "250", 3) != 0)
		goto err;

	sprintf(req, "MAIL %s\r\n", ml->mail_from);
	send(sockfd, req, strlen(req), 0);
	read_line(sockfd, buf, sizeof(buf));
	LOG("recv3:%s\n", buf);
	if (strncasecmp(buf, "250", 3) != 0)
		goto err;

	sprintf(req, "RCPT %s\r\n", ml->mail_to);
	send(sockfd, req, strlen(req), 0);
	read_line(sockfd, buf, sizeof(buf));
	LOG("recv4:%s\n", buf);
	if (strncasecmp(buf, "250", 3) != 0)
		goto err;

	strcpy(req, "DATA\r\n");
	send(sockfd, req, strlen(req), 0);
	read_line(sockfd, buf, sizeof(buf));
	LOG("recv5:%s\n", buf);
	if (strncasecmp(buf, ">\r\n354", 5) != 0)
		goto err;

	send(sockfd, content, strlen(content), 0);
	strcpy(req, ".\r\n");
	send(sockfd, req, strlen(req), 0);
	read_line(sockfd, buf, sizeof(buf));
	LOG("recv6:%s\n", buf);
	if (strncasecmp(buf, "250", 3) != 0)
		goto err;

	strcpy(req, "QUIT\r\n");
	send(sockfd, req, strlen(req), 0);
	read_line(sockfd, buf, sizeof(buf));
	LOG("recv7:%s\n", buf);
	if (strncasecmp(buf, "221", 3) != 0)
		goto err;
	LOG("transfer success!\n");
	LOG("\n===========after transfer====================\n");
	return;
	err:
	LOG("server response failure\n");
}

void *mail_send(void *p) {
	MailLink *ml = p;
	if (strcmp(ml->domain, "ch.com") == 0) {
		char filename[128] = { '\0' };
		char tbuf[128] = { '\0' };
		get_now_time(tbuf);
		sprintf(filename, "%s.txt", tbuf);

		char mail_path[256] = { '\0' };
		char mail_pac[256] = { '\0' };
		sprintf(mail_pac, "%s/%s", MAIL_BOX, ml->mail_to);
		char command[256] = { '\0' };
		sprintf(command, "%s %s", "mkdir -p", mail_pac);
		system(command);
		sprintf(mail_path, "%s/%s/%s", MAIL_BOX, ml->mail_to, filename);

		FILE* fp = fopen(mail_path, "w+");
		if (fp == NULL) {
			perror("In mail_send:fopen");
			return NULL;
		}
		int size = fwrite(content, 1, strlen(content), fp);
		printf("local handle:%d,%d\n", strlen(content), size);
		fclose(fp);

	} else {
		mail_transfer(p); // 如果所发的邮件的域名不存在的话，将此邮件依然回发给smtp客户端。
	}
	return NULL;
}

void * mail_handler(void *param) {
	int n = size();
	MailLink* p = head;
	pthread_t tids[n];    //处理群发的功能
	int i = 0;
	while (p != NULL) {
		LOG("name=%s,domain=%s,ip=%s\n", p->mail_to, p->domain, p->ip);
		pthread_create(&tids[i], NULL, mail_send, p);
		p = p->next;
		++i;
	}
	for (i = 0; i < n; ++i) {
		pthread_join(tids[i], NULL);
	}
	return NULL;
}

void get_now_time(char *ntm) {
	long t = time(NULL);
	struct tm *nt = localtime(&t);
	char tbuf[128] = { '\0' };
	strftime(tbuf, sizeof(tbuf), "%Y-%m-%d-%H:%M:%S", nt);
	strcpy(ntm, tbuf);
	return;
}

int do_data(int fd, char *param) {
	send(fd, SMTP_CON, strlen(SMTP_CON), 0);

	char tbuf[128] = { '\0' };
	get_now_time(tbuf);
	char start[] =
			"===================Welcome.==============================\n";
	sprintf(content, "%s%s time:%s\nContent:\n", start, mailfrom, tbuf);
	while (1) {
		char buf[256] = { '\0' };
		int size = read_line(fd, buf, sizeof(buf));
		if (size < 0)
			break;
		if (size == 3 && strncasecmp(buf, CMD_DOT, 1) == 0)
			break;
		strcat(content, buf);
	}
	char end[] = "===================@copyright JChen======================\n";
	strcat(content, end);

	send(fd, SMTP_SUC, strlen(SMTP_SUC), 0);

	int ret = pthread_create(&tid, NULL, mail_handler, NULL);
	if (ret)
		perror("pthread_create");
	return 0;

}

int do_reset(int fd, char *param) {
	//所有信息重置。
	MailLink *ml = getPointer(size());
	if (ml != NULL) {
		memset(mailfrom, '\0', sizeof(mailfrom));
		memset(content, '\0', sizeof(content));
		memset(ml->domain, '\0', sizeof(ml->domain));
		memset(ml->mail_from, '\0', sizeof(ml->mail_from));
		memset(ml->ip, '\0', sizeof(ml->ip));
		ml->next = NULL;
	} else {
		memset(mailfrom, '\0', sizeof(mailfrom));
		memset(content, '\0', sizeof(content));
	}
	write(fd, CMD_RSET, strlen(CMD_RSET));
	write(fd, "\r\n", 2);
	return 0;
}

int do_quit(int fd, char *param) {
	send(fd, SMTP_QUIT, strlen(SMTP_QUIT), 0);

	int shmid = shmget(1234, 512, IPC_CREAT | 0600);
	int *max = shmat(shmid, NULL, 0);
	//shm_del();
	(*max)--;

	LOG("a client disconnected!current client num=%d\n",*max);
	printf("fd = %d\n", fd);
	return 0;
}

int do_other(int fd, char *param) {
	send(fd, SMTP_FAL, strlen(SMTP_FAL), 0);
	return 0;
}

int process(int clientfd) {
	int shmid = shmget(1234, 512, IPC_CREAT | 0600);
	int *max = shmat(shmid, NULL, 0);

	struct sockaddr_in c_addr;
	unsigned int c_len = sizeof(struct sockaddr);
	getpeername(clientfd, (struct sockaddr *) &c_addr, &c_len);
	LOG("a client connected:ip=%s,port=%d,current client num=%d\n", inet_ntoa(c_addr.sin_addr),
			c_addr.sin_port,*max);

	send(clientfd, SMTP_READY, strlen(SMTP_READY), 0);

	while (1) {
		char buf[128] = { '\0' }, key[32], val[64];
		int size = read_line(clientfd, buf, sizeof(buf));
		if (size <= 0)
			break;

		sep_buf(buf, key, val);
		LOG("key=%s,val=%s\n", key, val);

		if (strcasecmp(CMD_HELO, key) == 0) {
			do_helo(clientfd, val);
		} else if (strcasecmp(CMD_EHLO, key) == 0) {
			do_ehlo(clientfd, val);
		} else if (strcasecmp(CMD_AUTH, key) == 0) {
			do_auth(clientfd, val);
		} else if (strcasecmp(CMD_MAIL, key) == 0) {
			do_mail(clientfd, val);
		} else if (strcasecmp(CMD_RCPT, key) == 0) {
			do_rcpt(clientfd, val);
		} else if (strcasecmp(CMD_DATA, key) == 0) {
			do_data(clientfd, val);
		} else if (strcasecmp(CMD_RSET, key) == 0) {
			do_reset(clientfd, val);
		} else if (strcasecmp(CMD_QUIT, key) == 0) {
			printf("clientfd = %d\n", clientfd);
			do_quit(clientfd, val);
			break;
		} else {
			do_other(clientfd, val);
		}
	}
	close(clientfd);
	pthread_join(tid, NULL); //!!!!
	return 0;
}

int smtp_ser() {
	int sockfd = socket_(NULL);
	struct sockaddr_in ser_addr;
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ser_addr.sin_port = htons(25);
	bzero(ser_addr.sin_zero, sizeof(ser_addr.sin_zero));

	reuse_addr(sockfd);

	bind_(&sockfd, &ser_addr);
	listen_(&sockfd);
	LOG("JChen's smtp server is ready!\n");

	shm_init();
	signal(SIGTERM,sig_clean);
	signal(SIGCHLD, sig_wait);

	int shmid = shmget(1234, 512, IPC_CREAT | 0600);
	int *max = shmat(shmid, NULL, 0);

	while (1) {
		int clientfd = accept_(&sockfd);
		if (-1 == clientfd) {
			perror("accept");
			continue;
		}

		int pid = fork();
		if (-1 == pid) {
			exit(1);
		} else if (0 == pid) {
			(*max)++;
			process(clientfd);
		}
		close(clientfd);
	}
	close(sockfd);
	return 0;
}

int shm_init() {
	int shmid = shmget(1234, 512, IPC_CREAT | 0600);
	int *max = shmat(shmid, NULL, 0);
	(*max) = 0;
	return 0;
}

int shm_del() {
	int shmid = shmget(1234, 512, IPC_CREAT | 0600);
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
		printf("delete shm error\n");
		exit(0);
	}
	return 0;
}


