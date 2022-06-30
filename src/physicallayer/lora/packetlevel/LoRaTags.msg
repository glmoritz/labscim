//
// This program is property of its copyright holder. All rights reserved.
//

import inet.common.INETDefs;
import inet.common.TagBase;
import inet.common.Units;

cplusplus{{
#include "inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h"
}}

namespace inet;


class LoRaParamsTagBase extends TagBase
{
    int LoRaSF = -1;
    int LoRaCR = -1; //the fraction is 4 / (LoraCR + 4)    
}

class LoRaParamsInd extends LoRaParamsTagBase
{
}

class LoRaParamsReq extends LoRaParamsTagBase
{
}

class LoRaFHSSParamsInd extends TagBase
{
    int HPW = -1;        
    int FHSSCR = -1;
    int FHSSBW = -1;
}
