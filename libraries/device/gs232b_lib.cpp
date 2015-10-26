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
//#if defined(COSMOS_WIN_OS) || defined(COSMOS_LINUX_OS) || defined(COSMOS_CYGWIN_OS) || defined(COSMOS_MAC_OS)

#include "gs232b_lib.h"
#include <cstring>
#include <cmath>

/**
* Internal descriptor for cssl serial control of GS-232B.
* @brief GS-232B serial handle
*/
static cssl_t *gs232b_serial = NULL;

static gs232b_state gs_state;

/**
* Connects to a Yaesu GS-232B computer controller, which in turn
* drives a Yaesu G-5500 antenna controller.
* @param dev pointer to a character string with the serial port it is
* connected to.
* @see cssl_start
* @see cssl_open
* @see cssl_setflowcontrol
*/
int32_t gs232b_connect(string dev)
{
	int32_t iretn;
	cssl_start();
	if (gs232b_serial != nullptr)
	{
		cssl_close(gs232b_serial);
		gs232b_serial = nullptr;
	}

	gs232b_serial = cssl_open(dev.c_str(), GS232B_BAUD, GS232B_BITS, GS232B_PARITY, GS232B_STOPBITS);
	if (gs232b_serial == nullptr)
	{
		return CSSL_ERROR_OPEN;
	}

	iretn = cssl_settimeout(gs232b_serial, 0, .5);
	if (iretn < 0)
	{
		return iretn;
	}

	iretn = gs232b_send((char *)"\r", true);
	if (iretn < 0)
	{
		return iretn;
	}
	char buf[100];
	iretn = gs232b_getdata(buf,100);
	if (iretn < 0)
	{
		return iretn;
	}
	iretn = gs232b_send((char *)"\r", true);
	if (iretn < 0)
	{
		return iretn;
	}
	iretn = gs232b_getdata(buf,100);
	if (iretn < 0)
	{
		return iretn;
	}
	if (buf[0] != '?' || buf[1] != '>')
	{
		return GS232B_ERROR_OPEN;
	}

	return 0;
}

/**
* Close currently open GS-232B.
*/
int32_t gs232b_disconnect()
{
	if (gs232b_serial == NULL)
		return (GS232B_ERROR_CLOSED);

	cssl_close(gs232b_serial);
	return 0;
}

/**
* Routine to use in blocking mode. Reads the serial port until a New
* Line is received, then returns entire buffer.
* @param buf Pointer to a char buffer
* @param buflen ::int32_t indicating the maximum size of the buffer
* @return ::int32_t containing the number of bytes read.
*/
int32_t gs232b_getdata(char *buf, int32_t buflen)
{
	int32_t i,j;

	i = 0;
	while ((j=cssl_getdata(gs232b_serial,(uint8_t *)&buf[i],buflen-i)))
	{
		if (j < 0)
		{
			return j;
		}
		else
		{
			i += j;
			if (buf[i-1] == '\n')
				break;
		}
	}
	buf[i] = 0;
	return (i);
}

/**
* Routine to help calibrate rotor offset on specified axis. Turns
* calibration mode on in GS-232B.
* @param axis ::int32_t , 0 = Azimuth, 1 = Elevation
*/
int32_t gs232b_offset_wait(int32_t axis)
{
	int32_t iretn;
	switch (axis)
	{
	case 0:
		iretn = gs232b_send((char *)"O\r", false);
		break;
	case 1:
		iretn = gs232b_send((char *)"O2\r", false);
		break;
	default:
		iretn = GS232B_ERROR_OUTOFRANGE;
		break;
	}
	return iretn;
}

/**
* Accept rotor offset calibration for currently calibrating axis.
*/
int32_t gs232b_offset_accept()
{
	int32_t iretn;
	iretn = cssl_putchar(gs232b_serial,'y');
	return iretn;
}

//void gs232b_full_scale_calibration(int32_t axis)
//{
//}

//void gs232b_rotate(int32_t axis, int32_t direction)
//{
//}

int32_t gs232b_az_speed(int32_t speed)
{
	int32_t iretn;
	char out[50];

	if (speed < 1 || speed > 4)
		return (GS232B_ERROR_OUTOFRANGE);
	sprintf(out,"X%1d\r",speed);
	iretn = gs232b_send(out, false);
	if (iretn < 0)
	{
		return iretn;
	}
	else
	{
		return (speed);
	}
}

