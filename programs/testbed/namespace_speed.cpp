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
#include "agentlib.h"
#include "jsonlib.h"
#include "datalib.h"
#include "elapsedtime.hpp"

int main(int argc, char *argv[])
{
	cosmosstruc *cdata;

	switch (argc)
	{
	case 3:
		setEnvCosmosNodes(argv[2]);
	case 2:
		cdata = agent_setup_client(NetworkType::UDP, argv[1]);
		break;
	default:
		printf("Usage: namespace_speed node [node_directory]\n");
		exit (1);
		break;
	}

	std::vector <std::string> names;
    std::map <std::string,jsonentry*> testmap;
	for (uint32_t i = 0; i < cdata[0].jmap.size(); i++)
	{
		for (uint32_t j = 0; j < cdata[0].jmap[i].size(); j++)
		{
			names.push_back(cdata[0].jmap[i][j].name);
			testmap[cdata[0].jmap[i][j].name] = json_entry_of(cdata[0].jmap[i][j].name, cdata);
		}
	}

	ElapsedTime et;

	for (uint16_t i=0; i<3; ++i)
	{
		et.start();

		for (std::string name: names)
		{
			jsonentry *tentry = json_entry_of(name, cdata);
			std::string tname = tentry->name;
		}
		printf("Standard Lookup: %d names, %f seconds\n", names.size(), et.lap());

		et.reset();

		for (std::string name: names)
		{
			jsonentry *tentry = testmap[name];
			std::string tname = tentry->name;
		}
		printf("Map Lookup: %d names, %f seconds\n", names.size(), et.lap());
	}

}


