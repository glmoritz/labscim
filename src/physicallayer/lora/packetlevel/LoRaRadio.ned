//
// Copyright (C) 2014 Florian Meier
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

package labscim.physicallayer.lora.packetlevel;

import inet.physicallayer.base.packetlevel.FlatRadioBase;

module LoRaRadio extends FlatRadioBase
{
    parameters:
        int LoRaSF;
        int LoRaCR;
        bool iAmGateway;
        @signal[loraradio_datarate_changed](type=double);                
        
        @statistic[meanSnirdB](title="Mean SNIR dB"; source=meanSnirdB(packetSentToUpper); record=histogram,vector);        
        @statistic[rxSF](title="SF from received packets (SF)"; source=LoRaSFFromReception(receptionStarted); record=histogram,vector);
        @statistic[rxDR](title="LoRa AU915 Datarate from received packets"; source=LoRaDRAU915FromReception(receptionStarted); record=histogram,vector);
        @statistic[rxBandwidth](title="Bandwidth from received packets (Hz)"; source=LoRaBWFromReception(receptionStarted); record=histogram,vector);        
        
        @statistic[txSF](title="SF from transmitted packets (SF)"; source=LoRaSFFromTransmission(transmissionStarted); record=histogram,vector);
        @statistic[txBandwidth](title="Bandwidth from transmitted packets (Hz)"; source=LoRaBWFromTransmission(transmissionStarted); record=histogram,vector);
        @statistic[txPower](title="Power from received packets (Hz)"; source=LoRaPowerFromTransmission(transmissionStarted); record=histogram,vector);
        @statistic[txDR](title="LoRa AU915 Datarate from transmitted packets"; source=LoRaDRAU915FromTransmission(transmissionStarted); record=histogram,vector);
        
        
        string signalAnalogRepresentation @enum("scalar", "dimensional") = default("dimensional");
        transmitter.typename = default(signalAnalogRepresentation == "scalar" ? "Ieee802154ScalarTransmitter" : signalAnalogRepresentation == "dimensional" ? "LoRaDimensionalTransmitter" : "");
        receiver.typename = default(signalAnalogRepresentation == "scalar" ? "Ieee802154ScalarReceiver" : signalAnalogRepresentation == "dimensional" ? "LoRaDimensionalReceiver" : "");

        centerFrequency = default(903.08 MHz);

        bandwidth = default(125 kHz);
        LoRaSF = default(7);
        LoRaCR = default(1); //coding rate at 4 / (4+LoRaCR) 
        iAmGateway = default(false);
           
        
        *.bitrate = default(5470 bps);        

        // PHY Header (without preamble), 802.15.4-2006 (page 43)
        // 1 octet SFD
        // 7 bit Frame length
        // 1 bit Reserved
        *.headerLength = (1*8 + 7 + 1) * 1 b;

        // Preamble
        // 4 octets Preamble
        // 1 symbol of 16us -> 4 bit
        transmitter.preambleDuration = (4*8/4) * 16 us;

        // RSSI sensitivity (ATmega256RFR2, page 566)
        receiver.energyDetection = default(-90dBm);

        // Receiver sensitivity (ATmega256RFR2, page 565)
        // TODO That is not quite true, because sensitivity
        //      is defined as the input signal power that yields
        //      a PER < 1% for a PSDU of 20 octets, but INET handles it
        //      as minimum reception power.
        receiver.sensitivity = default(-100dBm);

        // There is no fixed boundary, because of the
        // DSSS and the capture effect. Taking the sensitivity minus some
        // arbitrary value as an approximate guess.
        receiver.minInterferencePower = default(-120dBm);

		//K. Mikhaylov, J. Petäjäjärvi and J. Janhunen,
    	//"On LoRaWAN scalability: Empirical evaluation of susceptibility to inter-network interference,"
        //2017 European Conference on Networks and Communications (EuCNC), Oulu, 2017, pp. 1-6,
        //doi: 10.1109/EuCNC.2017.7980757.
        //-> this paper indicates reception above -6db SNR on fig 4d
    
        //L. E. Marquez, A. Osorio, M. Calle, J. C. Velez, A. Serrano and J. E. Candelo-Becerra,
        //"On the Use of LoRaWAN in Smart Cities: A Study With Blocking Interference,"
        //in IEEE Internet of Things Journal, vol. 7, no. 4, pp. 2806-2815, April 2020,
        //doi: 10.1109/JIOT.2019.2962976.
        //->this paper indicates reception above -15db for SF12 (TABLE III)
        receiver.snirNonLoRaThreshold = default(-6 dB);

        // TX Output power (typ. 3.5 dBm, ATmega256RFR2, page 564)
        transmitter.power = default(2.24mW);

        @class(labscim::physicallayer::LoRaRadio);
}

