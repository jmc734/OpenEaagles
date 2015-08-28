//------------------------------------------------------------------------------
// Class: Jammer
//------------------------------------------------------------------------------
#ifndef __Eaagles_Simulation_Jammer_H__
#define __Eaagles_Simulation_Jammer_H__

#include "openeaagles/simulation/RfSensor.h"

namespace Eaagles {
namespace Simulation {
//------------------------------------------------------------------------------
// Class: Jammer
// Description: Example Jammer
// Factory name: Jammer
//
// Default R/F sensor type ID is "JAMMER"
//------------------------------------------------------------------------------
class Jammer : public RfSensor
{
    DECLARE_SUBCLASS(Jammer,RfSensor)

public:
    Jammer();

protected:
    void transmit(const LCreal dt) override;
};

} // End Simulation namespace
} // End Eaagles namespace

#endif
