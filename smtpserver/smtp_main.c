/*
 * smtp_main.c
 *
 *  Created on: 2012-8-12
 *      Author: root
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "tcp.h"
#include "smtp_ser.h"

int main(void) {
	daemon(0, 0);
	signal(SIGCHLD, SIG_IGN);

	smtp_ser();
	return 0;
}
