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

#include "PacketForwarderNodeGlueMac.h"
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
#include "../../common/lora_gateway_setup.h"
#include "../../common/labscim_log.h"
#include "../../common/labscim_socket.h"
#include "../../common/sx126x_labscim.h"
#include "../../physicallayer/lora/packetlevel/LoRaTags_m.h"
#include "../../physicallayer/lora/packetlevel/LoRaRadioControlInfo_m.h"
#include "../../physicallayer/lora/packetlevel/LoRaDimensionalTransmitter.h"
#include "../../physicallayer/lora/packetlevel/LoRaDimensionalReceiver.h"
#include "../../physicallayer/lora/packetlevel/LoRaRadio.h"

using namespace inet::physicallayer;
using namespace omnetpp;
using namespace inet;

namespace labscim {

Define_Module(PacketForwarderNodeGlueMac);



void PacketForwarderNodeGlueMac::initialize(int stage)
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
        mTransmissionState == radio->getTransmissionState();



        EV_DETAIL << "Finished Packet Forwarder Glue MAC init stage 1." << endl;
    }
    else if (stage == INITSTAGE_NETWORK_CONFIGURATION)
    {
        double boot_time = par("BootTime").doubleValue();

        uint32_t ServerPort = par("NodeProcessConnectionPort").intValue();
        char msgname[64];
        cMessage* BootMsg;
        std::string cmd("");
        std::stringstream stream;
        stream << "gateway-node-" << std::hex << interfaceEntry->getMacAddress().getInt();
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

        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
    }
}

void PacketForwarderNodeGlueMac::finish()
{

}

PacketForwarderNodeGlueMac::~PacketForwarderNodeGlueMac()
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

void PacketForwarderNodeGlueMac::configureInterfaceEntry()
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
void PacketForwarderNodeGlueMac::handleUpperPacket(Packet *packet)
{
    //we just ignore any upper packet
    delete packet;
}

void PacketForwarderNodeGlueMac::attachSignal(Packet *mac, simtime_t_cref startTime)
{
    mac->setDuration(mLoRaRadio->getPacketRadioTimeOnAir(mac));
}

void PacketForwarderNodeGlueMac::configureRadio(ConfigureLoRaRadioCommand* config_msg)
{
    auto request = new Message("ConfigureRadio", RADIO_C_CONFIGURE);
    request->setControlInfo(config_msg);
    sendDown(request);
}


void PacketForwarderNodeGlueMac::configureRadio(Hz CenterFrequency, Hz Bandwidth, W Power, bps Bitrate, int mode /*= -1*/)
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


