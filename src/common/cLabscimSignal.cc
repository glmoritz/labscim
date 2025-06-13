#include "cLabscimSignal.h"

using namespace omnetpp;

namespace labscim {

cLabscimSignal::cLabscimSignal(uint64_t SignalID, char* msg, uint64_t size)
{
    gMessage = new char[size];
    memcpy(gMessage,msg,size);
    gSize = size;
    gSignalID = SignalID;
}

cLabscimSignal::~cLabscimSignal()
{
    delete[] gMessage;
}

cObject* cLabscimSignal::dup() const
{
    return new cLabscimSignal(gSignalID, gMessage,gSize);
}

}
