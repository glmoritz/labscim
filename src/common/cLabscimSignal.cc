#include "cLabscimSignal.h"

using namespace omnetpp;

namespace labscim {

cLabscimSignal::cLabscimSignal(char* msg, uint64_t size)
{
    gMessage = new char[size];
    memcpy(gMessage,msg,size);
    gSize = size;
}

cLabscimSignal::~cLabscimSignal()
{
    delete[] gMessage;
}

cObject* cLabscimSignal::dup() const
{
    return new cLabscimSignal(gMessage,gSize);
}

}
