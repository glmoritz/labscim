/* -*- mode:c++ -*- ********************************************************
 * file:        Ieee802154Mac.cc
 *
 * author:      Jerome Rousselot, Marcel Steine, Amre El-Hoiydi,
 *              Marc Loebbers, Yosia Hadisusanto, Andreas Koepke
 *
 * copyright:   (C) 2007-2009 CSEM SA
 *              (C) 2009 T.U. Eindhoven
 *                (C) 2004,2005,2006
 *              Telecommunication Networks Group (TKN) at Technische
 *              Universitaet Berlin, Germany.
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
#include <cassert>
#include <math.h>

#include "cryptopp/cryptlib.h"
#include "cryptopp/sha.h"

#include "LoRaMacNodeGlueMac.h"
#include "inet/common/FindModule.h"
#include "inet/common/INETMath.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/Message.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/ieee802154/Ieee802154Mac.h"
#include "inet/linklayer/ieee802154/Ieee802154MacHeader_m.h"
#include "inet/physicallayer/contract/packetlevel/SignalTag_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "../../common/LabscimConnector.h"
#include "../../common/labscim-lora-radio-protocol.h"
#include "../../common/labscim_loramac_setup.h"
#include "../../common/labscim_log.h"
#include "../../common/labscim_socket.h"
#include "../../common/sx126x_labscim.h"
#include "../../common/cLabscimSignal.h"
#include "../../physicallayer/lora/packetlevel/LoRaTags_m.h"
#include "../../physicallayer/lora/packetlevel/LoRaRadioControlInfo_m.h"
#include "../../physicallayer/lora/packetlevel/LoRaDimensionalTransmitter.h"
#include "../../physicallayer/lora/packetlevel/LoRaDimensionalReceiver.h"
#include "../../physicallayer/lora/packetlevel/LoRaRadio.h"

using namespace inet::physicallayer;
using namespace omnetpp;
using namespace inet;
using namespace CryptoPP;

namespace labscim {

Define_Module(LoRaMacNodeGlueMac);

uint64_t gTimeReference = 0;

void LoRaMacNodeGlueMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        headerLength = par("headerLength");
        ccaDetectionTime = par("ccaDetectionTime");
        rxSetupTime = par("rxSetupTime");
        aTurnaroundTime = par("aTurnaroundTime");
        statisticTemplate = getProperties()->get("statisticTemplate", "nbStats");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);

        mLastRadioModeSwitch=0;
        for(uint32_t i=0;i<inet::physicallayer::IRadio::RadioMode::RADIO_MODE_SWITCHING+1;i++)
        {
            mRadioModeTimes[i]=0;
        }
        mLastRadioMode = inet::physicallayer::IRadio::RadioMode::RADIO_MODE_OFF;
        mRadioModeTimesSignals[inet::physicallayer::IRadio::RadioMode::RADIO_MODE_OFF] = cComponent::registerSignal("radioOffTimeChanged");
        mRadioModeTimesSignals[inet::physicallayer::IRadio::RadioMode::RADIO_MODE_SLEEP] = cComponent::registerSignal("radioSleepTimeChanged");
        mRadioModeTimesSignals[inet::physicallayer::IRadio::RadioMode::RADIO_MODE_RECEIVER] = cComponent::registerSignal("radioRxTimeChanged");
        mRadioModeTimesSignals[inet::physicallayer::IRadio::RadioMode::RADIO_MODE_TRANSMITTER] = cComponent::registerSignal("radioTxTimeChanged");
        mRadioModeTimesSignals[inet::physicallayer::IRadio::RadioMode::RADIO_MODE_TRANSCEIVER] = cComponent::registerSignal("radioTxRxTimeChanged");
        mRadioModeTimesSignals[inet::physicallayer::IRadio::RadioMode::RADIO_MODE_SWITCHING] = cComponent::registerSignal("radioSwitchingTimeChanged");

        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(labscim::physicallayer::LoRaRadio::loraradio_datarate_changed, this);

        radio = check_and_cast<IRadio *>(radioModule);
        mLoRaRadio = check_and_cast<labscim::physicallayer::LoRaRadio*>(radio);

        //check parameters for consistency
        //aTurnaroundTime should match (be equal or bigger) the RX to TX
        //switching time of the radio
        if (radioModule->hasPar("timeRXToTX")) {
            simtime_t rxToTx = radioModule->par("timeRXToTX");
            if (rxToTx > aTurnaroundTime) {
                throw cRuntimeError("Parameter \"aTurnaroundTime\" (%f) does not match"
                        " the radios RX to TX switching time (%f)! It"
                        " should be equal or bigger",
                        SIMTIME_DBL(aTurnaroundTime), SIMTIME_DBL(rxToTx));
            }
        }
        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);



        EV_DETAIL << "Finished LoRaMAC Glue MAC init stage 1." << endl;
    }
    else if (stage == INITSTAGE_NETWORK_CONFIGURATION)
    {
        double boot_time = par("BootTime").doubleValue();
        uint32_t ServerPort = par("NodeProcessConnectionPort").intValue();
        char msgname[64];
        cMessage* BootMsg;
        std::string cmd("");
        std::stringstream stream;
        stream << "node-" << std::hex << interfaceEntry->getMacAddress().getInt();
        mNodeName = std::string(stream.str() );

        std::string MemoryName = std::string("labscim-") + mNodeName + std::string("-") + GenerateRandomString(16);

        nbBufferSize = par("SocketBufferSize").intValue();
        interfaceEntry->setDatarate(mLoRaRadio->getPacketDataRate().get());

        if(!par("NodeDebug").boolValue())
        {
#ifdef LABSCIM_REMOTE_SOCKET //shared memory communication
            cmd = std::string("ssh ") + std::string(par("NodeProcessSshAddress").stringValue()) + std::string(" /usr/bin/nohup ");
#endif
            cmd = cmd + std::string(par("NodeProcessCommand").stringValue()) + std::string(" -b") + std::to_string(par("SocketBufferSize").intValue());
            cmd = cmd + std::string(" -p") + std::to_string(par("NodeProcessConnectionPort").intValue());
            cmd = cmd + std::string(" -n") + MemoryName + std::string(" -alocalhost");
            cmd = cmd + std::string(" ") + std::string(par("NodeExtraArguments").stringValue());
            cmd = cmd + std::string(" > /dev/null 2> /dev/null < /dev/null &");
            //cmd = cmd + std::string(" &");
        }
        else
        {
            MemoryName = std::string("labscim-debug-") + mNodeName;
        }

        //ssh guilherme@guilherme-ubuntu.local '/usr/bin/nohup /home/guilherme/contiki-ng/examples/hello-world/hello-world.labscim > /dev/null 2> /dev/null < /dev/null &'

        SpawnProcess(cmd, MemoryName, ServerPort, nbBufferSize);

        //SpawnProcess("::1", "/home/guilherme/contiking/examples/hello/hello","-b 512 -p 9608" , 9608, nbBufferSize);
        BootMsg = new cMessage((mNodeName + "-boot").c_str());
        BootMsg->setKind(BOOT_MSG);
        scheduleAt(boot_time, BootMsg);

        //create the RX timer msg
        mRXTimerMsg = new cMessage((mNodeName + "-rxtimer").c_str());

        mRXTimerMsg->setKind(RX_TIMER);

        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
    }
}

void LoRaMacNodeGlueMac::finish()
{
    mRadioModeTimes[mLastRadioMode] += simTime() - mLastRadioModeSwitch;
    emit(mRadioModeTimesSignals[mLastRadioMode], mRadioModeTimes[mLastRadioMode]);
}

LoRaMacNodeGlueMac::~LoRaMacNodeGlueMac()
{
    for (cMessage* ptr: mScheduledTimerMsgs)
    {
        cancelAndDelete(ptr);
    }
    mScheduledTimerMsgs.clear();
    if(mCCATimerMsg!=nullptr)
    {
        cancelAndDelete(mCCATimerMsg);
    }
    if(mRXTimerMsg!=nullptr)
    {
        cancelAndDelete(mRXTimerMsg);
    }
}

void LoRaMacNodeGlueMac::configureInterfaceEntry()
{
    MacAddress address = parseMacAddressParameter(par("address"));

    // data rate
    const double EstimatedDataRate =  5469; //SF7 @ 125kHz -> will be adjusted upon radio configuration
    interfaceEntry->setDatarate(EstimatedDataRate);

    // generate a link-layer address to be used as interface token for IPv6
    interfaceEntry->setMacAddress(address);
    interfaceEntry->setInterfaceToken(address.formInterfaceIdentifier());

    // capabilities
    interfaceEntry->setMtu(par("mtu"));
    interfaceEntry->setMulticast(true);
    interfaceEntry->setBroadcast(true);
}

/**
 * Encapsulates the message to be transmitted and pass it on
 * to the FSM main method for further processing.
 */
