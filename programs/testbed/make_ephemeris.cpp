#include "jsonlib.h"
#include "physicslib.h"
#include "mathlib.h"
#include "jsonlib.h"
#include "agentlib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/types.h>
#include <sys/stat.h>

vector<nodestruc> track;
cosmosstruc *cdata;
gj_handle gjh;
char buf[3000], fname[200];
svector azel;

int main(int argc, char *argv[])
{
	double fyear;
	beatstruc imubeat;
	double mjdnow, lastlat, lastsunradiance, fd, lastgsel[MAXTRACK];
	string output;
	char date[30];
	FILE *eout, *fout;
	int i, j, iretn, gsup[MAXTRACK];
	int year, month, day, hour, minute, second;
	struct stat sbuf;
	string jstring;

	cdata = json_create();
	node_init(argv[1],cdata);
	json_clone(cdata);
	iretn=agent_get_server(cdata, cdata->node.name,(char *)"simulator",5,&imubeat);
	agent_send_request(cdata, imubeat,(char *)"statevec",buf,1000,5);
	json_parse(buf,cdata);
	agent_send_request(cdata, imubeat,(char *)"value node_name",buf,1000,5);
	json_parse(buf,cdata);

	mjdnow = cdata->node.loc.utc;
	fyear = mjd2year(mjdnow);
	year = (int)fyear;
	day = (int)(365.26 * (fyear-year)) + 1;
	sprintf(fname,"data/%4d/%03d",year,day);
#if defined(COSMOS_WIN_OS)
	mkdir(fname);
#else
	mkdir(fname,00775);
#endif

	sprintf(fname,"data/%4d/%03d/orbitalevents",year,day);
	if (stat(fname,&sbuf) == -1)
	{
		// Move old orbitalevents to directory
		rename("orbitalevents",fname);
	}


	gauss_jackson_init_eci(gjh, 4, 0, 5., mjdnow, cdata->node.loc.pos.eci, cdata->node.loc.att.icrf, *cdata);
	mjdnow = cdata->node.loc.utc;
	lastlat = cdata->node.loc.pos.geod.s.lat;
	for (j=0; j<3; j++)
	{
		lastgsel[j] = azel.phi;
		gsup[j] = 0;
	}
	lastsunradiance = cdata->node.loc.pos.sunradiance;
	fout = fopen("ephemeris","w");
	eout = fopen("orbitalevents","w");
	for (i=0; i<8640; i++)
	{
		mjdnow += 10./86400.;
		gauss_jackson_propagate(gjh, *cdata, mjdnow);
		output = json_of_ephemeris(jstring,  cdata);
		fprintf(fout,"%s\n",output.c_str());
		fflush(fout);
		mjd2cal(mjdnow,&year,&month,&day,&fd,&iretn);
		hour = (int)(24. * fd);
		minute = (int)(1440. * fd - hour * 60.);
		second = (int)(86400. * fd - hour * 3600. - minute * 60.);
		sprintf(date,"%4d-%02d-%02d,%02d:%02d:%02d,O00000,",year,month,day,hour,minute,second);
		if (lastlat <= RADOF(-60.) && cdata->node.loc.pos.geod.s.lat >= RADOF(-60.))
			fprintf(eout,"%sS60A\n",date);
		if (lastlat <= RADOF(-30.) && cdata->node.loc.pos.geod.s.lat >= RADOF(-30.))
			fprintf(eout,"%sS30A\n",date);
		if (lastlat <= 0. && cdata->node.loc.pos.geod.s.lat >= 0.)
			fprintf(eout,"%sEQA\n",date);
		if (lastlat <= RADOF(30.) && cdata->node.loc.pos.geod.s.lat >= RADOF(30.))
			fprintf(eout,"%sN30A\n",date);
		if (lastlat <= RADOF(60.) && cdata->node.loc.pos.geod.s.lat >= RADOF(60.))
			fprintf(eout,"%sN60A\n",date);
		if (lastlat >= RADOF(-60.) && cdata->node.loc.pos.geod.s.lat <= RADOF(-60.))
			fprintf(eout,"%sS60D\n",date);
		if (lastlat >= RADOF(-30.) && cdata->node.loc.pos.geod.s.lat <= RADOF(-30.))
			fprintf(eout,"%sS30D\n",date);
		if (lastlat >= 0. && cdata->node.loc.pos.geod.s.lat <= 0.)
			fprintf(eout,"%sEQD\n",date);
		if (lastlat >= RADOF(30.) && cdata->node.loc.pos.geod.s.lat <= RADOF(30.))
			fprintf(eout,"%sN30D\n",date);
		if (lastlat >= RADOF(60.) && cdata->node.loc.pos.geod.s.lat <= RADOF(60.))
			fprintf(eout,"%sN60D\n",date);

		if (lastsunradiance == 0. && cdata->node.loc.pos.sunradiance > 0.)
		{
			date[20] = JSON_TYPE_UINT32;
			fprintf(eout,"%sUMB_OUT\n",date);
		}
		if (lastsunradiance > 0. && cdata->node.loc.pos.sunradiance == 0.)
		{
			date[20] = JSON_TYPE_UINT32;
			fprintf(eout,"%sUMB_IN\n",date);
		}
		lastlat = cdata->node.loc.pos.geod.s.lat;
		lastsunradiance = cdata->node.loc.pos.sunradiance;
		for (j=0; j<3; j++)
		{
			if (azel.phi <= lastgsel[j] && azel.phi > 0.)
			{
				if (gsup[j] > 0.)
				{
					date[20] = 'G';
					fprintf(eout,"%sMAX_%3s_%03.0f\n",date,track[j].name,DEGOF(azel.phi));
				}
				gsup[j] = -1;
			}
			else
				gsup[j] = 1;
			if (azel.phi < RADOF(10.) && lastgsel[j] >= RADOF(10.))
			{
				date[20] = 'G';
				fprintf(eout,"%sLOS+10_%3s\n",date,track[j].name);
			}
			if (azel.phi < RADOF(5.) && lastgsel[j] >= RADOF(5.))
			{
				date[20] = 'G';
				fprintf(eout,"%sLOS+5_%3s\n",date,track[j].name);
			}
			if (azel.phi < 0. && lastgsel[j] >= 0.)
			{
				date[20] = 'G';
				fprintf(eout,"%sLOS_%3s\n",date,track[j].name);
			}
			if (azel.phi >= 0. && lastgsel[j] < 0.)
			{
				date[20] = 'G';
				fprintf(eout,"%sAOS_%3s\n",date,track[j].name);
			}
			if (azel.phi >= RADOF(5.) && lastgsel[j] < RADOF(5.))
			{
				date[20] = 'G';
				fprintf(eout,"%sAOS+5_%3s\n",date,track[j].name);
			}
			if (azel.phi >= RADOF(10.) && lastgsel[j] < RADOF(10.))
			{
				date[20] = 'G';
				fprintf(eout,"%sAOS+10_%3s\n",date,track[j].name);
			}
			lastgsel[j] = azel.phi;
		}
	}
	fclose(fout);
	fclose(eout);
}
