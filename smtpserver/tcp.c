/*
 * tcp.c
 *
 *  Created on: 2012-8-11
 *      Author: root
 */
//为什么#include "tcp.h"放在这里会出错？
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "tcp.h"

#define MAX_LISTEN 10

int socket_() {
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		perror("socket");
		exit(1);
	}
	return sockfd;
}

int bind_(int *sockfd, struct sockaddr_in *addr) {
	int ret = bind(*sockfd, (struct sockaddr *)addr,
			sizeof(struct sockaddr));
	if (-1 == ret) {
		perror("bind");
		exit(1);
	}
	return ret;
}

int listen_(int *sockfd) {
	int ret = listen(*sockfd, MAX_LISTEN);
	if (-1 == ret) {
		perror("listen");
		exit(1);
	}
	return ret;
}

int accept_(int *sockfd){
	struct sockaddr_in c_addr;
	unsigned int c_len = sizeof(struct sockaddr);
	int clientfd = accept(*sockfd,(struct sockaddr *)&c_addr,&c_len);
	if (-1 == clientfd) {
			perror("accept");
			exit(1);
		}
	return clientfd;
}

int connect_(int *sockfd,struct sockaddr_in *addr){
	int ret = connect(*sockfd,(struct sockaddr *)addr,sizeof(struct sockaddr));
	if (-1 == ret) {
			perror("connect");
			exit(1);
		}
	return ret;
}

int reuse_addr(int fd){
	int opt = 1;
	int ret = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	return ret;
}

