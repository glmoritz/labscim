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

#include "ContikiNGIeee802154GlueMac.h"
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
#include "../../common/labscim-contiki-radio-protocol.h"
#include "../../common/labscim_log.h"
#include "../../common/cLabscimSignal.h"



using namespace inet::physicallayer;
using namespace omnetpp;
using namespace inet;





namespace labscim {

Define_Module(ContikiNGIeee802154GlueMac);




void ContikiNGIeee802154GlueMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        headerLength = par("headerLength");
        ccaDetectionTime = par("ccaDetectionTime");
        rxSetupTime = par("rxSetupTime");
        aTurnaroundTime = par("aTurnaroundTime");
        mCurrentBitrate = bps(par("bitrate"));
        mCurrentMode = IRadio::RADIO_MODE_OFF;
        statisticTemplate = getProperties()->get("statisticTemplate", "nbStats");

    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

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

        EV_DETAIL << " bitrate = " << mCurrentBitrate.get() << endl;

        EV_DETAIL << "Finished Contiki Glue MAC init stage 1." << endl;
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
        BootMsg = new cMessage((mNodeName + "-boot").c_str());
        BootMsg->setKind(BOOT_MSG);
        scheduleAt(boot_time, BootMsg);
    }
}

void ContikiNGIeee802154GlueMac::finish()
{

}

ContikiNGIeee802154GlueMac::~ContikiNGIeee802154GlueMac()
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
}

