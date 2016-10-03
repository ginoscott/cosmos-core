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

#include "agent/agentclass.h"
#include "jsonlib.h"
#include "convertlib.h"
#include "datalib.h"
#include "command_queue.h"

#include <iostream>
#include <iomanip>

/*! \file agent_exec.cpp
* \brief Executive Agent source file
*/

//! \ingroup agents
//! \defgroup agent_exec Executive Agent program
//! This Agent manages commanding within the COSMOS system.
//! A single command queue is kept containing both time, and time and condition driven
//! commands. Commands can be added or removed from this queue, either through direct requests
//! or through command files.
//!
//! Commands are represented as a ::eventstruc. If EVENT_FLAG_CONDITIONAL is set, the condition
//! part of the ::eventstruc is evaluated as a COSMOS equation to determine whether the command
//! should be executed. Either way, commands are only executed if their time has passed. Once a
//! command has executed, it is either remove from the queue, or if EVENT_FLAG_REPEAT is set,
//! it is disabled from executing until such time as the condition goes false again, after which
//! it can once again execute.
//!
//! Any execution of a command is reflected in two log files, one of which tracks the results of the
//! command, and the other of which logs the actual ::eventstruc for the command, with the utcexec field
//! set to the actual time of execution.
//!
//! Usage: agent_exec node_name

#include <list>
#include <fstream>
#include <sstream>

//Agent *agent;
Agent *agent;

// Executive specfic declarations
//#include "agent_exec.h"


std::string incoming_dir;
std::string outgoing_dir;
std::string temp_dir;

std::string nodename;

double logdate_exec=0.;
double newlogstride_exec = 900. / 86400.;
double logstride_exec = 0.;

int32_t request_get_queue_size(char *request, char* response, Agent *);
int32_t request_get_queue_entry(char *request, char* response, Agent *);
int32_t request_del_queue_entry(char *request, char* response, Agent *);
int32_t request_add_queue_entry(char *request, char* response, Agent *);
int32_t request_run(char *request, char* response, Agent *);
int32_t request_soh(char *request, char* response, Agent *);
int32_t request_reopen_exec(char* request, char* output, Agent *agent);
int32_t request_set_logstride_exec(char* request, char* output, Agent *agent);

command_queue cmd_queue;

// SOH specific declarations
int32_t request_reopen_soh(char* request, char* output, Agent *agent);
int32_t request_set_logperiod(char* request, char* output, Agent *agent);
int32_t request_set_logstring(char* request, char* output, Agent *agent);
int32_t request_get_logstring(char* request, char* output, Agent *agent);
int32_t request_set_logstride_soh(char* request, char* output, Agent *agent);

std::string jjstring;
std::string myjstring;

NetworkType ntype = NetworkType::UDP;
int waitsec = 5;

void collect_data_loop();
std::thread cdthread;

int myagent();

std::string logstring;
std::vector<jsonentry*> logtable;
double logdate_soh=0.;
int32_t newlogperiod = 10, logperiod = 0;
double newlogstride_soh = 900. / 86400.;
double logstride_soh = 0.;

std::vector<shorteventstruc> eventdict;
std::vector<shorteventstruc> events;

int pid;
int state = 0;
double cmjd;

beatstruc iscbeat;
std::string node = "hiakasat";
char response[300];

