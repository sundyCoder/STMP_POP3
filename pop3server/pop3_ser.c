/*
 * pop3_ser.c
 *
 *  Created on: 2012-8-12
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <wait.h>
#include <strings.h>
#include <string.h>
#include <sqlite3.h>
#include <fcntl.h>
#include "md5.h"

#include "pop3_ser.h"

#define PORT 110
#define MAIL_BOX "/opt/mailbox"

int shmid = 0;
char username[64];
char password[64];

int elog(char *msg) {
	FILE *fp = fopen("debug.log", "a+");
	if (NULL != fp) {
		perror("fopen");
		exit(1);
	}

	fputs(msg, fp);
	return 0;
}

int shm_init() {
	int shmid = shmget(4321, 512, IPC_CREAT | 0600);
	int *max = shmat(shmid, NULL, 0);
	(*max) = 0;
	return 0;
}

int shm_del() {
	int shmid = shmget(4321, 512, IPC_CREAT | 0600);
	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
		perror("shmdel");
		exit(1);
	}
	return 0;
}

void sig_clean(int signo) {
	printf("catch %d\n", signo);
	shm_del();
}

void sig_wait(int signo) {
	wait(NULL);
	printf("catch %d\n", signo);
}

int read_line(int fd, char *buf, int size) {
	int i = 0;
	for (i = 0; i < size - 2; ++i) { // \r\n
		int n = recv(fd, buf + i, 1, 0);
		//		printf("read_line n = %d\n", n);
		if (1 == n) {
			if (buf[i] == '\n') {
				break;
			}
		} else {
			perror("readLine!");
			return -1;
		}
	}
	buf[++i] = '\0';
	return i;
}

int sep_buf(char *buf, char *key, char *val) {
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

	//	printf("buf=%s,key =%s,val=%s\n",buf,key,val);
	strcpy(key, buf);
	strcpy(val, p);
	printf("key = %s,val = %s\n", key, val);
	return 0;
}

void do_user(int fd, char *param) {
	printf("In do_user param=%s\n", param);
	strcpy(username, param);
	write(fd, RESP_SUC, strlen(RESP_SUC));
	return;
}

sqlite3 *pdb = NULL;

void do_pass(int fd, char *param) {
	printf("in do_pass:param = %s\n", param);
	int ret = sqlite3_open("pop3.db3", &pdb);
	if (SQLITE_OK != ret) {
		write(fd, DB_BUSY, sizeof(DB_BUSY));
		printf("sqlite3_open error!");
		return;
	}

	char sql[128] = { '\0' };
	sprintf(sql, "select * from user");//where name='%s'", username
	sqlite3_stmt *stmt;
	ret = sqlite3_prepare(pdb, sql, strlen(sql), &stmt, NULL);
	if (SQLITE_OK != ret) {
		write(fd, DB_BUSY, sizeof(DB_BUSY));
		printf("sqlite3_prepare error!");
		return;
	}

	int flag = 0;
	while (SQLITE_ROW == sqlite3_step(stmt)) {
		int i = -1;
		char *name = (char *) sqlite3_column_text(stmt, ++i);
		char *pwd = (char *) sqlite3_column_text(stmt, ++i);
		printf(
				"strlen(usernaem) = %d,username = %s,strlen(param)=%d,param = %s\n",
				strlen(username), username, strlen(param), param);
		printf("name = %s, pwd = %s\n", name, pwd);

		if (strncmp(name, username, strlen(username) - 2) == 0 && strncmp(pwd,
				param, strlen(param) - 2) == 0) {
			flag = 1;
			break;
		}
	}
	sqlite3_finalize(stmt);
	sqlite3_close(pdb);
	if (flag) {
		write(fd, RESP_SUC, strlen(RESP_SUC));
	} else {
		write(fd, RESP_ERR, strlen(RESP_ERR));
		exit(-1);
	}
	sqlite3_close(pdb);
	printf("pass success end......\n");
	return;
}

int do_stat(int fd) {
	printf("do_stat testing...........\n");
	char mail_path[128] = { '\0' };
	sprintf(mail_path, "%s/%s", MAIL_BOX, username);

	printf("in do_stat:mail_path=%s\n", mail_path);

	int i = 0, sum = 0;
	DIR * dir = opendir(mail_path);
	struct dirent *dirent = NULL;
	while ((dirent = readdir(dir)) != NULL) {
		if ((strcmp(dirent->d_name, ".") != 0) && (strcmp(dirent->d_name, "..")
				!= 0)) {
			char temp[128] = { '\0' };
			sprintf(temp, "%s/%s", mail_path, dirent->d_name);
			printf("temp = %s\n", temp);
			struct stat buf;
			stat(temp, &buf);

			sum += buf.st_size;
			++i;
		} else {
			continue;
		}
	}

	closedir(dir);
	char result[32] = { '\0' };
	sprintf(result, "+OK %d %d\r\n", i, sum);
	send(fd, result, sizeof(result), 0);
	return i;
}

int do_list(int fd, char *param) {
	char path[128] = { '\0' };
	sprintf(path, "%s/%s", MAIL_BOX, username);

	DIR *pdir = opendir(path);
	struct dirent *pdirent = NULL;
	int i = 0, sum = 0;
	char tempres[32] = { '\0' };
	while ((pdirent = readdir(pdir)) != NULL) {
		if (strcmp(pdirent->d_name, ".") == 0 || strcmp(pdirent->d_name, "..")
				== 0)
			continue;
		char temp[128] = { '\0' };
		sprintf(temp, "%s/%s", path, pdirent->d_name);
		printf("list temp = %s\n",temp);
		struct stat buf;
		stat(temp, &buf);
		sum += buf.st_size;
		i++;
		char r[32] = { '\0' };
		sprintf(r, "%d %ld\r\n", i, buf.st_size);
		strcat(tempres, r);
	}
	closedir(pdir);

	char result[32] = { '\0' };
	sprintf(result, "+OK %d %d\r\n", i, sum);
	strcat(result, tempres);
	strcat(result, ".\r\n");
	write(fd, result, strlen(result));
	return 0;
}

int do_retr(int fd, char *param) {
	char path[128] = { '\0' };
	sprintf(path, "%s/%s", MAIL_BOX, username);

	DIR *pdir = opendir(path);
	struct dirent *pdirent = NULL;
	int i = 1, num = 0, num2 = 0;
	char *content = NULL;
	while ((pdirent = readdir(pdir)) != NULL) {
		if (strcmp(pdirent->d_name, ".") == 0 || strcmp(pdirent->d_name, "..")
				== 0)
			continue;
		if (i == atoi(param)) {
			char temp[128] = { '\0' };
			sprintf(temp, "%s/%s", path, pdirent->d_name);
			struct stat buf;
			stat(temp, &buf);
			num = buf.st_size;
			content = malloc(num * 2);
			memset(content, '\0', num * 2);
			int rfd = open(temp, O_RDONLY);
			num2 = read(rfd, content, num);
			printf("content size:%d,%d\n", num, num2);
			close(rfd);
			break;
		}
		++i;
	}
	closedir(pdir);
	char result[32] = { '\0' };
	sprintf(result, "+OK %d\r\n", num);
	write(fd, result, strlen(result));
	strcat(content, ".\r\n");
	write(fd, content, strlen(content));
	if (content != NULL) {
		printf("free:%p\n", content);
		free(content);
	}
	return 0;
}

int do_dele(int fd, char *param) {
	char path[128] = { '\0' };
	sprintf(path, "%s/%s", MAIL_BOX, username);
	DIR *dir = opendir(path);
	struct dirent *dirent = NULL;
	int i = 1;

	while ((dirent = readdir(dir)) != NULL) {
		if ((strcmp(dirent->d_name, ".") != 0) && (strcmp(dirent->d_name, "..")
				!= 0)) {
			char temp[128] = { '\0' };
			sprintf(temp, "%s/%s", path, dirent->d_name);
			if (atoi(param) <= do_stat(fd)) {
				if (atoi(param) == i) {
					char cmd1[32] = "rm -f ";
					char cmd[64] = { '\0' };
					sprintf(cmd, "%s%s", cmd1, temp);
					system(cmd);
					char result[32] = { '\0' };
					sprintf(result, "+OK dele %d:%s", i, dirent->d_name);
					printf("%s\n", result);
					write(fd, RESP_SUC, strlen(RESP_SUC));
					return 1;
				}
				i++;
			} else {
				printf("No such file!\n");
			}
		} else {
			continue;
		}
	}
	closedir(dir);
	return 0;
}

int do_rset(int fd, char *param) {
	memset(username, '\0', sizeof(username));
	memset(password, '\0', sizeof(password));
	write(fd, RESP_SUC, strlen(RESP_SUC));
	return 0;
}

int do_uidl(int fd, char *param) {
	char path[128] = { '\0' };
	sprintf(path, "%s/%s", MAIL_BOX, username);
	if (param == NULL || param[0] == '\0') {
		DIR *pdir = opendir(path);
		struct dirent *pdirent = NULL;
		int i = 0, sum = 0;
		char tempres[1024] = { '\0' };
		char filename_md5[128] = { '\0' };
		while ((pdirent = readdir(pdir)) != NULL) {
			if (strcmp(pdirent->d_name, ".") == 0 || strcmp(pdirent->d_name,
					"..") == 0)
				continue;
			char temp[128] = { '\0' };
			sprintf(temp, "%s/%s", path, pdirent->d_name);
			struct stat buf;
			stat(temp, &buf);
			sum += buf.st_size;
			//md5 filename
			md5_in((unsigned char *) pdirent->d_name,
					(unsigned char *) filename_md5);
			i++;
			char r[32] = { '\0' };
			sprintf(r, "%d %s\r\n", i, filename_md5);
			strcat(tempres, r);
		}
		closedir(pdir);
		char result[32] = { '\0' };
		sprintf(result, "+OK %d %d\r\n", i, sum);
		strcat(result, tempres);
		strcat(result, ".\r\n");
		write(fd, result, strlen(result));
	} else {
		DIR *pdir = opendir(path);
		struct dirent *pdirent = NULL;
		int i = 1;
		char filename_md5[128] = { '\0' };
		while ((pdirent = readdir(pdir)) != NULL) {
			if (strcmp(pdirent->d_name, ".") == 0 || strcmp(pdirent->d_name,
					"..") == 0)
				continue;
			if (i == atoi(param)) {
				md5_in((unsigned char *) pdirent->d_name,
						(unsigned char *) filename_md5);
				break;
			}
			++i;
		}
		closedir(pdir);
		char result[32] = { '\0' };
		sprintf(result, "+OK %d %s\r\n", i, filename_md5);
		write(fd, result, strlen(result));
	}
	return 0;
}

int do_top(int fd, char *param) {
	send(fd, RESP_SUC, strlen(RESP_SUC), 0);
	return 0;
}

int do_quit(int fd, char *param) {
	return 0;
}

int do_other(int fd) {
	send(fd, RESP_ERR, sizeof(RESP_ERR), 0);
	return 0;
}

int timeout(int fd) {
	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = 300;
	tv.tv_usec = 0;

	int retval = select(fd + 1, &rfds, NULL, NULL, &tv);
	return retval;
}

int process(int clientfd) {

	shmid = shmget(4321, 512, IPC_CREAT | 0600);
	int *max = shmat(shmid, NULL, 0);
	(*max)++;

	struct sockaddr_in c_addr;
	unsigned int c_len = sizeof(struct sockaddr_in);
	getpeername(clientfd, (struct sockaddr *) &c_addr, &c_len);
	printf("a client connected! ip=%s,port=%d,current client=%d\n", inet_ntoa(
			c_addr.sin_addr), c_addr.sin_port, *max);

	send(clientfd, SERVER_READY, sizeof(SERVER_READY), 0);

	while (1) {
		char buf[128] = { '\0' }, key[64] = { '\0' }, val[64] = { '\0' };

		int ret = timeout(clientfd);

		if (ret > 0) {
			int n = read_line(clientfd, buf, sizeof(buf));
			if (n < 0)
				break;

			sep_buf(buf, key, val);
			printf("buf=%s,key=%s,val=%s\n", buf, key, val);
			if (strcasecmp(key, CMD_USER) == 0) {
				do_user(clientfd, val);
			} else if (strcasecmp(key, CMD_PASS) == 0) {
				do_pass(clientfd, val);
			} else if (strcasecmp(key, CMD_STAT) == 0) {
				do_stat(clientfd);
			} else if (strcasecmp(key, CMD_LIST) == 0) {
				do_list(clientfd, val);
			} else if (strcasecmp(key, CMD_RETR) == 0) {
				do_retr(clientfd, val);
			} else if (strcasecmp(key, CMD_DELE) == 0) {
				do_dele(clientfd, val);
			} else if (strcasecmp(key, CMD_RSET) == 0) {
				do_rset(clientfd, val);
			} else if (strcasecmp(key, CMD_UIDL) == 0) {
				do_uidl(clientfd, val);
			} else if (strcasecmp(key, CMD_TOP) == 0) {
				do_top(clientfd, val);
			} else if (strcasecmp(key, CMD_QUIT) == 0) {
				do_quit(clientfd, val);
				break;
			} else {
				do_other(clientfd);
			}
			memset(buf, '\0', sizeof(buf));
			memset(key, '\0', sizeof(key));
			memset(val, '\0', sizeof(val));
		} else if (ret == 0) {
			send(clientfd, RESP_TIMEOUT, sizeof(RESP_TIMEOUT), 0);
			printf("time out!");
			break;
		} else {
			printf("other error\n");
			break;
		}
	}
	return 0;
}

int pop3_ser() {
	int sockfd = socket_();
	struct sockaddr_in ser_addr;
	ser_addr.sin_family = PF_INET;
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ser_addr.sin_port = htons(PORT);
	bzero(ser_addr.sin_zero, sizeof(ser_addr.sin_zero));

	reuse_addr(sockfd);

	bind_(&sockfd, &ser_addr);

	listen_(&sockfd);
	printf("JChen's pop3 server is ready!\n");

	shm_init();
	signal(SIGTERM, sig_clean); //当客户端退出时，销毁它占的共享內存
	signal(SIGCHLD, sig_wait); //


	while (1) {
		int clientfd = accept_(&sockfd);

		shmid = shmget(6666, 512, 0600 | IPC_CREAT);
		int *max = shmat(shmid, 0, 0); //the number of client
		if (*max + 1 > 3) {
			write(clientfd, RESP_BUSY, strlen(RESP_BUSY));
			close(clientfd);
			continue;
		}

		int pid = fork();
		if (-1 == pid) {
			perror("fork");
			exit(0);
		} else if (pid == 0) {
			process(clientfd);
		}
		close(clientfd);
	}
	close(sockfd);
	return 0;
}
