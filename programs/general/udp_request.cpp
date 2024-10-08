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

#include "agent/agentclass.h"
#include "support/jsondef.h"
#include "support/sliplib.h"

char address[] = "127.0.0.2";
//uint16_t port = 6868;
uint16_t port = 10020;

uint16_t bsize = 1500;
uint16_t delay = 1;
uint32_t packets = 10000;
float speed = 281250.;
double cmjd, mjd;


int main(int argc, char *argv[])
{

    int32_t iretn = 0;
    uint32_t count=0;
    socket_channel chan;
    socket_channel chanin;
    uint8_t buf3[10000];
    int32_t lsleep;
//    int32_t lat, lon, alt;
    uint16_t len3;
//    int32_t hour, min;
//    float sec;
    double imjd, elapsed;


    if ((iretn = socket_open(&chanin, NetworkType::UDP, address, port, SOCKET_LISTEN, SOCKET_BLOCKING, AGENTRCVTIMEO)) < 0)
    {
        printf("Unable to open connection to listen %d\n", iretn);
    }

    if ((iretn=socket_open(&chan, NetworkType::UDP, address, port, SOCKET_TALK, SOCKET_BLOCKING, AGENTRCVTIMEO)) < 0)
    {
        printf("Unable to open connection to [%s:6101]\n",address);
    }

    COSMOS_USLEEP(1*1000000);
//    lat = 3705459;
//    lon = -12083358;
//    alt = 50;
    imjd = currentmjd(0.);
    buf3[0] = 130;

    while (true)
    {

        count++;
        mjd = currentmjd(0.);
        elapsed = 86400. * (mjd - imjd);
        sprintf((char *)&buf3[3],"{\"agent_utc\":%.15g,\"agent_node\":\"%s\",\"agent_proc\":\"%s\",\"agent_addr\":\"%s\",\"agent_port\":%u,\"agent_bprd\":%f,\"agent_bsz\":%u,\"agent_cpu\":%f,\"agent_memory\":%f,\"agent_jitter\":%f,\"agent_dcycle\":%f,\"node_utcoffset\":%.15g}",
                mjd,
                "node",
                "process",
                chanin.address,
                chanin.cport,
                0,
                60000,
                .1,
                .2,
                elapsed,
                .4,
                0.);
        for (uint16_t i=1; i<argc; ++i)
        {
            sprintf((char *)&buf3[3+strlen((char *)&buf3[3])]," %s", argv[i]);
        }
        len3 = strlen((char *)&buf3[3]);
        buf3[1] = len3%256;
        buf3[2] = len3 / 256;

        // Send packet
        sendto(chan.cudp, (const char *)buf3, len3+3, 0, (struct sockaddr *)&chan.caddr, sizeof(struct sockaddr_in));

        printf("[%s]\n", &buf3[3]);
        recvfrom(chanin.cudp, &buf3[3], AGENTMAXBUFFER, 0, static_cast<struct sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));
        printf("[%s]\n", buf3);
        lsleep = 1*1000000;
        COSMOS_USLEEP(lsleep);
    }

}
