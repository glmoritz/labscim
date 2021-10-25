/*
 * labscim_log.h
 *
 *  Created on: 16 de jul de 2020
 *      Author: root
 */

#ifndef LABSCIM_LOG_H_
#define LABSCIM_LOG_H_

void Node_Log(double time, uint64_t NodeID, uint8_t* text);
void Node_Log_Output(double time, uint64_t NodeID, uint8_t* text);

#endif /* LABSCIM_LOG_H_ */
