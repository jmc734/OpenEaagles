#include "openeaagles/simulation/Radio.h"

#include "openeaagles/simulation/Antenna.h"
#include "openeaagles/simulation/Datalink.h"
#include "openeaagles/simulation/Emission.h"
#include "openeaagles/simulation/Player.h"
#include "openeaagles/simulation/Simulation.h"

#include "openeaagles/basic/PairStream.h"
#include "openeaagles/basic/Identifier.h"
#include "openeaagles/basic/Number.h"
#include "openeaagles/basic/String.h"
#include "openeaagles/basic/Decibel.h"
#include "openeaagles/basic/units/Frequencies.h"
#include "openeaagles/basic/units/Powers.h"

namespace Eaagles {
namespace Simulation {

//==============================================================================
// Class: Radio
//==============================================================================
IMPLEMENT_SUBCLASS(Radio,"Radio")

// Slot table
BEGIN_SLOTTABLE(Radio)
   "numChannels",       // 1: Number of channels (less than or equal MAX_CHANNELS)
   "channels",          // 2: Our channels (list of Basic::Frequency objects)
   "channel",           // 3: Channel number [ 1 .. numChanels ]
   "maxDetectRange",    // 4: maximum detection capability (NM) (def: 120NM)
   "radioID",           // 5: radioID used by DIS
END_SLOTTABLE(Radio)

//  Map slot table
BEGIN_SLOT_MAP(Radio)
    ON_SLOT(1, setSlotNumChannels,  Basic::Number)
    ON_SLOT(2, setSlotChannels,     Basic::PairStream)
    ON_SLOT(3, setSlotChannel,      Basic::Number)
    ON_SLOT(4, setSlotMaxDetectRange, Basic::Number)
    ON_SLOT(5, setSlotRadioId,      Basic::Number)
END_SLOT_MAP()

//------------------------------------------------------------------------------
// Constructor(s)
//------------------------------------------------------------------------------
Radio::Radio()
{
   STANDARD_CONSTRUCTOR()

   initData();
}

void Radio::initData()
{
   channel = 0;
   chanFreqTbl = nullptr;
   numChan = 0;
   maxDetectRange = 120.0;
   radioId = 0;
}

//------------------------------------------------------------------------------
// copyData() -- copy member data
//------------------------------------------------------------------------------
void Radio::copyData(const Radio& org, const bool cc)
{
   BaseClass::copyData(org);
   if (cc) initData();

   // (Re)set the number of channels
   setNumberOfChannels(org.numChan);

   // Copy the channel frequencies
   unsigned short numChannels = getNumberOfChannels();
   for (unsigned short chan = 1; chan <= numChannels; chan++) {
      setChannelFrequency( chan, org.getChannelFrequency(chan) );
   }

   // Set the channel number
   setChannel(org.channel);

   radioId = org.radioId;
   maxDetectRange = org.maxDetectRange;
}

//------------------------------------------------------------------------------
// deleteData() -- delete member data
//------------------------------------------------------------------------------
void Radio::deleteData()
{
   setNumberOfChannels(0);
}

//------------------------------------------------------------------------------
// Get functions
//------------------------------------------------------------------------------

// Is the radio tuned to its current channel number
bool Radio::isChannelTuned() const
{
   return getChannelFrequency(channel) == getFrequency();
}

// Is the radio manually tuned (i.e., the current frequency
// is not the same as the current channel's frequency)
bool Radio::isManualTuned() const
{
   return getChannelFrequency(channel) != getFrequency();
}

// Returns the radio's channel number
unsigned short Radio::getChannel() const
{
   return channel;
}

// Number of channels
unsigned short Radio::getNumberOfChannels() const
{
   return (chanFreqTbl != nullptr) ? numChan : 0;
}

// Get a channel's frequency (Hz)
// Returns -1 if the channel is invalid
LCreal Radio::getChannelFrequency(const unsigned short chan) const
{
   unsigned short nc = getNumberOfChannels();
   LCreal freq = -1;
   if (chanFreqTbl != nullptr && nc > 0 && chan > 0 && chan <= nc) {
      freq = chanFreqTbl[chan-1];
   }
   return freq;
}

// Returns the radio's maximum detection range (NM)
LCreal Radio::getMaxDetectRange() const
{
   return maxDetectRange;
}

// DIS radio ID
unsigned short Radio::getRadioId() const
{
   return radioId;
}


//------------------------------------------------------------------------------
// Set Functions
//------------------------------------------------------------------------------

// Sets the radio's channel number and changes the radio frequency
bool Radio::setChannel(const unsigned short chan)
{
   bool ok = false;

   unsigned short nc = getNumberOfChannels();
   if (nc > 0 && chan <= nc) {

      // Set a temporary channel number, c1, which defaults to the
      // current channel number
      unsigned short c1 = chan;
      if (chan == 0) c1 = channel;

      // If the temp channel number is greater than zero then try
      // to set the channel's frequency
      if (c1 > 0) {
         ok = setFrequency( getChannelFrequency(c1) );
      }

      // If we were able to tune to this channel then make it the current channel
      if (ok) channel = c1;

   }
   return ok;
}

// Set a channel's frequency; channel numbers [ 1 .. getNumberOfChannels() ]
bool Radio::setChannelFrequency(const unsigned short chan, const LCreal freq)
{
   bool ok = false;

   unsigned short nc = getNumberOfChannels();
   if (chanFreqTbl != nullptr && nc > 0 && chan > 0 && chan <= nc) {
      chanFreqTbl[chan-1] = freq;
      ok = true;
   }

   return ok;
}

// Sets the number of channels; previous channels are lost!
bool Radio::setNumberOfChannels(const unsigned short n)
{
   bool ok = true;

   // When 'n' is zero
   if (n == 0) {
      // delete the old table
      numChan = 0;
      if (chanFreqTbl != nullptr) {
         delete[] chanFreqTbl;
         chanFreqTbl = nullptr;
      }
   }

   // When 'n' is less than or equal to the max
   else if (n <= MAX_CHANNELS) {
      // delete the old table and create a new one.
      if (chanFreqTbl != nullptr) delete[] chanFreqTbl;
      chanFreqTbl = new LCreal[n];
      numChan = n;
   }

   // default:
   else {
      // nothing done
      ok = false;
   }

   return ok;
}

// setMaxDetectRange() -- set the max range (NM)
bool Radio::setMaxDetectRange(const LCreal num)
{
   maxDetectRange = num;
   return true;
}

// setRadioID() -- set Radio ID
bool Radio::setRadioId(const unsigned short num)
{
   radioId = num;
   return true;
}

//------------------------------------------------------------------------------
// receive() -- process received emissions
//------------------------------------------------------------------------------
void Radio::receive(const LCreal dt)
{
   BaseClass::receive(dt);

   // Receiver losses
   LCreal noise = getRfRecvNoise();

   // ---
   // Process Emissions
   // ---

   Emission* em = nullptr;
   LCreal signal = 0;

   // Get an emission from the queue
   lcLock(packetLock);
   if (np > 0) {
      np--; // Decrement 'np', now the array index
      em = packets[np];
      signal = signals[np];
   }
   lcUnlock(packetLock);

   while (em != nullptr) {


      // Signal/Noise  (Equation 2-9)
      LCreal sn = signal / noise;
      LCreal snDbl = 10.0 * lcLog10(sn);

      // Is S/N above receiver threshold?
      if ( snDbl >= getRfThreshold() ) {
         // Report this valid emission to the radio model ...
         receivedEmissionReport(em);
      }

      em->unref();
      em = nullptr;

      // Get another emission from the queue
      lcLock(packetLock);
      if (np > 0) {
         np--;
         em = packets[np];
         signal = signals[np];
      }
      lcUnlock(packetLock);
   }
}

//------------------------------------------------------------------------------
// receivedEmissionReport() -- default (nothing to do)
//  Handle reports of valid emission reports (signal/noise ratio above threshold).
//------------------------------------------------------------------------------
void Radio::receivedEmissionReport(Emission* const)
{
}

//------------------------------------------------------------------------------
// Slot Functions  (return 'true' if the slot was set, else 'false')
//------------------------------------------------------------------------------

bool Radio::setSlotNumChannels(Basic::Number* const msg)
{
   bool ok = false;
   if (msg != nullptr) {
      const int v = msg->getInt();
      if (v >= 0 && v <= 0xFFFF) {
         ok = setNumberOfChannels( static_cast<unsigned short>(v) );
      }
   }
   return ok;
}

bool Radio::setSlotChannels(const Basic::PairStream* const msg)
{
   // ---
   // Quick out if the number of channels hasn't been set.
   // ---
   unsigned short nc = getNumberOfChannels();
   if (nc == 0 || msg == nullptr) {
      std::cerr << "Radio::setSlotChannels() Number of channels is not set!" << std::endl;
      return false;
   }

   {
      unsigned short chan = 1;
      const Basic::List::Item* item = msg->getFirstItem();
      while (chan <= nc && item != nullptr) {

         const Basic::Pair* pair = static_cast<const Basic::Pair*>(item->getValue());
         const Basic::Frequency* p = static_cast<const Basic::Frequency*>(pair->object());
         if (p != nullptr) {
            LCreal freq = Basic::Hertz::convertStatic( *p );
            bool ok = setChannelFrequency(chan, freq);
            if (!ok) {
               std::cerr << "Radio::setSlotChannels() Error setting frequency for channel " << chan << "; with freq = " << *p << std::endl;
            }
         }
         else {
            std::cerr << "Radio::setSlotChannels() channel# " << chan << " is not of type Frequency" << std::endl;
         }

         chan++;
         item = item->getNext();
      }
   }

   return true;
}

// channel: Channel the radio is set to
bool Radio::setSlotChannel(Basic::Number* const msg)
{
   bool ok = false;
   if (msg != nullptr) {
      const int v = msg->getInt();
      if (v >= 0 && v <= 0xFFFF) {
         ok = setChannel( static_cast<unsigned short>(v) );
      }
   }
   return ok;
}

// maxDetectRange: maximum detection capability (NM)
bool Radio::setSlotMaxDetectRange(Basic::Number* const num)
{
   bool ok = false;
   if (num != nullptr) {
      maxDetectRange = num->getReal();
      ok = true;
   }
   return ok;
}

// radio ID: the radio id used for DIS
bool Radio::setSlotRadioId(Basic::Number* const num)
{
   bool ok = false;
   if (num != nullptr) {
      const unsigned short num2 = static_cast<unsigned short>(num->getInt());
      ok = setRadioId(num2);
   }
   return ok;
}

//------------------------------------------------------------------------------
// getSlotByIndex()
//------------------------------------------------------------------------------
Basic::Object* Radio::getSlotByIndex(const int si)
{
   return BaseClass::getSlotByIndex(si);
}

//------------------------------------------------------------------------------
// serialize
//------------------------------------------------------------------------------
std::ostream& Radio::serialize(std::ostream& sout, const int i, const bool slotsOnly) const
{
   int j = 0;
   if ( !slotsOnly ) {
      indent(sout,i);
      sout << "( " << getFactoryName() << std::endl;
      j = 4;
   }

   indent(sout,i+j);
   sout << "numChannels: " << getNumberOfChannels() << std::endl;

   if (getNumberOfChannels() > 0) {
      indent(sout,i+j);
      sout << "channels: }" << std::endl;

      for (unsigned short chan = 1; chan <= getNumberOfChannels(); chan++) {
         LCreal freq = getChannelFrequency(chan);
         indent(sout,i+j+4);
         sout << "( MegaHertz " << (freq * Basic::Frequency::Hz2MHz) << " )" << std::endl;
      }

      indent(sout,i+j);
      sout << "}" << std::endl;
   }

   indent(sout,i+j);
   sout << "channel: " << getChannel() << std::endl;

   indent(sout,i+j);
   sout << "maxDetectRange: ( NauticalMiles " << maxDetectRange << " )" << std::endl;

   if (radioId > 0) {
      indent(sout,i+j);
      sout << "radioID: " << radioId << std::endl;
   }

   BaseClass::serialize(sout,i+j,true);

   if ( !slotsOnly ) {
      indent(sout,i);
      sout << ")" << std::endl;
   }

   return sout;
}


//==============================================================================
// Class: CommRadio
//==============================================================================
IMPLEMENT_SUBCLASS(CommRadio,"CommRadio")
EMPTY_SLOTTABLE(CommRadio)
EMPTY_SERIALIZER(CommRadio)

//------------------------------------------------------------------------------
// Constructor(s)
//------------------------------------------------------------------------------
CommRadio::CommRadio()
{
   STANDARD_CONSTRUCTOR()

   initData();
}

void CommRadio::initData()
{
   datalink = nullptr;
}

//------------------------------------------------------------------------------
// copyData() -- copy member data
//------------------------------------------------------------------------------
void CommRadio::copyData(const CommRadio& org, const bool cc)
{
   BaseClass::copyData(org);
   if (cc) initData();

   // No datalink yet
   setDatalink(nullptr);
}

//------------------------------------------------------------------------------
// deleteData() -- delete member data
//------------------------------------------------------------------------------
void CommRadio::deleteData()
{
   setDatalink(nullptr);
}

//------------------------------------------------------------------------------
// get functions
//------------------------------------------------------------------------------

Datalink* CommRadio::getDatalink()
{
   return datalink;
}

const Datalink* CommRadio::getDatalink() const
{
   return datalink;
}

//------------------------------------------------------------------------------
// Set functions
//------------------------------------------------------------------------------

bool CommRadio::setDatalink(Datalink* const p)
{
   datalink = p;
   return true;
}

//------------------------------------------------------------------------------
// transmitDataMessage() -- send a data message emission;
// returns true if the data emission was sent.
//------------------------------------------------------------------------------
bool CommRadio::transmitDataMessage(Basic::Object* const msg)
{
   bool sent = false;
   // Transmitting, scanning and have an antenna?
   if(getOwnship() == nullptr) {
      if (isMessageEnabled(MSG_DEBUG)) {
         std::cout << "CommRadio ownship == nullptr!" << std::endl;
      }
      return sent;
   }

   if (msg != nullptr && isTransmitterEnabled() && getAntenna() != nullptr) {
      // Send the emission to the other player
      Emission* em = new Emission();
      em->setDataMessage(msg);
      em->setFrequency(getFrequency());
      em->setBandwidth(getBandwidth());
      em->setPower(getPeakPower());
      em->setTransmitLoss(getRfTransmitLoss());
      em->setMaxRangeNM(getMaxDetectRange());
      em->setTransmitter(this);
      em->setReturnRequest(false);
      getAntenna()->rfTransmit(em);
      em->unref();
      sent = true;
   }
   return sent;
}

//------------------------------------------------------------------------------
// receivedEmissionReport() -- Datalink messages --
//  Handle reports of valid emission reports (signal/noise ratio above threshold).
//------------------------------------------------------------------------------
void CommRadio::receivedEmissionReport(Emission* const em)
{
   if (em != nullptr && datalink != nullptr) {
      // If we have a datalink and this emission contains a message, then it
      // must be a datalink message.
      Basic::Object* msg = em->getDataMessage();
      if (msg != nullptr) datalink->event(DATALINK_MESSAGE, msg);
   }
}

} // End Simulation namespace
} // End Eaagles namespace
