/* -*- mode:c++ -*- ********************************************************
 * file:        Ieee802154Mac.h
 *
 * author:     Jerome Rousselot, Marcel Steine, Amre El-Hoiydi,
 *                Marc Loebbers, Yosia Hadisusanto
 *
 * copyright:    (C) 2007-2009 CSEM SA
 *              (C) 2009 T.U. Eindhoven
 *                (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 *
 * Funding: This work was partially financed by the European Commission under the
 * Framework 6 IST Project "Wirelessly Accessible Sensor Populations"
 * (WASP) under contract IST-034963.
 ***************************************************************************
 * part of:    Modifications to the MF-2 framework by CSEM
 **************************************************************************/

#ifndef __LABSCIM_ContikiNGIeee802154GlueMac_H
#define __LABSCIM_ContikiNGIeee802154GlueMac_H

#include "inet/queueing/contract/IPacketQueue.h"
#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "../../common/LabscimConnector.h"
#include "../../common/labscim_contiking_setup.h"
#include "../../common/labscim-contiki-radio-protocol.h"

using namespace inet;


namespace labscim {

/**
 * @brief Generic CSMA Mac-Layer.
 *
 * Supports constant, linear and exponential backoffs as well as
 * MAC ACKs.
 *
 * @author Jerome Rousselot, Amre El-Hoiydi, Marc Loebbers, Yosia Hadisusanto, Andreas Koepke
 * @author Karl Wessel (port for MiXiM)
 *
 * \image html csmaFSM.png "CSMA Mac-Layer - finite state machine"
 */
class INET_API ContikiNGIeee802154GlueMac : public MacProtocolBase, public IMacProtocol, public LabscimConnector
{
public:
    ContikiNGIeee802154GlueMac()
: MacProtocolBase()
, LabscimConnector()
, radio(nullptr)
, transmissionState(physicallayer::IRadio::TRANSMISSION_STATE_UNDEFINED)
, headerLength(0)
, ccaDetectionTime()
, rxSetupTime()
, aTurnaroundTime()
, aUnitBackoffPeriod()
, txPower(0)
{

        mCurrentMode = 0;
        mCurrentCenterFrequency = Hz(0);
        mCurrentBandwidth = Hz(0);
        mCurrentPower = W(0);
        mCurrentBitrate = bps(0);
        mCCATimerMsg = nullptr;
        mTransmitRequestSeqNo = 0;
        mRadioConfigured = false;
}

    virtual ~ContikiNGIeee802154GlueMac();

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int) override;

    /** @brief Delete all dynamically allocated objects of the module*/
    virtual void finish() override;

    /** @brief Handle messages from lower layer */
    virtual void handleLowerPacket(Packet *packet) override;

    /** @brief Handle messages from upper layer */
    virtual void handleUpperPacket(Packet *packet) override;

    /** @brief Handle self messages such as timers */
    virtual void handleSelfMessage(cMessage *) override;

    /** @brief Handle control messages from lower layer */
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;


protected:
    uint32_t nbBufferSize;

    uint64_t RegisterSignal(uint8_t* signal_name);

    /** @brief Self Messages from module
     * see states diagram.
     */
    enum self_msgs {
        BOOT_MSG = 1,
        CONTIKI_TIMER_MSG = 2,
        CCA_ENDED = 3
    };

    /*************************************************************/
    /****************** TYPES ************************************/
    /*************************************************************/

    /** @brief The radio. */
    physicallayer::IRadio *radio;
    physicallayer::IRadio::TransmissionState transmissionState;

    simtime_t mLastRadioModeSwitch;
    simtime_t mRadioModeTimes[physicallayer::IRadio::RadioMode::RADIO_MODE_SWITCHING+1];
    simsignal_t mRadioModeTimesSignals[physicallayer::IRadio::RadioMode::RADIO_MODE_SWITCHING+1];
    physicallayer::IRadio::RadioMode mLastRadioMode;


    /** @brief Length of the header*/
    int headerLength;

    /** @brief CCA detection time */
    simtime_t ccaDetectionTime;
    /** @brief Time to setup radio from sleep to Rx state */
    simtime_t rxSetupTime;
    /** @brief Time to switch radio from Rx to Tx state */
    simtime_t aTurnaroundTime;
    /** @brief base time unit for calculating backoff durations */
    simtime_t aUnitBackoffPeriod;
    /** @brief Stores if the MAC expects Acks for Unicast packets.*/

    /** @brief The power (in mW) to transmit with.*/
    double txPower;

    std::vector<std::string> mRegisteredSignals;
    std::vector<uint64_t> mSubscribedSignals;

    cProperty *statisticTemplate;

    std::string mNodeName;

    int mCurrentMode; //the current radio mode (maybe redundant)
    Hz mCurrentCenterFrequency;
    Hz mCurrentBandwidth;
    W mCurrentPower;
    /** @brief the bit rate at which we transmit */
    bps mCurrentBitrate;

    bool mRadioConfigured;

    std::list<cMessage*> mScheduledTimerMsgs;
    cMessage* mCCATimerMsg;
    uint32_t mTransmitRequestSeqNo;



protected:
    /** @brief Generate new interface address*/
    virtual void configureInterfaceEntry() override;
    virtual void handleCommand(cMessage *msg) {}

    void PerformRadioCommand(struct labscim_radio_command* cmd);
    void configureRadio(Hz CenterFrequency, Hz Bandwidth, W Power, bps Bitrate, int mode /*= -1*/);
    void attachSignal(Packet *mac, simtime_t_cref startTime);
    cMessage* GetScheduledTimeEvent(uint32_t sequence_number);

    void ProcessCommands();

private:

    /** @brief Copy constructor is not allowed.
     */
    ContikiNGIeee802154GlueMac(const ContikiNGIeee802154GlueMac&);
    /** @brief Assignment operator is not allowed.
     */
    ContikiNGIeee802154GlueMac& operator=(const ContikiNGIeee802154GlueMac&);
};

} // namespace inet

#endif // __LABSCIM_ContikiNGIeee802154GlueMac_H

