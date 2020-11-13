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
    int LoRaSF = 6;
    int LoRaCR = 1; //the fraction is 4 / (LoraCR + 4)
}

class LoRaParamsInd extends LoRaParamsTagBase
{
}

class LoRaParamsReq extends LoRaParamsTagBase
{
}