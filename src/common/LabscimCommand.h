/*
 * LabscimCommand.h
 *
 *  Created on: 2 de jun de 2020
 *      Author: guilherme
 */

#ifndef COMMON_LABSCIMCOMMAND_H_
#define COMMON_LABSCIMCOMMAND_H_

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>



class LabscimCommand {
public:
    LabscimCommand();

    LabscimCommand(struct labscim_set_time_event* msg);

    LabscimCommand(struct labscim_protocol_yield* msg);

    LabscimCommand(struct labscim_print_message* msg);

    uint16_t GetType();
    virtual ~LabscimCommand();

    LabscimCommand(const LabscimCommand &other);

    LabscimCommand& operator=(LabscimCommand other);

    void* GetData();

private:
    void* mData;
    size_t mDataSize;
    uint16_t mType;
};

#endif /* COMMON_LABSCIMCOMMAND_H_ */
