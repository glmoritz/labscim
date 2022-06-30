/*
 * LoRaFHSSHopEntry.h
 *
 *  Created on: 3 de jun de 2022
 *      Author: guilherme
 */

#ifndef PHYSICALLAYER_LORA_PACKETLEVEL_LORAFHSSHOPENTRY_H_
#define PHYSICALLAYER_LORA_PACKETLEVEL_LORAFHSSHOPENTRY_H_
#include <inttypes.h>
#include "inet/common/Units.h"
#include "inet/common/math/Functions.h"

using namespace inet;

namespace labscim {
namespace physicallayer {

const double PLL_STEP_IN_HZ=0.95367431640625; //found in the Semtech's FHSS code

class LoRaFHSSHopEntry {
public:
    LoRaFHSSHopEntry(uint16_t nb_symbols, uint32_t freq_in_pll_steps, bool isHeader);
    virtual ~LoRaFHSSHopEntry();

    // Copy constructor
    LoRaFHSSHopEntry(const LoRaFHSSHopEntry& p1):
    nb_symbols(p1.getSymbols()),
    freq_pll_steps(p1.getCenterFrequencySteps()),
    Header(p1.isHeader())
    {
    }

    LoRaFHSSHopEntry& operator= (LoRaFHSSHopEntry const &other);

    void copy(const LoRaFHSSHopEntry& other);

    virtual const Hz getBandwidth() const {return Hz(488.28125);};
    virtual const Hz getCenterFrequency() const {return Hz(freq_pll_steps*PLL_STEP_IN_HZ);};
    virtual const uint32_t getCenterFrequencySteps() const {return freq_pll_steps;};
    virtual const uint16_t getSymbols() const {return nb_symbols;};
    virtual const bool isHeader() const {return Header;};

    virtual void setCenterFrequencySteps(uint32_t freq_pll_steps) {this->freq_pll_steps = freq_pll_steps;};
    virtual void setSymbols(uint16_t nb_symbols) {this->nb_symbols = nb_symbols;};
    virtual void setisHeader(bool Header) {this->Header= Header;};



    virtual const simtime_t getDuration() const {return simtime_t(nb_symbols / 488.28125);};

private:
    uint16_t nb_symbols;
    uint32_t freq_pll_steps;
    bool Header;

};
}
} /* namespace labscim */

#endif /* PHYSICALLAYER_LORA_PACKETLEVEL_LORAFHSSHOPENTRY_H_ */
