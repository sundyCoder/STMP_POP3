/*
 * tcp.h
 *
 *  Created on: 2012-8-11
 *      Author: root
 */

#ifndef TCP_H_
#define TCP_H_
#include <netinet/in.h>

int socket_();
int bind_(int *sockfd, struct sockaddr_in *addr);
int listen_(int *sockfd);
int accept_(int *sockfd);

int connect_(int *sockfd,struct sockaddr_in *addr);

int reuse_addr(int fd);

#endif /* TCP_H_ */
