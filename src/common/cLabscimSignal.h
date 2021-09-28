

#ifndef __LABSCIM_LABSCIM_SIGNAL_H
#define __LABSCIM_LABSCIM_SIGNAL_H

#include <omnetpp.h>

using namespace omnetpp;

namespace labscim {



/**
 * @brief A straightforward implementation of labscim signal.
 *
 * @ingroup Signals
 */
class SIM_API cLabscimSignal : public cObject
{
    public:
        char* gMessage;
        uint64_t gSize;

    public:
        cLabscimSignal(char* msg, uint64_t size);
        ~cLabscimSignal();

        void getMessage(char* buf, size_t MaxSize){memcpy(buf,gMessage,MaxSize>gSize?gSize:MaxSize);};
        uint64_t getMessageSize(){return gSize;};

        virtual cObject *dup() const override;

};

}

#endif