void LoRaMacNodeGlueMac::handleUpperPacket(Packet *packet)
{
    //we just ignore any upper packet
    delete packet;
}

void LoRaMacNodeGlueMac::attachSignal(Packet *mac, simtime_t_cref startTime)
{
    mac->setDuration(mLoRaRadio->getPacketRadioTimeOnAir(mac));
}

void LoRaMacNodeGlueMac::configureRadio(ConfigureLoRaRadioCommand* config_msg)
{
    auto request = new Message("ConfigureRadio", RADIO_C_CONFIGURE);
    request->setControlInfo(config_msg);
    sendDown(request);
}


void LoRaMacNodeGlueMac::configureRadio(Hz CenterFrequency, Hz Bandwidth, W Power, bps Bitrate, int mode /*= -1*/)
{

    auto configureCommand = new ConfigureLoRaRadioCommand();

#ifdef LABSCIM_LOG_COMMANDS
    std::stringstream stream;
    stream << "Radio CH " << "?" << " at " << CenterFrequency  << ", " << Bitrate << " @ " << Bandwidth << " BW " << Power << " tx. Radio mode: ";
    switch(mode)
    {
    case IRadio::RADIO_MODE_OFF:
    {
        stream << "OFF";
        break;
    }
    case IRadio::RADIO_MODE_SLEEP:
    {
        stream << "SLEEPING";
        break;
    }
    case IRadio::RADIO_MODE_RECEIVER:
    {
        stream << "RECEIVER";
        break;
    }
    case IRadio::RADIO_MODE_TRANSMITTER:
    {
        stream << "TRANSMITTER";
        break;
    }
    case IRadio::RADIO_MODE_TRANSCEIVER:
    {
        stream << "TRANSCEIVER";
        break;
    }
    case IRadio::RADIO_MODE_SWITCHING:
    {
        stream << "SWITCHING";
        break;
    }
    }
    stream << "\n";
    Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
    EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif

    configureCommand->setPower(Power);
    configureCommand->setBitrate(Bitrate);
    configureCommand->setCenterFrequency(CenterFrequency);
    configureCommand->setBandwidth(Bandwidth);
    configureCommand->setRadioMode(mode);
    configureRadio(configureCommand);

}


