/*
 * LabscimCommand.cpp
 *
 *  Created on: 2 de jun de 2020
 *      Author: guilherme
 */

#include "LabscimCommand.h"
#include "labscim_protocol.h"

LabscimCommand::LabscimCommand(struct labscim_set_time_event* msg)
{
    mData = (void*)msg;
    mType = LABSCIM_SET_TIME_EVENT;
}

LabscimCommand::LabscimCommand(struct labscim_protocol_yield* msg)
{
    mData = NULL;
    free(msg);
    mType = LABSCIM_PROTOCOL_YIELD;
}

LabscimCommand::LabscimCommand(struct labscim_print_message* msg)
{
    mData = (void*)msg;
    mType = LABSCIM_PRINT_MESSAGE;
}

LabscimCommand::LabscimCommand(const LabscimCommand &other)
{
    if(other.mData!=NULL)
    {
        mData = malloc(other.mDataSize);
        if(mData!=NULL)
        {
            memcpy(mData,other.mData,other.mDataSize);
            mDataSize = other.mDataSize;
        }
        else
        {
            mDataSize = 0;
        }
    }
    mType = other.mType;
}

LabscimCommand& LabscimCommand::operator=(LabscimCommand other)
{
    if(other.mData!=NULL)
    {
        mData = malloc(other.mDataSize);
        if(mData!=NULL)
        {
            memcpy(mData,other.mData,other.mDataSize);
            mDataSize = other.mDataSize;
        }
        else
        {
            mDataSize = 0;
        }
    }
    mType = other.mType;
    return *this;
}

uint16_t LabscimCommand::GetType()
{
    return mType;
}

void* LabscimCommand::GetData()
{
    return mData;
}

LabscimCommand::~LabscimCommand()
{
    if(mData!=NULL)
    {
        free(mData);
    }
}

