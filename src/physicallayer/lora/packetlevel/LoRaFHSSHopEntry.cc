/*
 * LoRaFHSSHopEntry.cpp
 *
 *  Created on: 3 de jun de 2022
 *      Author: guilherme
 */

#include "LoRaFHSSHopEntry.h"

namespace labscim {
namespace physicallayer {

LoRaFHSSHopEntry::LoRaFHSSHopEntry(uint16_t nb_symbols, uint32_t freq_in_pll_steps, bool isHeader):
                    nb_symbols(nb_symbols),
                    freq_pll_steps(freq_in_pll_steps),
                    Header(isHeader)
{
}

LoRaFHSSHopEntry& LoRaFHSSHopEntry::operator=(const LoRaFHSSHopEntry& other)
{
    if (this == &other) return *this;
    labscim::physicallayer::LoRaFHSSHopEntry::operator=(other);
    copy(other);
    return *this;
}

void LoRaFHSSHopEntry::copy(const LoRaFHSSHopEntry& other)
{
    this->freq_pll_steps = other.freq_pll_steps;
    this->nb_symbols  = other.nb_symbols;
    this->Header = other.Header;
}


LoRaFHSSHopEntry::~LoRaFHSSHopEntry() {
}

} /* namespace physicallayer */
} /* namespace labscim */