void LoRaMacNodeGlueMac::PerformRadioCommand(struct labscim_radio_command* cmd)
{
    switch(cmd->radio_command)
    {
    case LORA_RADIO_SEND:
    {
        struct lora_radio_payload* payload = (struct lora_radio_payload*)cmd->radio_struct;
        auto cmsg = new Packet((mNodeName + "-packet").c_str());
        auto dataMessage = makeShared<BytesChunk>();
        std::vector<uint8_t> vec(payload->Message, payload->Message + payload->MessageSize_bytes);
        dataMessage->setBytes(vec);
        cmsg->addTag<CreationTimeTag>()->setCreationTime(simTime());
        cmsg->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ieee802154);
        cmsg->insertAtBack(dataMessage);

        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
        attachSignal(cmsg, simTime() + aTurnaroundTime);
        mTransmitRequestSeqNo = cmd->hdr.sequence_number;

        // give time for the radio to be in Tx state before transmitting
        sendDelayed(cmsg, aTurnaroundTime, lowerLayerOutGateId);

#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "Sending " << payload->MessageSize_bytes << " bytes\n";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
        free(cmd);
        break;
    }
    case LORA_RADIO_IS_CHANNEL_FREE:
    {
        if(mCCATimerMsg == nullptr)
        {
            struct lora_is_channel_free* params = (struct lora_is_channel_free*)cmd;
            cMessage* CCAMsg;
            double us = simTime().dbl() * 1000000;
            CCAMsg = new cMessage((mNodeName + "-perform-cca").c_str());
            CCAMsg->setKind(CCA_ENDED);
            CCAMsg->setContextPointer((void*)cmd);
            mCCATimerMsg = CCAMsg;
            scheduleAt(us + rxSetupTime + ((double)params->MaxCarrierSenseTime_us)/1000000.0, CCAMsg);
#ifdef LABSCIM_LOG_COMMANDS
            std::stringstream stream;
            stream << "CCA Start\n";
            Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
            EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
            auto configureCommand = new ConfigureLoRaRadioCommand();
            configureCommand->setBandwidth(Hz(params->RxBandWidth_Hz));
            configureCommand->setCenterFrequency(Hz(params->Frequency_Hz));
            configureCommand->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            configureRadio(configureCommand);
        }
    }
    case LORA_RADIO_GET_STATE:
    {
        struct lora_radio_status state;
        state.RadioMode = (uint32_t)radio->getRadioMode();
        state.ChannelIsFree = (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE)?1:0;
        SendRadioResponse(LORA_RADIO_GET_STATE_RESULT, (uint64_t)std::round((simTime().dbl() * 1000000)),(uint8_t*)&state, sizeof(struct lora_radio_status), cmd->hdr.sequence_number);
        free(cmd);
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "Radio get state: ";
        switch(state.RadioMode)
        {
        case RECEPTION_STATE_UNDEFINED:
        {
            stream << "UNDEFINED";
            break;
        }
        case RECEPTION_STATE_IDLE:
        {
            stream << "IDLE";
            break;
        }
        case RECEPTION_STATE_BUSY:
        {
            stream << "BUSY";
            break;
        }
        case RECEPTION_STATE_RECEIVING:
        {
            stream << "RECEIVING";
            break;
        }
        }
        stream << "\n";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
        break;
    }
    case LORA_RADIO_SET_MODEM:
    {
        struct lora_set_modem* modem_type = (struct lora_set_modem*)cmd->radio_struct;
        if(!mRadioConfigured)
        {
            cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
            mRadioConfigured = true;
            radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        }
        if(modem_type->Modem!=1)
        {
            throw cRuntimeError("Only LoRa Modulation is Supported by this module");
        }
        free(cmd);
        break;
    }
    case LORA_RADIO_SET_MODULATION_PARAMS:
    {
        struct lora_set_modulation_params* mp = (struct lora_set_modulation_params*)cmd->radio_struct;
        auto configureCommand = new ConfigureLoRaRadioCommand();
        configureCommand->setPower(mW(dBmW2mW(mp->TransmitPower_dBm)));
        configureCommand->setLoRaSF(mp->ModulationParams.Params.LoRa.SpreadingFactor);
        configureCommand->setLoRaCR(mp->ModulationParams.Params.LoRa.CodingRate);
        configureCommand->setLowDataRate_optimization(mp->ModulationParams.Params.LoRa.LowDatarateOptimize);
        configureCommand->setBandwidth(Hz(labscim::physicallayer::LoRaDimensionalTransmitter::RadioGetLoRaBandwidthInHz(mp->ModulationParams.Params.LoRa.Bandwidth)));
        free(cmd);
        configureRadio(configureCommand);
        break;
    }
    case LORA_RADIO_SET_PACKET_PARAMS:
    {
        struct lora_set_packet_params* pp = (struct lora_set_packet_params*)cmd->radio_struct;
        auto configureCommand = new ConfigureLoRaRadioCommand();
        configureCommand->setHeader_enabled(pp->PacketParams.Params.LoRa.HeaderType);
        configureCommand->setPayload_length(pp->PacketParams.Params.LoRa.PayloadLength);
        configureCommand->setCRC_enabled(pp->PacketParams.Params.LoRa.CrcMode);
        configureCommand->setPreamble_length(pp->PacketParams.Params.LoRa.PreambleLength);
        free(cmd);
        configureRadio(configureCommand);
        break;
    }
    case LORA_RADIO_SET_RX:
    {
        struct lora_set_rx* srx = (struct lora_set_rx*)cmd->radio_struct;
        mRx_window_us=srx->Timeout_us;
        mSleep_window_us=0;
        cancelEvent(mRXTimerMsg);

        switch(srx->Timeout_us)
        {
        case (~((uint64_t)0)):
                  {
            mRX_fsm = RX_CONTINUOUS_START;
            break;
                  }
        default:
        {
            //single mode
            mRX_fsm = RX_SINGLE_START;
            break;
        }
        }
        scheduleAt(simTime(), mRXTimerMsg);
        free(cmd);
        break;
    }
    case LORA_RADIO_SET_RX_DUTYCYCLE:
    {
        struct lora_set_rx_dutycycle* srx = (struct lora_set_rx_dutycycle*)cmd->radio_struct;
        mRx_window_us=srx->Rx_window_us;
        mSleep_window_us=srx->Sleep_window_us;
        cancelEvent(mRXTimerMsg);
        mRX_fsm = RX_WINDOW_START;
        scheduleAt(simTime(), mRXTimerMsg);
        free(cmd);
        break;
    }
    case LORA_RADIO_SET_IDLE:
    {
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        mRX_fsm = RX_IDLE;
        cancelEvent(mRXTimerMsg);
        free(cmd);
        break;
    }
    case LORA_RADIO_SET_CHANNEL:
    {
        struct lora_set_frequency* lsf = (struct lora_set_frequency*)cmd->radio_struct;
        auto configureCommand = new ConfigureLoRaRadioCommand();
        configureCommand->setCenterFrequency(Hz(lsf->Frequency_Hz));
        free(cmd);
        configureRadio(configureCommand);
        break;
    }
    default:
    {
        free(cmd);
        break;
    }
    }
}

