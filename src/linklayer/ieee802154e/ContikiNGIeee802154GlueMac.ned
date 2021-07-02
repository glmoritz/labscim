//***************************************************************************
//* file:        Ieee802154Mac.ned
//*
//* author:      Jerome Rousselot, Marc Loebbers
//*
//* copyright:   (C) 2007-2009 CSEM SA, Neuchatel, Switzerland.
//*                  (C) 2004 Telecommunication Networks Group (TKN) at
//*              Technische Universitaet Berlin, Germany.
//*
//*              This program is free software; you can redistribute it
//*              and/or modify it under the terms of the GNU General Public
//*              License as published by the Free Software Foundation; either
//*              version 2 of the License, or (at your option) any later
//*              version.
//*              For further information see file COPYING
//*              in the top level directory
//*
//* Funding: This work was partially financed by the European Commission under the
//* Framework 6 IST Project "Wirelessly Accessible Sensor Populations"
//* (WASP) under contract IST-034963.
//***************************************************************************
//* part of:    Modifications to the MF-2 framework by CSEM
//**************************************************************************/

package labscim.linklayer.ieee802154e;
import inet.queueing.contract.IPacketQueue;
import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;

@namespace(tsch);

//
// Generic CSMA protocol supporting Mac-ACKs as well as
// constant, linear or exponential backoff times.
//
module ContikiNGIeee802154GlueMac extends MacProtocolBase like IMacProtocol
{
    parameters:
        int SocketBufferSize = default(512); //the buffer size for socket connection with contiking instance
        
        volatile double BootTime @unit(s) = default(0); //time to boot up the node 
                
        string address = default("auto"); // MAC address as hex string (12 hex digits), or
                                          // "auto". "auto" values will be replaced by
                                          // a generated MAC address in init stage 0.
        
        string NodeProcessSshAddress = default("::1"); // SSH address of the machine in which the upper layer process will be executed
		string NodeProcessCommand; // Command to be executed in the remote host to spawn the upper layer process                                           
		string NodeExtraArguments = default(""); // Extra arguments for the node
		bool NodeDebug = default(false); // Command to be executed in the remote host to spawn the upper layer process
		bool OutputLogs = default(false); // If true, contiki will write its printfs to a file
		bool TSCHCoordinator = default(false); // If true, contiki node will be a TSCH coordinator
		int NodeProcessConnectionPort = default(9608);//The default TCP port that will be used for connection                                          
                                          
        // length of MAC header
        int headerLength @unit(b) = default(72 b);
        int mtu @unit(B) = default(0B);
        // bit rate
        double bitrate @unit(bps) = default(50000 bps); //1 bit per symbol

        // Clear Channel Assessment detection time
        double ccaDetectionTime @unit(s) = default(0.000160 s); // 8 symbols @ 50k baud (Table 11-1 and 20-1 from 802.15.4g std)
        // Time to setup radio to reception state
        double rxSetupTime @unit(s) = default(0 s);
        // Time to switch radio from Rx to Tx state
        double aTurnaroundTime @unit(s) = default(0 s);//default(0.000240 s);    //Section 20.6.9 - Table 11-1 -> 12 symbols @ 50k baud
        
        string radioModule = default("^.radio");   // The path to the Radio module  //FIXME remove default value               
        @class(ContikiNGIeee802154GlueMac);        
        
        @statisticTemplate[nbStats](record=histogram,vector?; interpolationmode=none);
    submodules:
        queue: <default("DropTailQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(100);
                @display("p=100,100;q=l2queue");
        }
    connections allowunconnected:
}