int32_t gs232b_goto(float az, float el)
{
	int32_t iretn;
	char out[50];
	float mel, daz, del, sep;
	static int32_t taz = 500;
	static int32_t tel = 500;
	int32_t iaz, iel;

	if (az < 0 || az > RADOF(450) || el < 0 || el > DPI)
		return (GS232B_ERROR_OUTOFRANGE);

	iretn = gs232b_get_az_el(gs_state.currentaz,gs_state.currentel);
	if (iretn < 0)
	{
		return iretn;
	}

	if (az < DPI2 && az < gs_state.currentaz)
	{
		az += D2PI;
	}
	else
		if (az > D2PI && az > gs_state.currentaz)
		{
			az -= D2PI;
		}
	
	mel = .001+(el+gs_state.currentel)/2.;
	del = el-gs_state.currentel;
	daz = (az-gs_state.currentaz)/cos(mel);
	sep = sqrt(daz*daz+del*del);
//	printf("az: %7.2f-%7.2f el: %7.2f-%7.2f sep: %8.3f \n",DEGOF(gs_state.currentaz),DEGOF(az),DEGOF(gs_state.currentel),DEGOF(el),DEGOF(sep));
	if (sep > RADOF(2.))
	{
		switch ((int)(4.5*sep/DPI))
		{
		case 0:
			break;
		case 1:
			gs232b_send((char *)"X1\r", false);
			break;
		case 2:
			gs232b_send((char *)"X2\r", false);
			break;
		case 3:
			gs232b_send((char *)"X3\r", false);
			break;
		case 4:
		default:
			gs232b_send((char *)"X4\r", false);
			break;
		}
		iaz = (int)(DEGOF(az)+.5);
		iel = (int)(DEGOF(el)+.5);
		if (iaz != taz || iel != tel)
		{
			sprintf(out,"W%03d %03d\r",iaz,iel);
			gs232b_send(out, false);
			taz = iaz;
			tel = iel;
		}
	}
	return 0;
}

/**
* Routine to stop current action. Whatever the current command is, it will
* cancelled before completeion.
* @return 0 or negative error.
*/
int32_t gs232b_stop()
{
	int32_t iretn;

	iretn = gs232b_send((char *)"S\r", false);

	return iretn;
}

float gs232b_get_az()
{
	char buf[20];
	gs232b_send((char *)"C\r",1);
	gs232b_getdata(buf,20);
	sscanf(buf,"AZ=%03f\r\n",&gs_state.currentaz);
	gs_state.currentaz = RADOF(gs_state.currentaz);
	return (gs_state.currentaz);
}

float gs232b_get_el()
{
	char buf[20];
	gs232b_send((char *)"B\r",1);
	gs232b_getdata(buf,20);
	sscanf(buf,"EL=%03f\r\n",&gs_state.currentel);
	gs_state.currentel = RADOF(gs_state.currentel);
	return (gs_state.currentel);
}

int32_t gs232b_get_az_el(float &az, float &el)
{
	int32_t iretn = 0;
	char buf[40];

	iretn = gs232b_send((char *)"C2\r",1);
	iretn = gs232b_getdata(buf,40);
	if (iretn == 0)
	{
		iretn = gs232b_getdata(buf,40);
	}
	if (iretn)
	{
		sscanf(buf,"AZ=%03f  EL=%03f\r\n",&az,&el);
		az = RADOF(az);
		el = RADOF(el);
	}
	return iretn;
}

float gs232b_get_az_offset()
{
	return (gs_state.az_offset);
}

float gs232b_get_el_offset()
{
	return (gs_state.el_offset);
}

void gs232b_get_state(gs232b_state &state)
{
	state = gs_state;
}

int32_t gs232b_send(char *buf, bool force)
{
	int32_t iretn = 0;
	static char lastbuf[256];

	if (strcmp(lastbuf,buf) || force)
	{
//		printf("%s\n",buf);
		iretn = cssl_putstring(gs232b_serial,buf);
		strncpy(lastbuf,buf,256);
	}

	return iretn;
}

//#endif // define(COSMOS_MAC_OS) || define(COSMOS_LINUX_OS)