cMessage* LoRaMacNodeGlueMac::GetScheduledTimeEvent(uint32_t sequence_number)
{
    std::list<cMessage*>::iterator it;
    for (it = mScheduledTimerMsgs.begin(); it != mScheduledTimerMsgs.end(); ++it)
    {
        if((*it)->getContextPointer()!=nullptr)
        {
            struct labscim_set_time_event* ste = (struct labscim_set_time_event*)((*it)->getContextPointer());
            if(ste->hdr.sequence_number == sequence_number)
            {
                return *it;
            }
        }
    }
    EV_ERROR << "Could not find timer";
    return nullptr;
}

void LoRaMacNodeGlueMac::ProcessCommands()
{
    uint32_t CommandsExecuted=0;
    void* cmd;
    uint32_t CommandsReceived=1;
    uint32_t YieldReceived=0;
    while(!YieldReceived)
    {
#ifdef LABSCIM_LOG_COMMANDS
        char log[128];
#endif
        CommandsReceived = WaitForCommand();
        if(CommandsReceived==0)
        {
            //TODO: Node is dead (500ms timeout). Notify and Drop
        }
        do
        {
            cmd = labscim_ll_pop_front(&mCommands);
            if(cmd!=NULL)
            {
                struct labscim_protocol_header* hdr = (struct labscim_protocol_header*)cmd;
                switch(hdr->labscim_protocol_code)
                {
                case LABSCIM_PROTOCOL_YIELD:
                {
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tPROTOCOL_YIELD\n",hdr->sequence_number);
                    std::stringstream stream;
                    stream << "Yield\n" << "\n";
                    Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
                    EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
                    YieldReceived = 1;
                    free(cmd);
                    break;
                }
                case LABSCIM_SET_TIME_EVENT:
                {
                    struct labscim_set_time_event* ste = (struct labscim_set_time_event*)cmd;
                    cMessage* TimeEventMsg;
                    double us = simTime().dbl() * 1000000;
                    TimeEventMsg = new cMessage((mNodeName + "-timer").c_str());
                    TimeEventMsg->setKind(LORAMAC_TIMER_MSG);
                    TimeEventMsg->setContextPointer((void*)ste);
                    if(ste->is_relative)
                    {
#ifdef LABSCIM_LOG_COMMANDS
                        sprintf(log,"seq%4d\tSET_TIME_EVENT,%s, %f\n",hdr->sequence_number,mNodeName,  (us + (double)ste->time_us ) / 1000000);
#endif


                        mScheduledTimerMsgs.push_front(TimeEventMsg);
                        scheduleAt((us + (double)ste->time_us)/1000000, TimeEventMsg);
                    }
                    else
                    {
                        if(ste->time_us >= us)
                        {
#ifdef LABSCIM_LOG_COMMANDS
                            sprintf(log,"seq%4d\tSET_TIME_EVENT, %s, %f\n",hdr->sequence_number, mNodeName, ste->time_us);
#endif
                            mScheduledTimerMsgs.push_front(TimeEventMsg);
                            scheduleAt((double)ste->time_us/1000000, TimeEventMsg);
                        }
                        else
                        {
                            EV_ERROR << "scheduling a past timer";
#ifdef LABSCIM_LOG_COMMANDS
                            sprintf(log,"seq%4d\tSET_TIME_EVENT, %s, %f, ERROR - PAST TIMER\n",hdr->sequence_number, mNodeName, ste->time_us);
#endif
                            free(cmd);
                            delete TimeEventMsg;
                        }
                    }
                    break;
                }
                case LABSCIM_CANCEL_TIME_EVENT:
                {
                    struct labscim_cancel_time_event* cte = (struct labscim_cancel_time_event*)cmd;
                    cMessage* to_be_cancelled = GetScheduledTimeEvent(cte->cancel_sequence_number);
                    if(to_be_cancelled != nullptr)
                    {
                        mScheduledTimerMsgs.remove(to_be_cancelled);
                        cancelEvent(to_be_cancelled);
                        free(to_be_cancelled->getContextPointer());
                        delete(to_be_cancelled);
#ifdef LABSCIM_LOG_COMMANDS
                        sprintf(log,"seq%4d\tCANCEL_TIME_EVENT, event %d,OK, %s\n",hdr->sequence_number,cte->cancel_sequence_number,mNodeName);
#endif
                    }
                    else
                    {
#ifdef LABSCIM_LOG_COMMANDS
                        sprintf(log,"seq%4d\tCANCEL_TIME_EVENT, event %d, NOT FOUND, %s\n",hdr->sequence_number, cte->cancel_sequence_number, mNodeName);
#endif

                        EV_ERROR << "Nullptr on cancel time event";
                    }
                    free(cte);
                    break;
                }
                case LABSCIM_PRINT_MESSAGE:
                {
                    struct labscim_print_message* pm = (struct labscim_print_message*)cmd;
                    EV_LOG((omnetpp::LogLevel)pm->message_type, nullptr) << pm->message;
                    //std::stringstream stream;
                    //stream << pm->message;
                    //EV_DEBUG << (uint8_t*)stream.str().c_str();
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tPRINT_MESSAGE\n",hdr->sequence_number);
                    //Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
#endif
                    free(hdr);
                    break;
                }
                case LABSCIM_RADIO_COMMAND:
                {
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tRADIO_COMMAND\n",hdr->sequence_number);
#endif
                    PerformRadioCommand((struct labscim_radio_command*)cmd);
                    break;
                }
                case LABSCIM_SIGNAL_REGISTER:
                {
                    struct labscim_signal_register* reg = (struct labscim_signal_register*)cmd;
                    uint64_t signal_id = (int64_t)registerSignal((const char*)reg->signal_name);
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tLABSCIM_SIGNAL_REGISTER\n",hdr->sequence_number);
#endif
                    SendRegisterResponse(reg->hdr.sequence_number, signal_id);
                    free(cmd);
                    break;
                }
                case LABSCIM_SIGNAL_SUBSCRIBE:
                {
                    struct labscim_signal_subscribe* sub = (struct labscim_signal_subscribe*)cmd;
                    getSimulation()->getSystemModule()->subscribe(sub->signal_id, this);
                    mSubscribedSignals.push_back(sub->signal_id);
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tLABSCIM_SIGNAL_SUBSCRIBE\n",hdr->sequence_number);
#endif
                    free(cmd);
                    break;
                }
                case LABSCIM_SIGNAL_EMIT_DOUBLE:
                {
                    struct labscim_signal_emit_double* emit_signal = (struct labscim_signal_emit_double *)cmd;
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tLABSCIM_SIGNAL_EMIT_DOUBLE\n",hdr->sequence_number);
#endif
                    EV_DEBUG << "Emmiting " << getSignalName(emit_signal->signal_id) << ". Value: " << emit_signal->value;
                    emit(emit_signal->signal_id, emit_signal->value);
                    free(cmd);
                    break;
                }
                case LABSCIM_SIGNAL_EMIT_CHAR:
                {
                    struct labscim_signal_emit_char* emit_signal = (struct labscim_signal_emit_char *)cmd;
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tLABSCIM_SIGNAL_EMIT_CHAR\n",hdr->sequence_number);
#endif
                    cLabscimSignal sig(emit_signal->string,emit_signal->string_size);
                    EV_DEBUG << "Emmiting " << getSignalName(emit_signal->signal_id) << ". Value: (binary string)";
                    emit(emit_signal->signal_id, &sig);
                    free(cmd);
                    break;
                }
                case LABSCIM_GET_RANDOM:
                {
                    struct labscim_get_random* get_random = (struct labscim_get_random*)cmd;
                    union random_number resp;
                    GenerateRandomNumber(getRNG(0), get_random->distribution_type, get_random->param_1, get_random->param_2, get_random->param_3, &resp);
                    SendRandomNumber(get_random->hdr.sequence_number,resp);
                    free(cmd);
                }
                default:
                {
                    CommandsExecuted--;
                    break;
                }
                }
#ifdef LABSCIM_LOG_COMMANDS
                labscim_log(log, "pro ");
#endif
            }
        }while(cmd!=NULL);
    }
}


