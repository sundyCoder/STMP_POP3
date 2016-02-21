/*
 * pop3_main.c
 *
 *  Created on: 2012-8-12
 *      Author: root
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "pop3_ser.h"

int main(void){
	daemon(0,0);
	signal(SIGCHLD,SIG_IGN);
	pop3_ser();
	return 0;
}