void PacketForwarderNodeGlueMac::PerformRadioCommand(struct labscim_radio_command* cmd)
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

        auto loraparameters = cmsg->addTagIfAbsent<LoRaParamsReq>();
        loraparameters->setLoRaSF(payload->LoRaSF);
        loraparameters->setLoRaCR(payload->LoRaCR);

        auto bandparameters = cmsg->addTagIfAbsent<SignalBandReq>();
        bandparameters->setBandwidth(Hz(payload->LoRaBandwidth_Hz));
        bandparameters->setCenterFrequency(Hz(payload->CenterFrequency_Hz));

        auto powerparameters = cmsg->addTagIfAbsent<SignalPowerReq>();
        powerparameters->setPower(mW(dBmW2mW(payload->TxPower_dbm)));

        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
        attachSignal(cmsg, simTime() + aTurnaroundTime + (((float)(payload->Tx_Delay_us))/1e6));
        mTransmitRequestSeqNo = cmd->hdr.sequence_number;

        // give time for the radio to be in Tx state before transmitting
        sendDelayed(cmsg, aTurnaroundTime+(((float)(payload->Tx_Delay_us))/1e6), lowerLayerOutGateId);

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
        configureCommand->setBandwidth(Hz(mp->ModulationParams.Params.LoRa.Bandwidth));
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
        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        free(cmd);
        break;
    }
    case LORA_RADIO_SET_IDLE:
    {
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
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

cMessage* PacketForwarderNodeGlueMac::GetScheduledTimeEvent(uint32_t sequence_number)
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

void PacketForwarderNodeGlueMac::ProcessCommands()
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
                case LABSCIM_SIGNAL_EMIT:
                {
                    struct labscim_signal_emit* emit_signal = (struct labscim_signal_emit*)cmd;
#ifdef LABSCIM_LOG_COMMANDS
                    sprintf(log,"seq%4d\tLABSCIM_SIGNAL_EMIT\n",hdr->sequence_number);
#endif
                    EV_DEBUG << "Emmiting " << getSignalName(emit_signal->signal_id) << ". Value: " << emit_signal->value;
                    emit(emit_signal->signal_id, emit_signal->value);
                    free(cmd);
                    break;
                }
                case LABSCIM_GET_RANDOM:
                {
                    struct labscim_get_random* get_random = (struct labscim_get_random*)cmd;
                    union random_number resp;
                    GenerateRandomNumber(getRNG(0), get_random->distribution_type, get_random->param_1, get_random->param_2, get_random->param_3, &resp);
                    SendRandomNumber(get_random->hdr.sequence_number,resp);
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


uint64_t PacketForwarderNodeGlueMac::RegisterSignal(uint8_t* signal_name)
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
void PacketForwarderNodeGlueMac::handleSelfMessage(cMessage *msg)
{
    bool WaitForCommand = true;
    mCurrentProcessingMsg=msg;

    //EV_DETAIL << "It is wakeup time." << endl;
    switch(msg->getKind())
    {
    case BOOT_MSG:
    {
        struct lora_gateway_setup setup_msg;
        setup_msg.output_logs = par("OutputLogs").boolValue()?1:0;
        //EV_DETAIL << "Boot Message." << endl;
        memset(setup_msg.mac_addr, 0, sizeof(setup_msg.mac_addr));
        interfaceEntry->getMacAddress().getAddressBytes(setup_msg.mac_addr+(sizeof(setup_msg.mac_addr)-MAC_ADDRESS_SIZE));
        setup_msg.startup_time = (uint64_t)(simTime().dbl() * 1000000);
        setup_msg.labscim_log_master = par("IsMQTTLogger").boolValue()?1:0;
#ifdef LABSCIM_LOG_COMMANDS
        std::stringstream stream;
        stream << "BOOT\n";
        Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
        EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
        SendProtocolBoot((void*)&setup_msg,sizeof(struct lora_gateway_setup));
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
            mScheduledTimerMsgs.remove(msg);
            free(ste);
        }
        else
        {
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
    default:
    {
        WaitForCommand = false;
        delete msg;
        break;
    }
    }

    if(WaitForCommand)
    {
        ProcessCommands();
    }
    mCurrentProcessingMsg=nullptr;
}

void PacketForwarderNodeGlueMac::handleLowerPacket(Packet *packet)
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

    if (packet->findTag<LoRaParamsInd>() != nullptr)
    {
        auto loraind = packet->getTag<LoRaParamsInd>();
        payload->LoRaSF = loraind->getLoRaSF();
        payload->LoRaCR = loraind->getLoRaCR();
    }
    else
    {
        //something wrong
        payload->LoRaSF = 0;
        payload->LoRaCR = 0;
    }


    if (packet->findTag<SignalBandInd>() != nullptr)
    {
        auto bandind = packet->getTag<SignalBandInd>();
        payload->CenterFrequency_Hz = (uint32_t)bandind->getCenterFrequency().get();
        payload->LoRaBandwidth_Hz = (uint32_t)bandind->getBandwidth().get();
    }
    else
    {
        //something wrong
        payload->CenterFrequency_Hz = 0;
        payload->LoRaBandwidth_Hz = 0;
    }

    delete packet;
#ifdef LABSCIM_LOG_COMMANDS
    std::stringstream stream;
    stream << "Received " << message_size << "bytes. Sending to upper layers\n";
    Node_Log(simTime().dbl(), getId(), (uint8_t*)stream.str().c_str());
    EV_DEBUG << (uint8_t*)stream.str().c_str();
#endif
    if(radio->getRadioMode()!=IRadio::RADIO_MODE_RECEIVER)
    {
        // lora gateway radio is always listening
        radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    }




    //dispatch packet to LoRaMAC upper layers
    SendRadioResponse(LORA_RADIO_PACKET_RECEIVED, (uint64_t)std::round((simTime().dbl() * 1000000)),(void*)payload, FIXED_SIZEOF_LORA_RADIO_PAYLOAD + message_size, 0);
    free(payload);
    ProcessCommands();
}

void PacketForwarderNodeGlueMac::receiveSignal(cComponent *source, simsignal_t signalID, double value, cObject *details)
{
    if(signalID == labscim::physicallayer::LoRaRadio::loraradio_datarate_changed)
    {
        interfaceEntry->setDatarate(value);
    }
}

void PacketForwarderNodeGlueMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
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
                radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
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
}

} // namespace contikitsch

