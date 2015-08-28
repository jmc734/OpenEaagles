//------------------------------------------------------------------------------
// LagFilter class -- First order lag filter
//------------------------------------------------------------------------------
#ifndef __Eaagles_LinearSystem_LagFilter_H__
#define __Eaagles_LinearSystem_LagFilter_H__

#include "openeaagles/linearSys/FirstOrderTf.h"

namespace Eaagles {
   namespace Basic { class Time; }

namespace LinearSystem {

//------------------------------------------------------------------------------
// Class:  LagFilter
// Base class:  Object -> ScalerFunc-> DiffEquation -> Sz1 -> LagFilter
//
// Description: Models a first order lag filter with a time constant 'tau' seconds
//
// Transfer function:
//
//           1
//       ---------
//        tau*S + 1
//
// Note: tau must be greater than zero.
//
// Factory name: LagFilter
// Slots:
//    tau    <Time>     Filer time constant
//    tau    <Number>   Filer time constant (sec)
//
//
//------------------------------------------------------------------------------
class LagFilter : public FirstOrderTf
{
    DECLARE_SUBCLASS(LagFilter,FirstOrderTf)

public:
   LagFilter();
   LagFilter(const unsigned int rate, const LCreal tau);

   LCreal getTau() const                         { return tau; }
   virtual bool setTau(const LCreal v);

   virtual bool setSlotTau(const Basic::Time* const msg);
   virtual bool setSlotTau(const Basic::Number* const msg);

private:
   LCreal tau;    // filter time constant (seconds)
};

} // End LinearSystem namespace
} // End Eaagles namespace

#endif
