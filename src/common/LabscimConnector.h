/*
 * LabscimConnector.h
 *
 *  Created on: 26 de mai de 2020
 *      Author: guilherme
 */

#ifndef COMMON_LABSCIMCONNECTOR_H_
#define COMMON_LABSCIMCONNECTOR_H_

#include "labscim_protocol.h"
#include "labscim_socket.h"
#include "LabscimCommand.h"
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <omnetpp.h>
#include <string>
#include <list>


namespace labscim {

using namespace std;

class LabscimConnector {
public:
    LabscimConnector();
    virtual ~LabscimConnector();

    int32_t SpawnProcess(std::string processcommand, uint32_t serverport, uint32_t BufferSize);

    std::string ExecuteCommand(const char* cmd);

    int32_t SpawnProcess(std::string processcommand, std::string node_name, uint32_t serverport, uint32_t BufferSize);

    void SendProtocolBoot(void* ConfigMessage,size_t ConfigMessageSize);

    void SendTimeEvent(uint32_t SequenceNumber, uint32_t TimeEventID, uint64_t CurrentTime_us);

    void SendRadioResponse(uint16_t RadioResponse, uint64_t CurrentTime, void* RadioStruct, size_t RadioStructLen, uint32_t SequenceNumber);

    void SendRegisterResponse(uint32_t SequenceNumber, uint64_t SignalID);

    void SendRandomNumber(uint32_t SequenceNumber, union random_number Result);

    void GenerateRandomNumber(omnetpp::cRNG *rng, uint8_t distribution_type, union random_number param_1, union random_number param_2,union random_number param_3, union random_number* result);

    int32_t WaitForCommand();

private:

    uint8_t* mSocketBuffer;
    int64_t mBufferSize;
    uint32_t mPort;

    //data structures from labscim protocol

    buffer_circ_t* mNodeInputBuffer;
    buffer_circ_t* mNodeOutputBuffer;

protected:
    struct labscim_ll mCommands;

};

}

#endif /* COMMON_LABSCIMCONNECTOR_H_ */
