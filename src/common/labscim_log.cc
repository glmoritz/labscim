/*
 * labscim_log.c
 *
 *  Created on: 16 de jul de 2020
 *      Author: root
 */

#include <cassert>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

static FILE* gLogger=nullptr;
static FILE* gOutLogger=nullptr;
static double gLastLogTime;
static uint64_t gNodeID;

void Node_Log(double time, uint64_t NodeID, uint8_t* text)
{
#ifdef LABSCIM_LOG_COMMANDS
    if(gLogger==nullptr)
    {
        gLogger = fopen("central_log.txt", "w+");
    }

    if((time>gLastLogTime)||(gNodeID!=NodeID))
    {
        fprintf(gLogger,"%5.5f (+%2.5f)\t Node %3ld\n",time,(time-gLastLogTime), NodeID);
        fflush(gLogger);
        gNodeID = NodeID;
        gLastLogTime = time;
    }
    fprintf(gLogger,"\t\t\t%s", text);
#endif
}

void Node_Log_Output(double time, uint64_t NodeID, uint8_t* text)
{
#ifdef LABSCIM_LOG_OUTPUT
    if(gOutLogger==nullptr)
    {
        gOutLogger = fopen("node_output_log.txt", "w+");
    }
    fprintf(gOutLogger,"%5.5f\t%5ld\t",time, NodeID);
    fprintf(gOutLogger,"%s", text);
    fflush(gOutLogger);
#endif
}

