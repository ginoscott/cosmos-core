/********************************************************************
* Copyright (C) 2015 by Interstel Technologies, Inc.
*   and Hawaii Space Flight Laboratory.
*
* This file is part of the COSMOS/core that is the central
* module for COSMOS. For more information on COSMOS go to
* <http://cosmos-project.com>
*
* The COSMOS/core software is licenced under the
* GNU Lesser General Public License (LGPL) version 3 licence.
*
* You should have received a copy of the
* GNU Lesser General Public License
* If not, go to <http://www.gnu.org/licenses/>
*
* COSMOS/core is free software: you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation, either version 3 of
* the License, or (at your option) any later version.
*
* COSMOS/core is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* Refer to the "licences" folder for further information on the
* condititons and terms to use this software.
********************************************************************/

#include "configCosmos.h"
#include "elapsedtime.h"
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <math.h>
#include <signal.h>
//#include <sys/time.h>

char command[50];
char outbuf[10000000];
int s;
void abort_func(int);

int main(int argc, char *argv[])
{
	char hostname[64];
	struct hostent *hp;
	long i, inchar, iretn, cnt;
	unsigned short port;
	long number, size;
	struct sockaddr_in sin;
	double srate, srate2;
	float rate;

	if (argc != 5)
	{
		fprintf(stderr,"netspeed host number size port");
		exit(1);
	}

	strcpy(hostname,argv[1]);
	number = atol(argv[2]);
	size = atol(argv[3]);
	port = atoi(argv[4]);

	if ((hp = gethostbyname(hostname)) == NULL)
	{
		fprintf(stderr, "%s: host unknown.\n", hostname);
		exit(1);
	}

#ifdef COSMOS_WIN_OS
	//	unsigned long nonblocking = 1;
	//	struct sockaddr_storage ss;
	//	int sslen;
	WORD wVersionRequested;
	WSADATA wsaData;
	static bool started=false;

	if (!started)
	{
		wVersionRequested = MAKEWORD( 1, 1 );
		iretn = WSAStartup( wVersionRequested, &wsaData );
	}
#endif

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("server: socket");
		exit(1);
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons((unsigned short)port);
	memcpy((char *)&sin.sin_addr, (char *)hp->h_addr, hp->h_length);
	signal(SIGINT, abort_func);

	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) <0)
	{
		perror("server: connect");
		exit(1);
	}
	cnt = 0;
	srate = srate2 = 0;
	do
	{


		//gettimeofday(&tp0,NULL);
		ElapsedTime et;
		et.start();
		sprintf(command,"%ld %ld",number,size);
		send(s, command, 50, 0);
		if (number < 0)
			number *= -1;
		for (i=0; i<number; i++)
		{
			send(s, outbuf, size, 0);
		}
		inchar = 0;
		do
		{
			iretn = recv(s, &command[inchar], 50-inchar, 0);
			if (iretn > 0)
				inchar += iretn;
		} while (inchar < 50);
//		gettimeofday(&tp1,NULL);
//		rate = (double)(number*size)/(1024.*((tp1.tv_sec+tp1.tv_usec/1e6)-(tp0.tv_sec+tp0.tv_usec/1e6)));
		rate = (double)(number*size)/(1024.*((et.lap())));
		srate += rate;
		srate2 += rate * rate;
		cnt++;
		if (cnt > 1)
		{
			printf("Current %7.0f KBPS Average %7.0f +- %7.5f%% KBPS\r",rate,srate/cnt,100.*sqrt((srate2-(srate*srate)/cnt)/(cnt-1))/(srate/cnt));
		}
		fflush(stdout);
	} while (1);
	shutdown(s, 2);
}

void abort_func(int)
{
	shutdown(s, 2);
	exit(2);
}
