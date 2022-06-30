//
// Copyright (C) 2006 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, see <http://www.gnu.org/licenses/>.
//

import inet.common.INETDefs;
import inet.common.Units;
import inet.physicallayer.contract.packetlevel.IModulation;
import inet.physicallayer.contract.packetlevel.RadioControlInfo;
//import inet.physicallayer.contract.packetlevel.ConfigureRadioCommand;


namespace inet::physicallayer;

//
// Control info attached to a configure command that is sent to the ~Radio.
//
class ConfigureLoRaRadioCommand extends ConfigureRadioCommand
{
    int LoRaSF = -1;                  // new lora radio SF or -1 if not set.
    int LoRaCR = -1;                  // new lora radio CR or -1 if not set.  
    int CRC_enabled = -1;
    int Header_enabled = -1;
    int LowDataRate_optimization = -1;  
    int Preamble_length = -1;
    int Payload_length = -1;        
    int FHSSCR = -1;                  // new lora radio FHSS CR or -1 if not set.
    int FHSSBW = -1;                  // new lora FHSS Bandwidth index or -1 if not set.
    int FHSSGrid = -1;				// new lora FHSS grid index or -1 if not set.
}
