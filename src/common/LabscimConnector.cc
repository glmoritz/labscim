/*
 * LabscimConnector.cc
 *
 *  Created on: 26 de mai de 2020
 *      Author: guilherme
 */

#include "LabscimConnector.h"
#include <string>
#include <errno.h>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <sys/time.h>

using namespace labscim;
using namespace std;
using namespace omnetpp;

std::string LabscimConnector::ExecuteCommand(const char* cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe)
    {
        char err[256];
        sprintf(err, "popen() failed (%s)!",strerror(errno));
        throw std::runtime_error(err);
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
        end_simulation(mNodeInputBuffer);
        labscim_buffer_deinit(mNodeInputBuffer, 1);
        delete mNodeInputBuffer;
    }
    labscim_buffer_deinit(mNodeOutputBuffer, 0);
    labscim_socket_disconnect(mNodeOutputBuffer);
    delete mNodeOutputBuffer;
}

string LabscimConnector::GenerateRandomString(const int len)
{
    string tmp_s;
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    //srand( (unsigned) time(NULL) * getpid());

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
    {
        tmp_s += alphanum[(rand()>>24) % (sizeof(alphanum) - 1)];
    }
    return tmp_s;
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
            free(cmd);
            return -1;
        }
        labscim_buffer_init(mNodeInputBuffer, (char*)(std::string("/") + node_name + std::string("in")).c_str(), BufferSize, 0);
        free(cmd);
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

void LabscimConnector::SendSignal(uint64_t Signal, uint64_t CurrentTime, void* SignalStruct, size_t SignalStructLen)
{
    send_signal(mNodeInputBuffer, Signal, CurrentTime, SignalStruct, SignalStructLen);
    //radio_response(mNodeInputBuffer, (uint16_t)Signal, CurrentTime, SignalStruct, SignalStructLen, 5);
}


void LabscimConnector::SendRegisterResponse(uint32_t SequenceNumber, uint64_t SignalID)
{
    signal_register_response(mNodeInputBuffer, SequenceNumber,SignalID);
}

void LabscimConnector::SendRandomNumber(uint32_t SequenceNumber, union random_number Result)
{
    send_random(mNodeInputBuffer, Result, SequenceNumber);
}


void LabscimConnector::GenerateRandomNumber(omnetpp::cRNG *rng, uint8_t distribution_type, union random_number param_1, union random_number param_2,union random_number param_3, union random_number* result)
{
    switch(distribution_type)
    {
    default:
    case 0: // - uniform(a, b)   uniform distribution in the range [param_1,param_2)
    {
        result->double_number = omnetpp::uniform(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 1: //exponential(mean)   exponential distribution with the given mean
    {
        result->double_number = omnetpp::exponential(rng,param_1.double_number);
        break;
    }
    case 2: //normal(mean, stddev)    normal distribution with the given mean and standard deviation
    {
        result->double_number = omnetpp::normal(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 3: //truncnormal(mean, stddev)   normal distribution truncated to nonnegative values
    {
        result->double_number = omnetpp::truncnormal(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 4: //gamma_d(alpha, beta)    gamma distribution with parameters alpha>0, beta>0
    {
        result->double_number = omnetpp::gamma_d(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 5: //beta(alpha1, alpha2)    beta distribution with parameters alpha1>0, alpha2>0
    {
        result->double_number = omnetpp::beta(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 6: //erlang_k(k, mean)   Erlang distribution with k>0 phases and the given mean
    {
        result->double_number = omnetpp::erlang_k(rng,param_1.int_number,param_2.double_number);
        break;
    }
    case 7: //chi_square(k)   chi-square distribution with k>0 degrees of freedom
    {
        result->double_number = omnetpp::chi_square(rng,param_1.int_number);
        break;
    }
    case 8: //student_t(i)    student-t distribution with i>0 degrees of freedom
    {
        result->double_number = omnetpp::student_t(rng,param_1.int_number);
        break;
    }
    case 9: //cauchy(a, b)    Cauchy distribution with parameters a,b where b>0
    {
        result->double_number = omnetpp::cauchy(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 10: //triang(a, b, c)    triangular distribution with parameters a<=b<=c, a!=c
    {
        result->double_number = omnetpp::triang(rng,param_1.double_number,param_2.double_number,param_3.double_number);
        break;
    }
    case 11: //lognormal(m, s)    lognormal distribution with mean m and variance s>0
    {
        result->double_number = omnetpp::lognormal(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 12: //weibull(a, b)  Weibull distribution with parameters a>0, b>0
    {
        result->double_number = omnetpp::weibull(rng,param_1.double_number,param_2.double_number);
        break;
    }
    case 13: //pareto_shifted(a, b, c)    generalized Pareto distribution with parameters a, b and shift c1
    {
        result->double_number = omnetpp::pareto_shifted(rng,param_1.double_number,param_2.double_number,param_3.double_number);
        break;
    }
    //Discrete distributions
    case 14: //intuniform(a, b)   uniform integer from a..b
    {
        result->int_number = omnetpp::intuniform(rng,param_1.int_number,param_2.int_number);
        break;
    }
    case 15: //bernoulli(p)   result of a Bernoulli trial with probability 0<=p<=1 (1 with probability p and 0 with probability (1-p))
    {
        result->int_number = omnetpp::bernoulli(rng,param_1.double_number);
        break;
    }
    case 16: //binomial(n, p) binomial distribution with parameters n>=0 and 0<=p<=1
    {
        result->int_number = omnetpp::binomial(rng,param_1.int_number,param_2.double_number);
        break;
    }
    case 17: //geometric(p)   geometric distribution with parameter 0<=p<=1
    {
        result->int_number = omnetpp::geometric(rng,param_1.double_number);
        break;
    }
    case 18: //negbinomial(n, p)  negative binomial distribution with parameters n>0 and 0<=p<=1
    {
        result->int_number = omnetpp::negbinomial(rng,param_1.int_number,param_2.double_number);
        break;
    }
    case 19: //poisson(lambda)    Poisson distribution with parameter lambda
    {
        result->int_number = omnetpp::poisson(rng,param_1.double_number);
        break;
    }
    }
}