uint64_t LoRaMacNodeGlueMac::RegisterSignal(uint8_t* signal_name)
{
    std::string signalName((const char*)signal_name);
    uint64_t ret = registerSignal(signalName.c_str());
    //maybe append the mac as a signal prefix?
    if (std::find(mRegisteredSignals.begin(), mRegisteredSignals.end(), signalName) == mRegisteredSignals.end())
    {
        getEnvir()->addResultRecorders(this, registerSignal(signalName.c_str()), signalName.c_str(), statisticTemplate);
        mRegisteredSignals.push_back(signalName);
    }
    return ret;
}

/*
 * Binds timers to events and executes FSM.
 */
void LoRaMacNodeGlueMac::handleSelfMessage(cMessage *msg)
{
    bool WaitForCommand = true;
    mCurrentProcessingMsg=msg;

#ifdef LABSCIM_LOG_COMMANDS
        char log[128];
        log[0] = 0;
#endif

    //EV_DETAIL << "It is wakeup time." << endl;
    switch(msg->getKind())
    {
    case BOOT_MSG:
    {

        struct loramac_node_setup setup_msg;
        SHA1 hash;
        byte digest[40];
        hash.Update((const byte*)mNodeName.data(), mNodeName.size());
        hash.Final(digest);
        memcpy(setup_msg.AppKey, digest, 32);
        setup_msg.output_logs = par("OutputLogs").boolValue()?1:0;
        setup_msg.IsMaster = par("IsMaster").boolValue()?1:0;
        //EV_DETAIL << "Boot Message." << endl;
        memset(setup_msg.mac_addr, 0, sizeof(setup_msg.mac_addr));
        interfaceEntry->getMacAddress().getAddressBytes(setup_msg.mac_addr+(sizeof(setup_msg.mac_addr)-MAC_ADDRESS_SIZE));
        setup_msg.startup_time = (uint64_t)(simTime().dbl() * 1000000);
        if(gTimeReference == 0)
        {
            struct timeval tv;
            gettimeofday(&tv,NULL);
            gTimeReference = (tv.tv_sec * 1000000) + tv.tv_usec - setup_msg.startup_time;
        }
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "BOOT\n";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
        setup_msg.TimeReference = gTimeReference;
        SendProtocolBoot((void*)&setup_msg,sizeof(struct loramac_node_setup));

        delete msg;
        break;
    }
    case LORAMAC_TIMER_MSG:
    {
        //EV_DETAIL << "Timer Message." << endl;
        uint64_t us = (uint64_t)(std::round(simTime().dbl() * 1000000));
        if(msg->getContextPointer()!=nullptr)
        {
            struct labscim_set_time_event* ste = (struct labscim_set_time_event*)msg->getContextPointer();
            SendTimeEvent(ste->hdr.sequence_number, ste->time_event_id,us);
#ifdef LABSCIM_LOG_COMMANDS
            {
                char log[256];
                sprintf(log,"\tTIME_EVENT, %s, event, %d, time %f\n", mNodeName,ste->hdr.sequence_number,us);
            }
#endif
            mScheduledTimerMsgs.remove(msg);
            free(ste);
        }
        else
        {
#ifdef LABSCIM_LOG_COMMANDS
            sprintf(log,"\tTIME_EVENT, %s, nullptr time msg\n", mNodeName);
#endif
            EV_ERROR << "Nullptr on loramac timer msg";
            WaitForCommand = false;
        }
        delete msg;
        break;
    }
    case CCA_ENDED:
    {
        struct labscim_radio_command* cmd = (struct labscim_radio_command*)msg->getContextPointer();
        struct lora_is_channel_free_result ChannelFree;
        //PERFORM CCA
        ChannelFree.ChannelIsFree = (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE)?1:0;
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "CCA End: Channel is " << ChannelFree.ChannelIsFree?"Free":"Busy";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
        //radio->setRadioMode(IRadio::RADIO_MODE_SLEEP); //DO IT?
        SendRadioResponse(LORA_RADIO_IS_CHANNEL_FREE_RESULT, (uint64_t)std::round((simTime().dbl() * 1000000)),(uint8_t*)&ChannelFree, sizeof(struct lora_is_channel_free_result), cmd->hdr.sequence_number);
        mCCATimerMsg = nullptr;
        free(cmd);
        delete msg;
        break;
    }
    case RX_TIMER:
    {
        WaitForCommand = false;
        switch(mRX_fsm)
        {
        case RX_IDLE:
        {
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            break;
        }
        case RX_SINGLE_START:
        {
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            mRX_fsm = RX_SINGLE_LISTENING;
            if(mRx_window_us > 0)
            {
                scheduleAt(simTime().dbl() + ((double)mRx_window_us)/1000, mRXTimerMsg);
            }
            break;
        }
        case RX_SINGLE_LISTENING:
        {
            struct lora_set_rx RxResult;
            if(radio->getReceptionState()==IRadio::RECEPTION_STATE_RECEIVING)
            {
                //reschedule timeout
                scheduleAt(simTime().dbl() + ((double)mRx_window_us)/1000, mRXTimerMsg);
            }
            else
            {
                //timeout
                RxResult.Timeout_us = mRx_window_us;
                //rx timeout
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                mRX_fsm = RX_IDLE;
                SendRadioResponse(LORA_RADIO_RX_TIMEOUT, (uint64_t)std::round((simTime().dbl() * 1000000)),(uint8_t*)&RxResult, sizeof(struct lora_set_rx), 0);
                WaitForCommand = true;
            }
            break;
        }
        case RX_CONTINUOUS_START:
        {
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            mRX_fsm = RX_CONTINUOUS_LISTENING;
            break;
        }
        case RX_CONTINUOUS_LISTENING:
        {
            if(radio->getRadioMode()!=IRadio::RADIO_MODE_RECEIVER)
            {
                //no timer msgs should be received here, but we set radio to receiver just in case
                radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            }
            break;
        }
        case RX_SLEEP_WINDOW:
        case RX_WINDOW_START:
        {
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            mRX_fsm = RX_WINDOW_LISTENING;
            scheduleAt(simTime().dbl() + ((double)mRx_window_us)/1000, mRXTimerMsg);
            break;
        }
        case RX_WINDOW_LISTENING:
        {
            if(radio->getReceptionState()==IRadio::RECEPTION_STATE_RECEIVING)
            {
                //reschedule

                scheduleAt(simTime().dbl() + ((double)mRx_window_us)/1000, mRXTimerMsg);
            }
            else
            {
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                mRX_fsm = RX_SLEEP_WINDOW;
                scheduleAt(simTime().dbl() + ((double)mSleep_window_us)/1000, mRXTimerMsg);
            }
            break;
        }
        default:
        {
            break;
        }
        }
        break;
    }
    default:
    {
        WaitForCommand = false;
        delete msg;
        break;
    }
    }
#ifdef LABSCIM_LOG_COMMANDS
    if(log[0]!=0)
    {
        labscim_log(log, "pro ");
    }
#endif

    if(WaitForCommand)
    {
        ProcessCommands();
    }
    mCurrentProcessingMsg=nullptr;
}

