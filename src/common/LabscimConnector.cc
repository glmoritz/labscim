/*
 * LabscimConnector.cc
 *
 *  Created on: 26 de mai de 2020
 *      Author: guilherme
 */

#include "LabscimConnector.h"
#include <string>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <sys/time.h>

using namespace labscim;
using namespace std;

std::string LabscimConnector::ExecuteCommand(const char* cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    try
    {
        while (fgets(buffer, sizeof buffer, pipe) != NULL)
        {
            result += buffer;
        }
    }
    catch (...)
    {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}



LabscimConnector::LabscimConnector()
{
    mBufferSize = 0;
    labscim_ll_init_list(&mCommands);
}


LabscimConnector::~LabscimConnector()
{
    labscim_ll_deinit_list(&mCommands);

    if(mNodeOutputBuffer!=mNodeInputBuffer)
    {
        labscim_buffer_deinit(mNodeInputBuffer, 1);
        delete mNodeInputBuffer;
    }
    labscim_buffer_deinit(mNodeOutputBuffer, 0);
    labscim_socket_disconnect(mNodeOutputBuffer);
    delete mNodeOutputBuffer;
}

int32_t LabscimConnector::SpawnProcess(std::string processcommand, std::string node_name, uint32_t serverport, uint32_t BufferSize)
{
#ifndef LABSCIM_REMOTE_SOCKET //shared memory communication
    struct labscim_protocol_header* cmd;
#endif

    mNodeOutputBuffer = new buffer_circ_t;
    if(mNodeOutputBuffer == NULL)
    {
        return -1;
    }
    labscim_buffer_init(mNodeOutputBuffer, (char*)(std::string("/") + node_name + std::string("out")).c_str(), BufferSize, 1);
    if(!processcommand.empty())
    {
        std::string result = ExecuteCommand(processcommand.c_str());
    }
    //now wait for the incoming connection
    mBufferSize = BufferSize;
#ifdef LABSCIM_REMOTE_SOCKET //socket communication, local buffer
    labscim_socket_connect(serverport, mNodeOutputBuffer);
    mPort = serverport;
    //only one buffer is needed
    mNodeInputBuffer = mNodeOutputBuffer;
#else
    //shared memory communication
    pthread_mutex_lock(mNodeOutputBuffer->mutex.mutex);
    labscim_socket_handle_input(mNodeOutputBuffer, &mCommands);
    while(mCommands.count == 0)
    {
        pthread_cond_wait(mNodeOutputBuffer->mutex.more, mNodeOutputBuffer->mutex.mutex);
        labscim_socket_handle_input(mNodeOutputBuffer, &mCommands);
    }
    pthread_cond_signal(mNodeOutputBuffer->mutex.less);
    pthread_mutex_unlock(mNodeOutputBuffer->mutex.mutex);
    cmd = (labscim_protocol_header*)labscim_ll_pop_front(&mCommands);
    if(cmd->labscim_protocol_code != LABSCIM_NODE_IS_READY)
    {
        free(cmd);
        return -1;
    }
    else
    {
        //maps the buffer from the node
        mNodeInputBuffer = new buffer_circ_t;
        if(mNodeInputBuffer == NULL)
        {
            return -1;
        }
        labscim_buffer_init(mNodeInputBuffer, (char*)(std::string("/") + node_name + std::string("in")).c_str(), BufferSize, 0);
    }
#endif
    return 0;
}

int32_t LabscimConnector::WaitForCommand()
{
#ifdef LABSCIM_REMOTE_SOCKET //socket communication, local buffer
    bool KeepWaiting = true;
    while(KeepWaiting && mCommands.count==0)
    {
        fd_set fdr;
        fd_set fdw;
        int retval;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        FD_ZERO(&fdr);
        FD_ZERO(&fdw);
        FD_SET(mClientSocket, &fdr);
        retval = select(mClientSocket + 1, &fdr, &fdw, NULL, &tv);
        if(retval < 0)
        {
            if(errno != EINTR)
            {
                throw std::runtime_error("select() failed!");
            }
        }
        else if(retval > 0)
        {
            /* timeout => retval == 0 */
            if(FD_ISSET(mClientSocket, &fdr))
            {
                labscim_socket_handle_input(gNodeOutputBuffer,&mBufferStruct,&mCommands);
                KeepWaiting = false;
            }
        }
        if(Attemps++>5)
        {
            //500ms timeout
            KeepWaiting = false;
        }
    }
#else
    //shared memory communication
    pthread_mutex_lock(mNodeOutputBuffer->mutex.mutex);
    uint32_t timeout = 0;
    do{
        labscim_socket_handle_input(mNodeOutputBuffer, &mCommands);
        if(mCommands.count == 0)
        {
            int               rc;
            struct timespec   ts;
            struct timeval    tp;
            rc =  gettimeofday(&tp, NULL);
            /* Convert from timeval to timespec */
            ts.tv_sec  = tp.tv_sec;
            ts.tv_nsec = (tp.tv_usec+5*100000) * 1000;
            if(ts.tv_nsec > 1000000000)
            {
                ts.tv_nsec -= 1000000000;
                ts.tv_sec++;
            }
            rc = pthread_cond_timedwait(mNodeOutputBuffer->mutex.more, mNodeOutputBuffer->mutex.mutex, &ts);
            if (rc == 0)
            {
                labscim_socket_handle_input(mNodeOutputBuffer, &mCommands);
            }
            else if(rc == ETIMEDOUT)
            {
                timeout = 1;
            }
        }
    }while((mCommands.count == 0)&&(!timeout));
    pthread_cond_signal(mNodeOutputBuffer->mutex.less);
    pthread_mutex_unlock(mNodeOutputBuffer->mutex.mutex);
#endif
    return mCommands.count;
}


void LabscimConnector::SendProtocolBoot(void* ConfigMessage,size_t ConfigMessageSize)
{
    protocol_boot(mNodeInputBuffer, ConfigMessage, ConfigMessageSize);
}

void LabscimConnector::SendTimeEvent(uint32_t SequenceNumber, uint32_t TimeEventID, uint64_t CurrentTime_us)
{

    time_event(mNodeInputBuffer, SequenceNumber, TimeEventID, CurrentTime_us);
}

void LabscimConnector::SendRadioResponse(uint16_t RadioResponse, uint64_t CurrentTime, void* RadioStruct, size_t RadioStructLen, uint32_t SequenceNumber)
{
    radio_response(mNodeInputBuffer, RadioResponse, CurrentTime, RadioStruct, RadioStructLen, SequenceNumber);
}


void LabscimConnector::SendRegisterResponse(uint32_t SequenceNumber, uint64_t SignalID)
{
    signal_register_response(mNodeInputBuffer, SequenceNumber,SignalID);
}