void ContikiNGIeee802154GlueMac::configureInterfaceEntry()
{
    MacAddress address = parseMacAddressParameter(par("address"));

    // data rate
    interfaceEntry->setDatarate(mCurrentBitrate.get());

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
void ContikiNGIeee802154GlueMac::handleUpperPacket(Packet *packet)
{
    //we just ignore any upper packet
    delete packet;
}


void ContikiNGIeee802154GlueMac::attachSignal(Packet *mac, simtime_t_cref startTime)
{
    simtime_t duration = mac->getBitLength() / mCurrentBitrate.get();
    mac->setDuration(duration);
}

#define DOT_15_4G_CHAN0_FREQUENCY 902200
#define DOT_15_4G_CHANNEL_SPACING    200

void ContikiNGIeee802154GlueMac::configureRadio(Hz CenterFrequency, Hz Bandwidth, W Power, bps Bitrate, int mode /*= -1*/)
{
    auto configureCommand = new ConfigureRadioCommand();
    auto request = new Message("changeChannel", RADIO_C_CONFIGURE);

    if(
            (mCurrentMode != mode) ||                       \
            (mCurrentCenterFrequency != CenterFrequency) || \
            (mCurrentBandwidth != Bandwidth) ||             \
            (mCurrentPower != Power) ||                     \
            (mCurrentBitrate != Bitrate)                    \
    )
    {
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "Radio CH " << (float)(((float)(CenterFrequency.get()/1000) -(float)DOT_15_4G_CHAN0_FREQUENCY)/(float)DOT_15_4G_CHANNEL_SPACING) << " at " << CenterFrequency  << ", " << Bitrate << " @ " << Bandwidth << " BW " << Power << " tx. Radio mode: ";
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
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        configureCommand->setPower(Power);
        configureCommand->setBitrate(Bitrate);
        configureCommand->setCenterFrequency(CenterFrequency);
        configureCommand->setBandwidth(Bandwidth);
        configureCommand->setRadioMode(mode);
        request->setControlInfo(configureCommand);
        mCurrentMode = mode;
        mCurrentCenterFrequency = CenterFrequency;
        mCurrentBandwidth = Bandwidth;
        mCurrentPower = Power;
        mCurrentBitrate = Bitrate;
        sendDown(request);
    }
#ifdef LABSCIM_LOG_COMMANDS
    else
    {
        std::stringstream stream;
        stream << "Redundant configure radio command\n";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
    }
#endif
}


void ContikiNGIeee802154GlueMac::PerformRadioCommand(struct labscim_radio_command* cmd)
{

    switch(cmd->radio_command)
    {
    case CONTIKI_RADIO_SETUP:
    {
        //802.15.4g std sec 20.6.4
        //Channel switch time shall be less than or equal to 500 μs. The channel switch time is defined as the time
        //elapsed when changing to a new channel, including any required settling time.
        //... but for now we set that at 0us
        struct contiki_radio_setup* setup = (struct contiki_radio_setup*)cmd->radio_struct;
        configureRadio(Hz(setup->Frequency_Hz), Hz(setup->Bandwidth_Hz), mW(math::dBmW2mW(setup->Power_dbm)), bps(setup->Bitrate_bps), mCurrentMode);
        if(!mRadioConfigured)
        {
            cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
            mRadioConfigured = true;
            radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        }
        free(cmd);
        break;
    }
    case CONTIKI_RADIO_SET_MODE:
    {
        struct contiki_radio_mode* mode = (struct contiki_radio_mode*)cmd->radio_struct;
        configureRadio(mCurrentCenterFrequency, mCurrentBandwidth, mCurrentPower, mCurrentBitrate, mode->RadioMode);
        free(cmd);
        break;
    }
    case CONTIKI_RADIO_SEND:
    {
        struct contiki_radio_payload* payload = (struct contiki_radio_payload*)cmd->radio_struct;
        char msgname[20];
        sprintf(msgname, "packet-node-%d", getIndex());
        auto cmsg = new Packet(msgname);
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
    case CONTIKI_RADIO_PERFORM_CCA:
    {
        if(mCCATimerMsg == nullptr)
        {
            cMessage* CCAMsg;
            double us = simTime().dbl() * 1000000;
            CCAMsg = new cMessage((mNodeName + "-cca").c_str());
            CCAMsg->setKind(CCA_ENDED);
            CCAMsg->setContextPointer((void*)cmd);
            mCCATimerMsg = CCAMsg;
            scheduleAt(us + rxSetupTime + ccaDetectionTime, CCAMsg);
#ifdef LABSCIM_LOG_COMMANDS
            std::stringstream stream;
            stream << "CCA Start\n";
            Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
            EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
            configureRadio(mCurrentCenterFrequency, mCurrentBandwidth, mCurrentPower, mCurrentBitrate, IRadio::RADIO_MODE_RECEIVER);
        }
    }
    case CONTIKI_RADIO_GET_STATE:
    {
        struct contiki_radio_state state;
        state.State = (uint32_t)radio->getReceptionState();
        SendRadioResponse(CONTIKI_RADIO_STATE_RESULT, (uint64_t)round((simTime().dbl() * 1000000)),(uint8_t*)&state, sizeof(struct contiki_radio_state), cmd->hdr.sequence_number);
        free(cmd);
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "Radio get state: ";
        switch(state.State)
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
    default:
    {
        free(cmd);
        break;
    }
    }
}

cMessage* ContikiNGIeee802154GlueMac::GetScheduledTimeEvent(uint32_t sequence_number)
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
    return nullptr;
}

void ContikiNGIeee802154GlueMac::ProcessCommands()
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
                    TimeEventMsg->setKind(CONTIKI_TIMER_MSG);
                    TimeEventMsg->setContextPointer((void*)ste);
                    if(ste->is_relative)
                    {
#ifdef LABSCIM_LOG_COMMANDS
                        sprintf(log,"seq%4d\tSET_TIME_EVENT\n",hdr->sequence_number);
#endif
                        scheduleAt((us + (double)ste->time_us)/1000000, TimeEventMsg);
                        mScheduledTimerMsgs.push_front(TimeEventMsg);
                    }
                    else
                    {
                        if(ste->time_us >= us)
                        {
#ifdef LABSCIM_LOG_COMMANDS
                            sprintf(log,"seq%4d\tSET_TIME_EVENT\n",hdr->sequence_number);
#endif
                            scheduleAt((double)ste->time_us/1000000, TimeEventMsg);
                            mScheduledTimerMsgs.push_front(TimeEventMsg);
                        }
                        else
                        {
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
                        sprintf(log,"seq%4d\tCANCEL_TIME_EVENT\n",hdr->sequence_number);
#endif
                    }
                    free(cte);
                    break;
                }
                case LABSCIM_PRINT_MESSAGE:
                {
                    struct labscim_print_message* pm = (struct labscim_print_message*)cmd;
                    EV_LOG((omnetpp::LogLevel)pm->message_type, nullptr) << pm->message;
                    std::stringstream stream;
                    stream << pm->message;
                    EV_DEBUG << (uint8_t*)stream.str().c_str();
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tPRINT_MESSAGE\n",hdr->sequence_number);
                    Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
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


uint64_t ContikiNGIeee802154GlueMac::RegisterSignal(uint8_t* signal_name)
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
void ContikiNGIeee802154GlueMac::handleSelfMessage(cMessage *msg)
{
    bool WaitForCommand = true;

    //EV_DETAIL << "It is wakeup time." << endl;
    switch(msg->getKind())
    {
    case BOOT_MSG:
    {
        struct contiki_node_setup setup_msg;
        setup_msg.output_logs = (uint8_t)par("OutputLogs").boolValue();
        setup_msg.tsch_coordinator = (uint8_t)par("TSCHCoordinator").boolValue()?1:0;

        //EV_DETAIL << "Boot Message." << endl;
        memset(setup_msg.mac_addr, 0, sizeof(setup_msg.mac_addr));
        interfaceEntry->getMacAddress().getAddressBytes(setup_msg.mac_addr+(sizeof(setup_msg.mac_addr)-MAC_ADDRESS_SIZE));
        setup_msg.startup_time = (uint64_t)(simTime().dbl() * 1000000);
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "BOOT\n";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
        SendProtocolBoot((void*)&setup_msg,sizeof(struct contiki_node_setup));
        break;
    }
    case CONTIKI_TIMER_MSG:
    {
        //EV_DETAIL << "Timer Message." << endl;
        uint64_t us = (uint64_t)(round(simTime().dbl() * 1000000));
        if(msg->getContextPointer()!=nullptr)
        {
            struct labscim_set_time_event* ste = (struct labscim_set_time_event*)msg->getContextPointer();
            SendTimeEvent(ste->hdr.sequence_number, ste->time_event_id,us);
            mScheduledTimerMsgs.remove(msg);
            free(ste);
        }
        else
        {
            WaitForCommand = false;
        }
        break;
    }
    case CCA_ENDED:
    {
        struct labscim_radio_command* cmd = (struct labscim_radio_command*)msg->getContextPointer();
        struct contiki_radio_cca ChannelFree;
        //PERFORM CCA
        ChannelFree.ChannelIsFree = (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE)?1:0;
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "CCA End: Channel is " << ChannelFree.ChannelIsFree?"Free":"Busy";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
        //radio->setRadioMode(IRadio::RADIO_MODE_SLEEP); //DO IT?
        SendRadioResponse(CONTIKI_RADIO_CCA_RESULT, (uint64_t)round((simTime().dbl() * 1000000)),(uint8_t*)&ChannelFree, sizeof(struct contiki_radio_cca), cmd->hdr.sequence_number);
        mCCATimerMsg = nullptr;
        free(cmd);
        break;
    }
    default:
    {
        WaitForCommand = false;
        break;
    }
    }
    delete msg;

    if(WaitForCommand)
    {
        ProcessCommands();
    }
}


/**
 * We may flood our nodes, but all received packets must be sent to them
 */
void ContikiNGIeee802154GlueMac::handleLowerPacket(Packet *packet)
{
    if (packet->hasBitError()) {
        EV << "Received " << packet << " contains bit errors or collision, dropping it\n";
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }
    struct contiki_radio_payload* payload;
    uint64_t message_size = packet->getByteLength();
    payload = (struct contiki_radio_payload*)malloc(FIXED_SIZEOF_CONTIKI_RADIO_PAYLOAD + message_size);
    if(payload==NULL)
    {
        //dammit, something very wrong
        delete packet;
        return;
    }
    payload->MessageSize_bytes = message_size;
    payload->RX_timestamp_us = (uint64_t)(round(packet->getArrivalTime().dbl() * 1000000));

    if (packet->findTag<SignalPowerInd>() != nullptr) {
        auto signalPowerInd = packet->getTag<SignalPowerInd>();
        payload->RSSI_dbm_x100 = (uint32_t)round(100*math::mW2dBmW(signalPowerInd->getPower().get()*1000));
    }
    else
    {
        payload->RSSI_dbm_x100 = 0;
    }

    const auto& data = packet->popAtFront<BytesChunk>();
    data->copyToBuffer(payload->Message, message_size);

    if (packet->findTag<SnirInd>() != nullptr) {
        auto snir = packet->getTag<SnirInd>();
        double snir_dbm = math::fraction2dB(snir->getAverageSnir());

//      802.15.4g - sec 10.2.6
//        The LQI measurement is a characterization of the strength and/or quality of a received packet. The
//        measurement may be implemented using receiver ED, a signal-to-noise ratio estimation, or a combination of
//        these methods. The use of the LQI result by the network or application layers is not specified in 1this
//        standard.
//        The LQI measurement shall be performed for each received packet. The minimum and maximum LQI
//        values (0x00 and 0xff) should be associated with the lowest and highest quality compliant signals detectable
//        by the receiver, and LQI values in between should be uniformly distributed between these two limits. At
//        least eight unique values of LQI shall be used.

        //0dbm is maximum LQI, -90dbm minimum
        if(snir_dbm < 0)
        {
            snir_dbm=0;
        }
        payload->LQI = (uint32_t)round(((snir_dbm)*255.0)/35.0);
        if(payload->LQI > 255)
        {
            payload->LQI = 255;
        }
    }
    else
    {
        payload->LQI = 0;
    }
    delete packet;
#ifdef LABSCIM_LOG_COMMANDS
    std::stringstream stream;
    stream << "Received " << message_size << "bytes. Sending to upper layers\n";
    Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
    EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
    //dispatch packet to contiki upper layers
    SendRadioResponse(CONTIKI_RADIO_PACKET_RECEIVED, (uint64_t)round((simTime().dbl() * 1000000)),(void*)payload, FIXED_SIZEOF_CONTIKI_RADIO_PAYLOAD + message_size, 0);

    free(payload);

    ProcessCommands();
}

void ContikiNGIeee802154GlueMac::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
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
            SendSignal(signalID, (uint64_t)round((simTime().dbl() * 1000000)), Msg, sig->getMessageSize()<256?sig->getMessageSize():256);
        }
        ProcessCommands();
    }
}