void LoRaMacNodeGlueMac::handleLowerPacket(Packet *packet)
{
    if (packet->hasBitError()) {
        EV << "Received " << packet << " contains bit errors or collision, dropping it\n";
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }
    struct lora_radio_payload* payload;
    uint64_t message_size = packet->getByteLength();
    payload = (struct lora_radio_payload*)malloc(FIXED_SIZEOF_LORA_RADIO_PAYLOAD + message_size);
    if(payload==NULL)
    {
        //dammit, something very wrong
        delete packet;
        return;
    }
    payload->MessageSize_bytes = message_size;
    payload->RX_timestamp_us = (uint64_t)(std::round(packet->getArrivalTime().dbl() * 1000000));
    payload->CRCError = 0;

    if (packet->findTag<LoRaParamsInd>() != nullptr) {
        auto lpi = packet->getTag<LoRaParamsInd>();
        payload->LoRaCR = lpi->getLoRaCR();
        payload->LoRaSF = lpi->getLoRaSF();
    }
    else
    {
        payload->LoRaCR = 0;
        payload->LoRaSF = 0;
    }

    if (packet->findTag<SignalBandInd>() != nullptr) {
        auto signalBandInd = packet->getTag<SignalBandInd>();
        payload->CenterFrequency_Hz = signalBandInd->getCenterFrequency().get();
        payload->LoRaBandwidth_Hz = signalBandInd->getBandwidth().get();
    }
    else
    {
        payload->CenterFrequency_Hz = 0;
        payload->LoRaBandwidth_Hz = 0;
    }
    if (packet->findTag<SignalPowerInd>() != nullptr) {
        auto signalPowerInd = packet->getTag<SignalPowerInd>();
        payload->RSSI_dbm = math::mW2dBmW(signalPowerInd->getPower().get()*1000);
    }
    else
    {
        payload->RSSI_dbm = -200.0;
    }

    const auto& data = packet->popAtFront<BytesChunk>();
    data->copyToBuffer(payload->Message, message_size);

    if (packet->findTag<SnirInd>() != nullptr) {
        auto snir = packet->getTag<SnirInd>();
        float snir_db = math::fraction2dB(snir->getAverageSnir());
        payload->SNR_db = snir_db;
    }
    else
    {
        payload->SNR_db = -200.0;
    }
    delete packet;
#ifdef LABSCIM_LOG_COMMANDS
    std::stringstream stream;
    stream << "Received " << message_size << "bytes. Sending to upper layers\n";
    Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
    EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
    switch(mRX_fsm)
    {
    case RX_CONTINUOUS_START:
    case RX_CONTINUOUS_LISTENING:
    {
        if(radio->getRadioMode()!=IRadio::RADIO_MODE_RECEIVER)
        {
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        }
        mRX_fsm = RX_CONTINUOUS_LISTENING;
        break;
    }
    case RX_WINDOW_START:
    case RX_WINDOW_LISTENING:
    default:
    {
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        mRX_fsm = RX_IDLE;
        cancelEvent(mRXTimerMsg);
        break;
    }
    }
    //dispatch packet to LoRaMAC upper layers
    SendRadioResponse(LORA_RADIO_PACKET_RECEIVED, (uint64_t)std::round((simTime().dbl() * 1000000)),(void*)payload, FIXED_SIZEOF_LORA_RADIO_PAYLOAD + message_size, 0);
    free(payload);
    ProcessCommands();
}