int main(int argc, char *argv[])
{
	int sleept;
	double lmjd, dmjd;
	double nextmjd;

    std::cout<<"Starting the executive/soh agent->..";
	int32_t iretn;
    NetworkType ntype = NetworkType::UDP;

	// Set node name to first argument
	if (argc!=2)
	{
		std::cout<<"Usage: agent_exec_soh node"<<std::endl;
		exit(1);
	}
	nodename = argv[1];

	// Establish the command channel and heartbeat
    agent = new Agent(ntype, nodename, "execsoh");
    agent->cinfo->pdata.node.utc = 0.;
    agent->cinfo->pdata.agent[0].aprd = .5;

	std::cout<<"  started."<<std::endl;

	// Establish Executive functions

	// Set the incoming, outgoing, and temp directories
	incoming_dir = data_base_path(nodename, "incoming", "exec") + "/";
	if (incoming_dir.empty())
	{
		std::cout<<"unable to create directory: <"<<(nodename+"/incoming")+"/exec"<<"> ... exiting."<<std::endl;
		exit(1);
	}
	outgoing_dir = data_base_path(nodename, "outgoing", "exec") + "/";
	if (outgoing_dir.empty())
	{
		std::cout<<"unable to create directory: <"<<(nodename+"/outgoing")+"/exec"<<"> ... exiting."<<std::endl;
		exit(1);
	}
	outgoing_dir = data_base_path(nodename, "outgoing", "exec") + "/";
	if (outgoing_dir.empty())
	{
		std::cout<<"unable to create directory: <"<<(nodename+"/outgoing")+"/exec"<<"> ... exiting."<<std::endl;
		exit(1);
	}
	temp_dir = data_base_path(nodename, "temp", "exec") + "/";
	if (temp_dir.empty())
	{
		std::cout<<"unable to create directory: <"<<(nodename+"/temp")+"/exec"<<"> ... exiting."<<std::endl;
		exit(1);
	}

	// Add agent request functions
    if ((iretn=agent->add_request("get_queue_size", request_get_queue_size, "", "returns the current size of the command queue")))
		exit (iretn);
    if ((iretn=agent->add_request("del_queue_entry", request_del_queue_entry, "entry #", "deletes the specified command queue entry")))
		exit (iretn);
    if ((iretn=agent->add_request("get_queue_entry", request_get_queue_entry, "[ entry # ]", "returns the requested command queue entry (or all if none specified)")))
		exit (iretn);
    if ((iretn=agent->add_request("add_queue_entry", request_add_queue_entry, "{\"event_name\":\"\"}{\"event_utc\":0}{\"event_utcexec\":0}{\"event_flag\":0}{\"event_type\":0}{\"event_data\":\"\"}{\"event_condition\":\"\"}", "adds the specified command queue entry")))
		exit (iretn);
    if ((iretn=agent->add_request("run", request_run, "", "run the requested command")))
		exit (iretn);
    if ((iretn=agent->add_request("reopen_exec", request_reopen_exec)))
		exit (iretn);
    if ((iretn=agent->add_request("set_logstride_exec", request_set_logstride_exec)))
		exit (iretn);

	// Reload existing queue
	std::string infilepath = temp_dir + ".queue";
    std::ifstream infile(infilepath.c_str());
	if(!infile.is_open())
	{
		std::cout<<"unable to read file <"<<infilepath<<">"<<std::endl;
	}
	else
	{

		//file is open for reading commands
		std::string line;
        Command cmd;

        while(std::getline(infile,line))
		{
            cmd.set_command(line, agent);

			std::cout<<cmd;

			if(cmd.is_command())
			{
				cmd_queue.add_command(cmd);
				printf("Loaded command: %s\n", line.c_str());
			}
			else
			{
				std::cout<<"Not a command!"<<std::endl;
			}
		}
		infile.close();
	}

	// Establish SOH functions

    if ((iretn=agent->add_request("reopen_soh", request_reopen_soh)))
		exit (iretn);
    if ((iretn=agent->add_request("set_logperiod" ,request_set_logperiod)))
		exit (iretn);
    if ((iretn=agent->add_request("set_logstring", request_set_logstring)))
		exit (iretn);
    if ((iretn=agent->add_request("get_logstring", request_get_logstring)))
		exit (iretn);
    if ((iretn=agent->add_request("set_logstride_soh", request_set_logstride_soh)))
		exit (iretn);

	// Create default logstring
    logstring = json_list_of_soh(agent->cinfo->pdata);
	printf("logstring: %s\n", logstring.c_str());
    json_table_of_list(logtable, logstring.c_str(), agent->cinfo->meta);
    //	agent_set_sohstring(agent->cinfo, logstring.c_str());

    load_dictionary(eventdict, agent->cinfo->meta, agent->cinfo->pdata, (const char *)"events.dict");

	// Start thread to collect SOH data
	cdthread = std::thread(collect_data_loop);

	// Start performing the body of the agent
	nextmjd = currentmjd();
	lmjd = currentmjd();
    while(agent->running())
	{
        nextmjd += agent->cinfo->pdata.agent[0].aprd/86400.;
		dmjd = (cmjd-lmjd)*86400.;
        agent->cinfo->pdata.node.utc = cmjd = currentmjd();

		// Check if the SOH logperiod has changed
		if (newlogperiod != logperiod )
		{
			logperiod = newlogperiod;
            logdate_soh = agent->cinfo->pdata.node.utc;
            log_move(agent->cinfo->pdata.node.name, "soh");
		}


		// Check if either of the logstride have changed
		if (newlogstride_exec != logstride_exec )
		{
			logstride_exec = newlogstride_exec;
			logdate_exec = currentmjd(0.);
			log_move(nodename, "exec");
		}

		if (newlogstride_soh != logstride_soh )
		{
			logstride_soh = newlogstride_soh;
			logdate_soh = currentmjd(0.);
			log_move(nodename, "soh");
		}

		// Check if either of the logstride have expired
		if (floor(cmjd/logstride_exec)*logstride_exec > logdate_exec)
		{
			logdate_exec = floor(cmjd/logstride_exec)*logstride_exec;
			log_move(nodename, "exec");
		}

		if (floor(cmjd/logstride_soh)*logstride_soh > logdate_soh)
		{
			logdate_soh = floor(cmjd/logstride_soh)*logstride_soh;
			log_move(nodename, "soh");
		}

		// Perform SOH specific functions
        if (agent->cinfo->pdata.node.utc != 0.)
		{
            loc_update(&agent->cinfo->pdata.node.loc);
            update_target(agent->cinfo->pdata);
            agent->post(Agent::AGENT_MESSAGE_SOH, json_of_table(myjstring, logtable, agent->cinfo->meta, agent->cinfo->pdata));
            calc_events(eventdict, agent->cinfo->meta, agent->cinfo->pdata, events);
			for (uint32_t k=0; k<events.size(); ++k)
			{
                memcpy(&agent->cinfo->pdata.event[0].s,&events[k],sizeof(shorteventstruc));
                strcpy(agent->cinfo->pdata.event[0].l.condition,agent->cinfo->meta.emap[events[k].handle.hash][events[k].handle.index].text);
                log_write(agent->cinfo->pdata.node.name,DATA_LOG_TYPE_EVENT,logdate_soh, json_of_event(jjstring, agent->cinfo->meta, agent->cinfo->pdata));
			}
		}

		// Check if SOH logperiod has expired
		if (dmjd-logperiod > -logperiod/20.)
		{
			lmjd = cmjd;
            if (agent->cinfo->pdata.node.utc != 0. && logstring.size())
			{
                log_write(agent->cinfo->pdata.node.name, DATA_LOG_TYPE_SOH, logdate_soh, json_of_table(jjstring, logtable, agent->cinfo->meta, agent->cinfo->pdata));
			}
		}

		// Perform Executive specific functions
        cmd_queue.load_commands(incoming_dir, agent);
        cmd_queue.run_commands(agent, nodename, logdate_exec);
        cmd_queue.save_commands(temp_dir);

		sleept = (int)((nextmjd-currentmjd())*86400000000.);
		if (sleept < 0) sleept = 0;
		COSMOS_USLEEP(sleept);
	}

    agent->shutdown();
	cdthread.join();
}