void ContikiNGIeee802154GlueMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = static_cast<IRadio::TransmissionState>(value);
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
        {
            if(mTransmitRequestSeqNo!=0)
            {
                struct contiki_radio_send_response response;
                response.ResponseCode = 1; //nothing is returned by now
                SendRadioResponse(CONTIKI_RADIO_SEND_COMPLETED, (uint64_t)round((simTime().dbl() * 1000000)),(uint8_t*)&response, sizeof(struct contiki_radio_send_response), mTransmitRequestSeqNo);
#ifdef LABSCIM_LOG_COMMANDS
                std::stringstream stream;
                stream << "TX is over\n";
                Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
                EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
                mTransmitRequestSeqNo = 0;
                mCurrentMode = radio->getReceptionState();
                //radio must be kept listening
                configureRadio(mCurrentCenterFrequency,mCurrentBandwidth, mCurrentPower, mCurrentBitrate, IRadio::RADIO_MODE_RECEIVER);
                ProcessCommands();
            }
            //maybe we just return when over?
        }
        transmissionState = newRadioTransmissionState;
    }
    else if(signalID == IRadio::receptionStateChangedSignal)
    {
        IRadio::ReceptionState newRadioReceptionState = static_cast<IRadio::ReceptionState>(value);

        if(newRadioReceptionState == IRadio::RECEPTION_STATE_RECEIVING)
        {
            struct contiki_radio_state response;
            response.State = (uint32_t)value;
            SendRadioResponse(CONTIKI_RADIO_STATE_CHANGED, (uint64_t)round((simTime().dbl() * 1000000)),(uint8_t*)&response, sizeof(struct contiki_radio_state), 0);
#ifdef LABSCIM_LOG_COMMANDS
            std::stringstream stream;
            stream << "RX is starting\n";
            Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
            EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
            ProcessCommands();
        }
    }
}

} // namespace contikitsch