void LoRaMacNodeGlueMac::receiveSignal(cComponent *source, simsignal_t signalID, double value, cObject *details)
{
    if(signalID == labscim::physicallayer::LoRaRadio::loraradio_datarate_changed)
    {
        interfaceEntry->setDatarate(value);
    }
}

void LoRaMacNodeGlueMac::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method_Silent();
    MacProtocolBase::receiveSignal(source, signalID, obj, details);
    if(std::find(mSubscribedSignals.begin(), mSubscribedSignals.end(), signalID) != mSubscribedSignals.end())
    {
        cLabscimSignal* sig = dynamic_cast<cLabscimSignal*>(obj);
        if(sig)
        {
            char Msg[256];
            sig->getMessage(Msg,256);
            SendSignal(signalID, (uint64_t)std::round((simTime().dbl() * 1000000)), Msg, sig->getMessageSize()<256?sig->getMessageSize():256);
        }
        ProcessCommands();
    }
}


void LoRaMacNodeGlueMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
        if (mTransmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
        {
            if(mTransmitRequestSeqNo!=0)
            {
                struct lora_radio_status response;
                response.RadioMode = value; //nothing is returned by now
                response.ChannelIsFree = (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE)?1:0;
                SendRadioResponse(LORA_RADIO_SEND_COMPLETED, (uint64_t)std::round((simTime().dbl() * 1000000)),(uint8_t*)&response, sizeof(struct lora_radio_status), mTransmitRequestSeqNo);
#ifdef LABSCIM_LOG_COMMANDS
                std::stringstream stream;
                stream << "TX is over\n";
                Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
                EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
                mTransmitRequestSeqNo = 0;
                //radio must be kept listening
                //configureRadio(mCurrentCenterFrequency,mCurrentBandwidth, mCurrentPower, mCurrentBitrate, IRadio::RADIO_MODE_RECEIVER);
                ProcessCommands();
            }
            //maybe we just return when over?
        }
        mTransmissionState = newRadioTransmissionState;
    }
    else if(signalID == IRadio::receptionStateChangedSignal)
    {
        IRadio::ReceptionState newRadioReceptionState = static_cast<IRadio::ReceptionState>(value);

        if(newRadioReceptionState == IRadio::RECEPTION_STATE_RECEIVING)
        {
            struct lora_radio_status response;
            response.RadioMode = (uint32_t)value;
            response.ChannelIsFree = (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE)?1:0;
            if(mRX_fsm==RX_WINDOW_LISTENING)
            {
                cancelEvent(mRXTimerMsg);
                scheduleAt(simTime().dbl() + ((double)(2*mRx_window_us+mSleep_window_us))/1000, mRXTimerMsg);
            }

            SendRadioResponse(LORA_RADIO_STATE_CHANGED, (uint64_t)std::round((simTime().dbl() * 1000000)),(uint8_t*)&response, sizeof(struct lora_radio_status), 0);
#ifdef LABSCIM_LOG_COMMANDS
            std::stringstream stream;
            stream << "RX is starting\n";
            Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
            EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
            ProcessCommands();
        }
    }
    else if (signalID == IRadio::radioModeChangedSignal)
    {
        IRadio::RadioMode newRadioMode = static_cast<IRadio::RadioMode>(value);
        if(mLastRadioMode != newRadioMode)
        {
            mRadioModeTimes[mLastRadioMode] += simTime() - mLastRadioModeSwitch;
            emit(mRadioModeTimesSignals[mLastRadioMode], mRadioModeTimes[mLastRadioMode]);
            mLastRadioModeSwitch = simTime();
            mLastRadioMode = newRadioMode;
        }
    }
}

} // namespace contikitsch