// Executive specific requests
int32_t request_set_logstride_exec(char* request, char* , Agent *)
{
	sscanf(request,"set_logstride_exec %lf",&newlogstride_exec);
	return 0;
}

int32_t request_reopen_exec(char* , char* , Agent *agent)
{
    logdate_exec = ((cosmosstruc *)agent->cinfo)->pdata.node.loc.utc;
    log_move(((cosmosstruc *)agent->cinfo)->pdata.node.name, "exec");
	return 0;
}

int32_t request_get_queue_size(char *, char* response, Agent *)
{
	sprintf(response,"%" PRIu32 "", cmd_queue.get_size());
	return 0;
}

int32_t request_get_queue_entry(char *request, char* response, Agent *)
{
    std::ostringstream ss;

	if(cmd_queue.get_size()==0)
		ss << "the command queue is empty";
	else
	{
		int j;
		int32_t iretn = sscanf(request,"get_queue_entry %d",&j);

		// if valid index then return command
		if (iretn == 1)
			if(j >= 0 && j < (int)cmd_queue.get_size() )
				ss << cmd_queue.get_command(j);
			else
				ss << "<" << j << "> is not a valid command queue index (current range between 0 and " << cmd_queue.get_size()-1 << ")";

		// if no index given, return the entire queue
		else if (iretn ==  -1)
			for(unsigned long int i = 0; i < cmd_queue.get_size(); ++i)
				ss << cmd_queue.get_command(i) << std::endl;

		// if the user supplied something that couldn't be turned into an integer
		else if (iretn == 0)
			ss << "Usage:\tget_queue_entry [ index ]\t";
	}

	strcpy(response, ss.str().c_str());
	return 0;
}

// Delete Queue Entry - by date and contents
int32_t request_del_queue_entry(char *request, char* response, Agent *)
{
    Command cmd;
	std::string line(request);

	// remove "del_queue_entry " from request string
	line.erase(0, 16);

    cmd.set_command(line, agent);

	//delete command
	int n = cmd_queue.del_command(cmd);

	sprintf(response,"%d commands deleted from the queue",n);

	return 0;
}

