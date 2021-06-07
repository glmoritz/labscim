//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef LABSCIM_LORABANDLISTENING_H_
#define LABSCIM_LORABANDLISTENING_H_

#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace inet;
//using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

class LabscimRadioRecorder : public cSimpleModule, public cListener
{
    private:
        bool LogEnabled;
        ofstream LogFile;

    protected:
        // The following redefined virtual function holds the algorithm.
        virtual void initialize(int stage) override;

        virtual void receiveSignal(cComponent *source, simsignal_t signalID, long l, cObject *details) override;
        virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

        virtual int numInitStages() const override { return NUM_INIT_STAGES; }

    public:
        virtual ~LabscimRadioRecorder();
};

} // namespace physicallayer

} // namespace labscim

#endif /* LABSCIM_LORABANDLISTENING_H_ */