// Add Queue Entry
int32_t request_add_queue_entry(char *request, char* response, Agent *)
{
    Command cmd;
	std::string line(request);

	// remove "add_queue_entry " from request string
	line.erase(0, 16);

    cmd.set_command(line, agent);

	// add command
	if(cmd.is_command())
		cmd_queue.add_command(cmd);

	strcpy(response, line.c_str());
	return 0;
}

int32_t request_run(char *request, char* response, Agent *)
{
	int i;
	int32_t iretn = 0;
	FILE *pd;
	bool flag;

	// Run Program
	flag = false;

	for (i=0; i<AGENTMAXBUFFER-1; i++)
	{
		if (flag)
		{
			if (request[i] != ' ')
				break;
		}
		else
		{
			if (request[i] == ' ')
				flag = true;
		}
	}

	if (i == AGENTMAXBUFFER-1)
	{
		sprintf(response,"unmatched");
	}
	else
	{
#ifdef COSMOS_WIN_BUILD_MSVC
		if ((pd=_popen(&request[i], "r")) != NULL)
#else
		if ((pd=popen(&request[i],"r")) != NULL)
#endif
		{
			iretn = fread(response,1,AGENTMAXBUFFER-1,pd);
			response[iretn] = 0;

			iretn = 0;
#ifdef COSMOS_WIN_BUILD_MSVC
			_pclose(pd);
#else
			pclose(pd); // close process
#endif
		}
		else
		{
			response[0] = 0;
			iretn = 0;
		}
	}

	return (iretn);
}

// SOH specific requests
int32_t request_reopen_soh(char* , char* , Agent *)
{
    logdate_soh = ((cosmosstruc *)agent->cinfo)->pdata.node.loc.utc;
    log_move(((cosmosstruc *)agent->cinfo)->pdata.node.name, "soh");
	return 0;
}

int32_t request_set_logperiod(char* request, char* , Agent *)
{
	sscanf(request,"set_logperiod %d",&newlogperiod);
	return 0;
}

int32_t request_set_logstring(char* request, char* , Agent *)
{
	logstring = &request[strlen("set_logstring")+1];
    json_table_of_list(logtable, logstring.c_str(), agent->cinfo->meta);
	return 0;
}

int32_t request_get_logstring(char* , char* output, Agent *)
{
	strcpy(output, logstring.c_str());
	return 0;
}

int32_t request_set_logstride_soh(char* request, char* , Agent *)
{
	sscanf(request,"set_logstride_soh %lf",&newlogstride_soh);
	return 0;
}

void collect_data_loop()
{
    int32_t my_position = -1;
    while (agent->running())
	{
		// Collect new data
        while (my_position != agent->message_head)
        {
            ++my_position;
            if (my_position >= agent->message_ring.size())
            {
                my_position = 0;
            }
            if (agent->cinfo->pdata.node.name == agent->message_ring[my_position].meta.beat.node && agent->message_ring[my_position].meta.type < Agent::AGENT_MESSAGE_BINARY)
            {
                agent->cinfo->sdata.node = agent->cinfo->pdata.node;
                agent->cinfo->sdata.device = agent->cinfo->pdata.device;
                json_parse(agent->message_ring[my_position].adata, agent->cinfo->meta, agent->cinfo->sdata);
                agent->cinfo->pdata.node  = agent->cinfo->sdata.node ;
                agent->cinfo->pdata.device  = agent->cinfo->sdata.device ;
                loc_update(&agent->cinfo->pdata.node.loc);
                agent->cinfo->pdata.node.utc = currentmjd(0.);
            }
        }
	}
	return;
}

// Prints the command information stored in local the copy of agent->cinfo->pdata.event[0].l
void print_command()
{
	std::string jsp;

    json_out(jsp,(char*)"event_utc", agent->cinfo->meta, agent->cinfo->pdata);
    json_out(jsp,(char*)"event_utcexec", agent->cinfo->meta, agent->cinfo->pdata);
    json_out(jsp,(char*)"event_name", agent->cinfo->meta, agent->cinfo->pdata);
    json_out(jsp,(char*)"event_type", agent->cinfo->meta, agent->cinfo->pdata);
    json_out(jsp,(char*)"event_flag", agent->cinfo->meta, agent->cinfo->pdata);
    json_out(jsp,(char*)"event_data", agent->cinfo->meta, agent->cinfo->pdata);
    json_out(jsp,(char*)"event_condition", agent->cinfo->meta, agent->cinfo->pdata);
	std::cout<<"<"<<jsp<<">"<<std::endl;

	return;
}







