//------------------------------------------------------------------------------
// Classes:
//      SimLogger                   -- Simulation Event logger
//      SimLogger::SimLogEvent      -- Abstract simulation log event
//      SimLogger::NewPlayer        -- New Player event
//      SimLogger::LogPlayerData    -- Log Player Data event
//      SimLogger::RemovePlayer     -- Remove Player event
//      SimLogger::WeaponRelease    -- Weapon Release
//      SimLogger::GunFired         -- Gun was fired
//      SimLogger::KillEvent        -- Kill
//      SimLogger::NewTrack         -- Added new RADAR track
//      SimLogger::UpdateTrack      -- Updated RADAR track
//      SimLogger::RemovedTrack     -- Removed old RADAR track
//      SimLogger::NewRwrTrack      -- Added new RWR track
//      SimLogger::UpdateRwrTrack   -- Updated RWR track
//      SimLogger::RemovedRwrTrack  -- Removed old RWR track
//      SimLogger::DetonationEvent  -- Weapon Detonated
//------------------------------------------------------------------------------

#include "openeaagles/simulation/SimLogger.h"

#include "openeaagles/simulation/AirVehicle.h"
#include "openeaagles/simulation/Emission.h"
#include "openeaagles/simulation/NetIO.h"
#include "openeaagles/simulation/Nib.h"
#include "openeaagles/simulation/Player.h"
#include "openeaagles/simulation/Simulation.h"
#include "openeaagles/simulation/Track.h"
#include "openeaagles/simulation/TrackManager.h"

#include "openeaagles/basic/units/Angles.h"
#include "openeaagles/basic/units/Times.h"

#include <string>
#include <sstream>
#include <cstring>

// Disable all deprecation warnings for now.  Until we fix them,
// they are quite annoying to see over and over again...
#if(_MSC_VER>=1400)   // VC8+
# pragma warning(disable: 4996)
#endif

// ---
// SIMLOGEVENT_B --
// use this macro in place of IMPLEMENT_EMPTY_SLOTTABLE_SUBCLASS() for classes derived from SimLogger::SimLogEvent
// ---
#define SIMLOGEVENT_B(ThisType,FORMNAME)                                        \
    IMPLEMENT_PARTIAL_SUBCLASS(SimLogger::ThisType,FORMNAME)                    \
    EMPTY_SLOTTABLE(SimLogger::ThisType)                                        \
    SimLogger::ThisType::ThisType(const ThisType& org)                          \
    {                                                                           \
        STANDARD_CONSTRUCTOR()                                                  \
        copyData(org,true);                                                     \
    }                                                                           \
    SimLogger::ThisType::~ThisType()                                            \
    {                                                                           \
       STANDARD_DESTRUCTOR()                                                    \
    }                                                                           \
    SimLogger::ThisType& SimLogger::ThisType::operator=(const ThisType& org)    \
    {                                                                           \
        if (this != &org) copyData(org,false);                                  \
        return *this;                                                           \
    }                                                                           \
    SimLogger::ThisType* SimLogger::ThisType::clone() const                     \
    {                                                                           \
        return new SimLogger::ThisType(*this);                                  \
    }

namespace Eaagles {
namespace Simulation {

//==============================================================================
// Class SimLogger
//==============================================================================
IMPLEMENT_PARTIAL_SUBCLASS(SimLogger,"SimLogger")
EMPTY_DELETEDATA(SimLogger)

//------------------------------------------------------------------------------
// Slot table
//------------------------------------------------------------------------------
BEGIN_SLOTTABLE(SimLogger)
   "timeline",             // 1: Source of the time line { UTC, SIM or EXEC } (default: UTC)  (Basic::Identifier)
   "includeUtcTime",       // 2: record UTC time for data  ( default: true)   (Basic::Number)
   "includeSimTime",       // 3: record SIM time for data  ( default: true)   (Basic::Number)
   "includeExecTime",      // 4: record EXEC time for data ( default: true)   (Basic::Number)
END_SLOTTABLE(SimLogger)

// Map slot table to handles
BEGIN_SLOT_MAP(SimLogger)
   ON_SLOT(1,  setSlotTimeline,        Basic::Identifier)
   ON_SLOT(2,  setSlotIncludeUtcTime,  Basic::Number)
   ON_SLOT(3,  setSlotIncludeSimTime,  Basic::Number)
   ON_SLOT(4,  setSlotIncludeExecTime, Basic::Number)
END_SLOT_MAP()

// Constructor
SimLogger::SimLogger() : seQueue(MAX_QUEUE_SIZE)
{
    STANDARD_CONSTRUCTOR()
    time = 0.0;
    execTime = 0.0;
    simTime = 0.0;
    utcTime = 0.0;
    timeline = UTC;
    includeUtcTime = true;
    includeSimTime = true;
    includeExecTime = true;
}

// Copy Constructor
SimLogger::SimLogger(const SimLogger& org) : seQueue(MAX_QUEUE_SIZE)
{
    STANDARD_CONSTRUCTOR()
    copyData(org,true);
}

// Destructor
SimLogger::~SimLogger()
{
   STANDARD_DESTRUCTOR()
}

// Copy operator
SimLogger& SimLogger::operator=(const SimLogger& org)
{
    if (this != &org) copyData(org,false);
    return *this;
}

// Clone
SimLogger* SimLogger::clone() const
{
    return new SimLogger(*this);
}

// copyData() -- copy member data
void SimLogger::copyData(const SimLogger& org, const bool)
{
    BaseClass::copyData(org);
    time = org.time;
    execTime = org.execTime;
    simTime = org.simTime;
    utcTime = org.utcTime;
    timeline = org.timeline;
    includeUtcTime = org.includeUtcTime;
    includeSimTime = org.includeSimTime;
    includeExecTime = org.includeExecTime;
}

//------------------------------------------------------------------------------
// updateTC() -- Update the simulation log time
//------------------------------------------------------------------------------
void SimLogger::updateTC(const LCreal dt)
{
    BaseClass::updateTC(dt);

    // Try to find our simulation
    Simulation* const sim = static_cast<Simulation*>(findContainerByType(typeid(Simulation)));

    // Update the simulation time
    if (sim != nullptr) {
        // Use the (UTC, SIM or EXEC) time
        if (getTimeline() == UTC) time = sim->getSysTimeOfDay();
        else if (getTimeline() == SIM) time = sim->getSimTimeOfDay();
        else time = sim->getExecTimeSec();

        execTime = sim->getExecTimeSec();          // Executive time since start (seconds)
        simTime = sim->getSimTimeOfDay();          // Simulated time of day (seconds)
        utcTime = sim->getSysTimeOfDay();          // Computer system's time of day (seconds)
    }
    else {
        // or keep your own time
        time += dt;
        execTime += dt;
    }
}

//------------------------------------------------------------------------------
// updateData() -- During background we'll build & send the descriptions of the
//                 simulation events to the log file.
//------------------------------------------------------------------------------
void SimLogger::updateData(const LCreal dt)
{
    BaseClass::updateData(dt);

    // Log the simulation events
    SimLogEvent* event = seQueue.get();
    while (event != nullptr) {
        const char* const p = event->getDescription();
        Basic::Logger::log(p);
        event->unref();

        event = seQueue.get();
    }
}

//------------------------------------------------------------------------------
// log() -- Log a simulation event -- this may be from the T/C thread so just
//          set the time and queue it up for processing by updateData().
//------------------------------------------------------------------------------
void SimLogger::log(LogEvent* const event)
{
    SimLogEvent* const simEvent = dynamic_cast<SimLogEvent*>(event);
    if (simEvent != nullptr) {
        simEvent->ref();
        simEvent->setTime(time);
        simEvent->setExecTime(execTime);
        simEvent->setUtcTime(utcTime);
        simEvent->setSimTime(simTime);
        simEvent->setPrintExecTime(includeExecTime);
        simEvent->setPrintUtcTime(includeUtcTime);
        simEvent->setPrintSimTime(includeSimTime);
        simEvent->captureData();

        seQueue.put(simEvent);
    }
    else {
        Basic::Logger::log(event);
    }
}

//------------------------------------------------------------------------------
// Set functions
//------------------------------------------------------------------------------

// Sets the logger's time line
bool SimLogger::setTimeline(const TSource ts)
{
    timeline = ts;
    return true;
}

bool SimLogger::setIncludeUtcTime(const bool b)
{
    includeUtcTime = b;
    return true;
}

bool SimLogger::setIncludeSimTime(const bool b)
{
    includeSimTime = b;
    return true;
}

bool SimLogger::setIncludeExecTime(const bool b)
{
    includeExecTime = b;
    return true;
}

//------------------------------------------------------------------------------
// Slot functions
//------------------------------------------------------------------------------

// Sets the source of the time ( UTM, SIM or EXEC )
bool SimLogger::setSlotTimeline(const Basic::Identifier* const p)
{
    bool ok = false;
    if (p != nullptr) {
        if (*p == "EXEC" || *p == "exec") {
            setTimeline( EXEC );
            ok = true;
        }
        else if (*p == "UTC" || *p == "utc") {
            setTimeline( UTC );
            ok = true;
        }
        else if (*p == "SIM" || *p == "sim") {
            setTimeline( SIM );
            ok = true;
        }
    }
    return ok;
}

// whether or not to include UTC time in tabular time message
bool SimLogger::setSlotIncludeUtcTime(const Basic::Number* const num)
{
    return setIncludeUtcTime(num->getBoolean());
}

// whether or not to include SIM time in tabular time message
bool SimLogger::setSlotIncludeSimTime(const Basic::Number* const num)
{
    return setIncludeSimTime(num->getBoolean());
}

// whether or not to include EXEC time in tabular time message
bool SimLogger::setSlotIncludeExecTime(const Basic::Number* const num)
{
    return setIncludeExecTime(num->getBoolean());
}

//------------------------------------------------------------------------------
// getSlotByIndex()
//------------------------------------------------------------------------------
Basic::Object* SimLogger::getSlotByIndex(const int si)
{
    return BaseClass::getSlotByIndex(si);
}

//------------------------------------------------------------------------------
// serialize
//------------------------------------------------------------------------------
std::ostream& SimLogger::serialize(std::ostream& sout, const int i, const bool slotsOnly) const
{
    int j = 0;
    if ( !slotsOnly ) {
        indent(sout,i);
        sout << "( " << getFactoryName() << std::endl;
        j = 4;
    }

    indent(sout,i+j);
    sout << "timeline: ";
    if (getTimeline() == EXEC) sout << "EXEC";
    else if (getTimeline() == SIM) sout << "SIM";
    else sout << "UTC";
    sout << std::endl;

    sout << "includeExecTime: ";
    if (includeExecTime == true)
        sout << "true";
    else
        sout << "false";
    sout << std::endl;

    sout << "includeUtcTime: ";
    if (includeUtcTime == true)
        sout << "true";
    else
        sout << "false";
    sout << std::endl;

    sout << "includeSimTime: ";
    if (includeSimTime == true)
        sout << "true";
    else
        sout << "false";

    sout << std::endl;

    BaseClass::serialize(sout,i+j,true);

    if ( !slotsOnly ) {
        indent(sout,i);
        sout << ")" << std::endl;
    }

    return sout;
}

//==============================================================================
// Class: SimLogEvent
//==============================================================================
IMPLEMENT_PARTIAL_SUBCLASS(SimLogger::SimLogEvent,"SimLogEvent")
EMPTY_SLOTTABLE(SimLogger::SimLogEvent)
EMPTY_SERIALIZER(SimLogger::SimLogEvent)

//------------------------------------------------------------------------------
// Constructor(s) & Destructors
//------------------------------------------------------------------------------
SimLogger::SimLogEvent::SimLogEvent()
{
    STANDARD_CONSTRUCTOR()

    time = 0.0;
    simTime = 0.0;
    execTime = 0.0;
    utcTime = 0.0;
    printUtcTime = false;
    printSimTime = false;
    printExecTime = false;
    msg = nullptr;
}

SimLogger::SimLogEvent::SimLogEvent(const SimLogEvent& org)
{
    STANDARD_CONSTRUCTOR()
    copyData(org,true);
}

// Destructor
SimLogger::SimLogEvent::~SimLogEvent()
{
    STANDARD_DESTRUCTOR()
}

//------------------------------------------------------------------------------
// Copy & Clone
//------------------------------------------------------------------------------
SimLogger::SimLogEvent& SimLogger::SimLogEvent::operator=(const SimLogEvent& org)
{
    if (this != &org) copyData(org,false);
    return *this;
}

SimLogger::SimLogEvent* SimLogger::SimLogEvent::clone() const
{
    return nullptr;
}

//------------------------------------------------------------------------------
// copyData() -- copy member data
//------------------------------------------------------------------------------
void SimLogger::SimLogEvent::copyData(const SimLogEvent& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) msg = nullptr;

    time = org.time;
    simTime = org.simTime;
    execTime = org.execTime;
    utcTime = org.utcTime;
    printUtcTime = org.printUtcTime;
    printSimTime = org.printSimTime;
    printExecTime = org.printExecTime;

    if (org.msg != nullptr) {
        size_t len = std::strlen(org.msg);
        msg = new char[len+1];
        lcStrcpy(msg,(len+1),org.msg);
    }
}

//------------------------------------------------------------------------------
// deleteData() -- delete member data
//------------------------------------------------------------------------------
void SimLogger::SimLogEvent::deleteData()
{
    if (msg != nullptr) delete[] msg;
    msg = nullptr;
}

//------------------------------------------------------------------------------
// makeTimeMsg() -- make the time string
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeTimeMsg(std::ostream& sout)
{
    char cbuf[16];
    int hh = 0;     // Hours
    int mm = 0;     // Min
    LCreal ss = 0;  // Sec
    Basic::Time::getHHMMSS(static_cast<LCreal>(time), &hh, &mm, &ss);
    std::sprintf(cbuf, "%02d:%02d:%06.3f", hh, mm, ss);
    sout << cbuf;
    return sout;
}


//------------------------------------------------------------------------------
// makeExecTimeMsg() -- make the time string for EXEC time
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeExecTimeMsg(std::ostream& sout)
{
    char cbuf[16];
    int hh = 0;     // Hours
    int mm = 0;     // Min
    LCreal ss = 0;  // Sec

    // exec time
    Basic::Time::getHHMMSS(static_cast<LCreal>(execTime), &hh, &mm, &ss);
    std::sprintf(cbuf, "%02d:%02d:%06.3f", hh, mm, ss);
    sout << cbuf;

    return sout;
}

//------------------------------------------------------------------------------
// makeUtcTimeMsg() -- make the time string for UTC time
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeUtcTimeMsg(std::ostream& sout)
{
    char cbuf[16];
    int hh = 0;     // Hours
    int mm = 0;     // Min
    LCreal ss = 0;  // Sec

    // sim time
    Basic::Time::getHHMMSS(static_cast<LCreal>(utcTime), &hh, &mm, &ss);
    std::sprintf(cbuf, "%02d:%02d:%06.3f", hh, mm, ss);
    sout << cbuf;

    return sout;
}

//------------------------------------------------------------------------------
// makeSimTimeMsg() -- make the time string for Simulation time
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeSimTimeMsg(std::ostream& sout)
{
    char cbuf[16];
    int hh = 0;     // Hours
    int mm = 0;     // Min
    LCreal ss = 0;  // Sec

    // utc time
    Basic::Time::getHHMMSS(static_cast<LCreal>(simTime), &hh, &mm, &ss);
    std::sprintf(cbuf, "%02d:%02d:%06.3f", hh, mm, ss);
    sout << cbuf;

    return sout;
}

//------------------------------------------------------------------------------
// makeTabTimeHdr() -- make heading for the time string
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeTabTimeHdr(std::ostream& sout)
{
    if(printExecTime)
        sout << "EXEC time" << "\t";

    if(printUtcTime)
        sout << "UTC time" << "\t";

    if(printSimTime)
        sout << "SIM time" << "\t";

    if(!printExecTime && !printUtcTime && !printSimTime)
        sout << "time" << "\t";

    return sout;
}

//------------------------------------------------------------------------------
// makeTimeMsg() -- make the time string
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeTabTimeMsg(std::ostream& sout)
{
    if(printExecTime)
    {
        makeExecTimeMsg(sout);
        sout << "\t";
    }

    if(printUtcTime)
    {
        makeUtcTimeMsg(sout);
        sout << "\t";
    }

    if(printSimTime)
    {
        makeSimTimeMsg(sout);
        sout << "\t";
    }

    if(!printExecTime && !printUtcTime && !printSimTime)
    {
        makeTimeMsg(sout);
        sout << "\t";
    }

    return sout;
}

//------------------------------------------------------------------------------
// makePlayerIdMsg() -- creates the player ID message
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makePlayerIdMsg(std::ostream& sout, const Player* const player)
{
    if (player != nullptr) {
        sout << "(" << player->getID();
        if (player->isNetworkedPlayer()) {
            const Nib* const pNib = player->getNib();
            sout << "," << static_cast<const char*>(*pNib->getFederateName());
        }
        sout << ")";
    }
    return sout;
}

//------------------------------------------------------------------------------
// makePlayerDataMsg() -- creates the player ID message
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makePlayerDataMsg(
            std::ostream& sout,
            osg::Vec3 pos0, osg::Vec3 vel0, osg::Vec3 angles0)
{
    sout << ", position=("    << pos0[0]    << "," << pos0[1]    << "," << pos0[2]    << ")";
    sout << ", velocity=("    << vel0[0]    << "," << vel0[1]    << "," << vel0[2]    << ")";
    sout << ", orientation=(" << (angles0[0] * Basic::Angle::R2DCC) << "," << (angles0[1] * Basic::Angle::R2DCC) << "," << (angles0[2] * Basic::Angle::R2DCC) << ")";
    return sout;
}

//------------------------------------------------------------------------------
// makeTrackDataMsg() -- creates the track data message
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeTrackDataMsg(std::ostream& sout, const Track* const trk)
{
    if (trk != nullptr) {
        sout << "\t Type              = " << trk->getType()            << "\n";
        sout << "\t Range Slang       = " << trk->getRange()           << "\n";
        sout << "\t Range Ground      = " << trk->getGroundRange()     << "\n";
        sout << "\t Azimuth True      = " << trk->getTrueAzimuthD()    << "\n";
        sout << "\t Azimuth Relative  = " << trk->getRelAzimuthD()     << "\n";
        sout << "\t Elevation Angle   = " << trk->getElevationD()      << "\n";
        osg::Vec3 tpos = trk->getPosition();
        sout << "\t Position          =(" << tpos[0]    << "," << tpos[1]    << "," << tpos[2]    << ")"      << "\n";;
        osg::Vec3 tvel = trk->getVelocity();
        sout << "\t Velocity          =(" << tvel[0]    << "," << tvel[1]    << "," << tvel[2]    << ")"      << "\n";

        const RfTrack* const rfTrk = dynamic_cast<const RfTrack*>(trk);
        if (rfTrk != nullptr) {
           sout << "\t Signal            = " << rfTrk->getAvgSignal()       << "\n";
        }
    }
    return sout;
}

//------------------------------------------------------------------------------
// makeEmissionDataMsg() -- creates the track data message
//------------------------------------------------------------------------------
std::ostream& SimLogger::SimLogEvent::makeEmissionDataMsg(std::ostream& sout, const Emission* const em)
{
    if (em != nullptr) {
        sout << "\t AOI Azimuth   = " << (Basic::Angle::R2DCC * em->getAzimuthAoi())   << "\n";
        sout << "\t AOI Elevation = " << (Basic::Angle::R2DCC * em->getElevationAoi()) << "\n";
        sout << "\t Frequency     = " << (em->getFrequency())                     << "\n";
        sout << "\t Lambda        = " << (em->getWavelength())                    << "\n";
        sout << "\t Pulse width   = " << (em->getPulseWidth())                    << "\n";
        sout << "\t PRF           = " << (em->getPRF())                           << "\n";
    }
    return sout;
}

//==============================================================================
// Class SimLogger::NewPlayer
//==============================================================================
SIMLOGEVENT_B(NewPlayer,"SimLogger::NewPlayer")
EMPTY_SERIALIZER(SimLogger::NewPlayer)

// Constructors
SimLogger::NewPlayer::NewPlayer(Player* const p)
{
    STANDARD_CONSTRUCTOR()
    initData();
    thePlayer = p;
}

void SimLogger::NewPlayer::initData()
{
    thePlayer = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
}

// Copy data function
void SimLogger::NewPlayer::copyData(const NewPlayer& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
}

// Delete data function
void SimLogger::NewPlayer::deleteData()
{
    thePlayer = nullptr;
}

// Get the description
const char* SimLogger::NewPlayer::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " ADDED_PLAYER:\n";

        // Print the Player data
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout,thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::NewPlayer::captureData()
{
    if (thePlayer != nullptr) {
        pos = thePlayer->getPosition();
        vel = thePlayer->getVelocity();
        angles = thePlayer->getEulerAngles();
    }
}

//==============================================================================
// Class SimLogger::LogPlayerData
//==============================================================================
SIMLOGEVENT_B(LogPlayerData,"SimLogger::LogPlayerData")
EMPTY_SERIALIZER(SimLogger::LogPlayerData)

// Constructors
SimLogger::LogPlayerData::LogPlayerData(Player* const p)
{
    STANDARD_CONSTRUCTOR()
    initData();
    thePlayer = p;
}

void SimLogger::LogPlayerData::initData()
{
    thePlayer = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
    alpha = 0.0;
    beta = 0.0;
    ias = 0.0;
}

// Copy data function
void SimLogger::LogPlayerData::copyData(const LogPlayerData& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
    alpha = org.alpha;
    beta = org.beta;
    ias = org.ias;
}

// Delete data function
void SimLogger::LogPlayerData::deleteData()
{
    thePlayer = nullptr;
}

// Get the description
const char* SimLogger::LogPlayerData::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " PLAYER_DATA:\n";

        // Print the Player data
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout,thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            if (ias >= 0.0f) {
                sout << ", alpha=" << alpha;
                sout << ", beta=" << beta;
                sout << ", ias=" << ias;
            }
            sout << "\n";
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::LogPlayerData::captureData()
{
    if (thePlayer != nullptr) {
        alpha = -1.0;
        beta = -1.0;
        ias = -1.0;
        {
            pos = thePlayer->getPosition();
            vel = thePlayer->getVelocity();
            angles = thePlayer->getEulerAngles();
            const Player* const p = thePlayer;
            const AirVehicle* const av = dynamic_cast<const AirVehicle*>(p);
            if (av != nullptr) {
                alpha = av->getAngleOfAttackD();
                beta = av->getSideSlipD();
                ias = av->getCalibratedAirspeed();
            }
        }
    }
}

//==============================================================================
// Class SimLogger::RemovePlayer
//==============================================================================
SIMLOGEVENT_B(RemovePlayer,"SimLogger::RemovePlayer")
EMPTY_SERIALIZER(SimLogger::RemovePlayer)

// Constructor
SimLogger::RemovePlayer::RemovePlayer(Player* const p)
{
    STANDARD_CONSTRUCTOR()
    initData();
    thePlayer = p;
}

void SimLogger::RemovePlayer::initData()
{
    thePlayer = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
}

// Copy data function
void SimLogger::RemovePlayer::copyData(const RemovePlayer& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
}

// Delete data function
void SimLogger::RemovePlayer::deleteData()
{
    thePlayer = nullptr;
}

// Get the description
const char* SimLogger::RemovePlayer::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " REMOVED_PLAYER:\n";

        // Print the Player Data
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout, thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::RemovePlayer::captureData()
{
    if (thePlayer != nullptr) {
        pos = thePlayer->getPosition();
        vel = thePlayer->getVelocity();
        angles = thePlayer->getEulerAngles();
    }
}

//==============================================================================
// Class SimLogger::WeaponRelease
//==============================================================================
SIMLOGEVENT_B(WeaponRelease,"SimLogger::WeaponRelease")
EMPTY_SERIALIZER(SimLogger::WeaponRelease)

// Constructor
SimLogger::WeaponRelease::WeaponRelease(Player* const lancher, Player* const wpn, Player* const tgt)
{
    STANDARD_CONSTRUCTOR()
    thePlayer = lancher;
    theWeapon = wpn;
    theTarget = tgt;
}

void SimLogger::WeaponRelease::initData()
{
    thePlayer = nullptr;
    theWeapon = nullptr;
    theTarget = nullptr;
}

// Copy data function
void SimLogger::WeaponRelease::copyData(const WeaponRelease& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();
    thePlayer = org.thePlayer;
    theWeapon = org.theWeapon;
    theTarget = org.theTarget;
}

// Delete data function
void SimLogger::WeaponRelease::deleteData()
{
    thePlayer = nullptr;
    theWeapon = nullptr;
    theTarget = nullptr;
}

// Get the description
const char* SimLogger::WeaponRelease::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " WEAPON_RELEASE:";

        // Print the Player ID
        if (thePlayer != nullptr) {
            sout << " launcher";
            makePlayerIdMsg(sout, thePlayer);
        }

        // Print the WPN ID
        if (theWeapon != nullptr) {
            sout << " wpn";
            makePlayerIdMsg(sout, theWeapon);
        }

        // Print the TGT ID
        if (theTarget != nullptr) {
            sout << " tgt";
            makePlayerIdMsg(sout, theTarget);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::WeaponRelease::captureData()
{
}

//==============================================================================
// Class SimLogger::GunFired
//==============================================================================
SIMLOGEVENT_B(GunFired,"SimLogger::GunFired")
EMPTY_SERIALIZER(SimLogger::GunFired)

// Constructor
SimLogger::GunFired::GunFired(Player* const lancher, const int n)
{
    STANDARD_CONSTRUCTOR()
    thePlayer = lancher;
    rounds = n;
}

void SimLogger::GunFired::initData()
{
    thePlayer = nullptr;
    rounds = 0;
}

// Copy data function
void SimLogger::GunFired::copyData(const GunFired& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    rounds = org.rounds;
}

// Delete data function
void SimLogger::GunFired::deleteData()
{
    thePlayer = nullptr;
}

// Get the description
const char* SimLogger::GunFired::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " GUN FIRED:";

        // Print the Player ID
        if (thePlayer != nullptr) {
            sout << " launcher";
            makePlayerIdMsg(sout, thePlayer);
        }

        sout << ", rounds=" << rounds;

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::GunFired::captureData()
{
}

//==============================================================================
// Class SimLogger::KillEvent
//==============================================================================
SIMLOGEVENT_B(KillEvent,"SimLogger::KillEvent")
EMPTY_SERIALIZER(SimLogger::KillEvent)

// Constructor
SimLogger::KillEvent::KillEvent(Player* const lancher, Player* const wpn, Player* const tgt)
{
    STANDARD_CONSTRUCTOR()
    thePlayer = lancher;
    theWeapon = wpn;
    theTarget = tgt;
}

void SimLogger::KillEvent::initData()
{
    thePlayer = nullptr;
    theWeapon = nullptr;
    theTarget = nullptr;
}

// Copy data function
void SimLogger::KillEvent::copyData(const KillEvent& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();
    thePlayer = org.thePlayer;
    theWeapon = org.theWeapon;
    theTarget = org.theTarget;
}

// Delete data function
void SimLogger::KillEvent::deleteData()
{
    thePlayer = nullptr;
    theWeapon = nullptr;
    theTarget = nullptr;
}

// Get the description
const char* SimLogger::KillEvent::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " KILL_EVENT:";

        // Print the Player ID
        if (thePlayer != nullptr) {
            sout << " launcher";
            makePlayerIdMsg(sout, thePlayer);
        }

        // Print the WPN ID
        if (theWeapon != nullptr) {
            sout << " wpn";
            makePlayerIdMsg(sout, theWeapon);
        }

        // Print the TGT ID
        if (theTarget != nullptr) {
            sout << " tgt";
            makePlayerIdMsg(sout, theTarget);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::KillEvent::captureData()
{
}

//==============================================================================
// Class SimLogger::DetonationEvent
//==============================================================================
SIMLOGEVENT_B(DetonationEvent,"SimLogger::DetonationEvent")
EMPTY_SERIALIZER(SimLogger::DetonationEvent)

// Constructor
SimLogger::DetonationEvent::DetonationEvent(Player* const lancher, Player* const wpn, Player* const tgt, const unsigned int t, const LCreal d)
{
    STANDARD_CONSTRUCTOR()
    thePlayer = lancher;
    theWeapon = wpn;
    theTarget = tgt;
    detType   = t;
    missDist  = d;
}

void SimLogger::DetonationEvent::initData()
{
    thePlayer = nullptr;
    theWeapon = nullptr;
    theTarget = nullptr;
    detType   = 0;
    missDist  = 0;
}

// Copy data function
void SimLogger::DetonationEvent::copyData(const DetonationEvent& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();
    thePlayer = org.thePlayer;
    theWeapon = org.theWeapon;
    theTarget = org.theTarget;
    detType   = org.detType;
    missDist  = org.missDist;
}

// Delete data function
void SimLogger::DetonationEvent::deleteData()
{
    thePlayer = nullptr;
    theWeapon = nullptr;
    theTarget = nullptr;
}

// Get the description
const char* SimLogger::DetonationEvent::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " WPN_DET_EVENT:";

        // Print the Player ID
        if (thePlayer != nullptr) {
            sout << " launcher";
            makePlayerIdMsg(sout, thePlayer);
        }

        // Print the WPN ID
        if (theWeapon != nullptr) {
            sout << " wpn";
            makePlayerIdMsg(sout, theWeapon);
        }

        // Print the TGT ID
        if (theTarget != nullptr) {
            sout << " tgt";
            makePlayerIdMsg(sout, theTarget);
        }

        sout << " type: " << detType;
        sout << " missDist: " << missDist;

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::DetonationEvent::captureData()
{
}

//==============================================================================
// Class SimLogger::NewTrack
//==============================================================================
SIMLOGEVENT_B(NewTrack,"SimLogger::NewTrack")
EMPTY_SERIALIZER(SimLogger::NewTrack)

// Constructor
SimLogger::NewTrack::NewTrack(TrackManager* const mgr, Track* const trk)
{
    STANDARD_CONSTRUCTOR()

    initData();

    theManager = mgr;
    theTrack = trk;

    if (theManager != nullptr) {
        thePlayer = dynamic_cast<const Player*>( theManager->findContainerByType(typeid(Player)) );
    }
    if (theTrack != nullptr) {
        const RfTrack* const rfTrk = dynamic_cast<const RfTrack*>(trk);
        if (rfTrk != nullptr) {
            theEmission = rfTrk->getLastEmission();
        }
    }
}

void SimLogger::NewTrack::initData()
{
    thePlayer = nullptr;
    theEmission = nullptr;
    theManager = nullptr;
    theTrack = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
    tgtPos.set(0,0,0);
    tgtVel.set(0,0,0);
    tgtAngles.set(0,0,0);
    sn = 0.0;
}

// Copy data function
void SimLogger::NewTrack::copyData(const NewTrack& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    theEmission = org.theEmission;
    theManager = org.theManager;
    theTrack = org.theTrack;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
    tgtPos = org.tgtPos;
    tgtVel = org.tgtVel;
    tgtAngles = org.tgtAngles;
    sn = org.sn;
}

// Delete data function
void SimLogger::NewTrack::deleteData()
{
    theManager = nullptr;
    theTrack = nullptr;
    thePlayer = nullptr;
    theEmission = nullptr;
}

// Get the description
const char* SimLogger::NewTrack::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " ADDED_TRACK:\n";

        // Player information
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout, thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Target Information
        if (theEmission != nullptr) {
            if (theEmission->getTarget() != nullptr) {
                sout << "\tTarget";
                makePlayerIdMsg(sout, theEmission->getTarget());
                makePlayerDataMsg(sout,tgtPos,tgtVel,tgtAngles);
                sout << "\n";
            }
        }

        // General track information
        if (theTrack != nullptr) {
            makeTrackDataMsg(sout, theTrack);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::NewTrack::captureData()
{
    if (thePlayer != nullptr) {
        {
            pos = thePlayer->getPosition();
            vel = thePlayer->getVelocity();
            angles = thePlayer->getEulerAngles();
        }
    }
    if (theEmission != nullptr) {
        if (theEmission->getTarget() != nullptr) {
            {
                tgtPos = theEmission->getTarget()->getPosition();
                tgtVel = theEmission->getTarget()->getVelocity();
                tgtAngles = theEmission->getTarget()->getEulerAngles();
            }
        }
    }
}

//==============================================================================
// Class SimLogger::UpdateTrack
//==============================================================================
SIMLOGEVENT_B(UpdateTrack,"SimLogger::UpdateTrack")
EMPTY_SERIALIZER(SimLogger::UpdateTrack)

// Constructor
SimLogger::UpdateTrack::UpdateTrack(TrackManager* const mgr, Track* const trk)
{
    STANDARD_CONSTRUCTOR()

    initData();

    theManager = mgr;
    theTrack = trk;
    if (theManager != nullptr) {
        thePlayer = dynamic_cast<const Player*>( theManager->findContainerByType(typeid(Player)) );
    }
    if (theTrack != nullptr) {
        const RfTrack* const rfTrk = dynamic_cast<const RfTrack*>(trk);
        if (rfTrk != nullptr) {
            theEmission = rfTrk->getLastEmission();
        }
    }
}

void SimLogger::UpdateTrack::initData()
{
    thePlayer = nullptr;
    theEmission = nullptr;
    theManager = nullptr;
    theTrack = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
    tgtPos.set(0,0,0);
    tgtVel.set(0,0,0);
    tgtAngles.set(0,0,0);
    sn = 0.0;
}


// Copy data function
void SimLogger::UpdateTrack::copyData(const UpdateTrack& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    theEmission = org.theEmission;
    theManager = org.theManager;
    theTrack = org.theTrack;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
    tgtPos = org.tgtPos;
    tgtVel = org.tgtVel;
    tgtAngles = org.tgtAngles;
    sn = org.sn;
}

// Delete data function
void SimLogger::UpdateTrack::deleteData()
{
    theManager = nullptr;
    theTrack = nullptr;
    thePlayer = nullptr;
    theEmission = nullptr;
}

// Get the description
const char* SimLogger::UpdateTrack::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " UPDATE_TRACK:\n";

        // Player information
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout, thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Target Information
        if (theEmission != nullptr) {
            if (theEmission->getTarget() != nullptr) {
                sout << "\tTarget";
                makePlayerIdMsg(sout, theEmission->getTarget());
                makePlayerDataMsg(sout,tgtPos,tgtVel,tgtAngles);
                sout << "\n";
            }
        }

        // General track information
        if (theTrack != nullptr) {
            makeTrackDataMsg(sout, theTrack);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::UpdateTrack::captureData()
{
    if (thePlayer != nullptr) {
        {
            pos = thePlayer->getPosition();
            vel = thePlayer->getVelocity();
            angles = thePlayer->getEulerAngles();
        }
    }
    if (theEmission != nullptr) {
        if (theEmission->getTarget() != nullptr) {
            {
                tgtPos = theEmission->getTarget()->getPosition();
                tgtVel = theEmission->getTarget()->getVelocity();
                tgtAngles = theEmission->getTarget()->getEulerAngles();
            }
        }
    }
}

//==============================================================================
// Class SimLogger::RemovedTrack
//==============================================================================
SIMLOGEVENT_B(RemovedTrack,"SimLogger::RemovedTrack")
EMPTY_SERIALIZER(SimLogger::RemovedTrack)

// Constructor
SimLogger::RemovedTrack::RemovedTrack(TrackManager* const mgr, Track* const trk)
{
    STANDARD_CONSTRUCTOR()

    initData();

    theManager = mgr;
    theTrack = trk;
    if (theManager != nullptr) {
        thePlayer = dynamic_cast<const Player*>( theManager->findContainerByType(typeid(Player)) );
    }
    if (theTrack != nullptr) {
        const RfTrack* const rfTrk = dynamic_cast<const RfTrack*>(trk);
        if (rfTrk != nullptr) {
            theEmission = rfTrk->getLastEmission();
        }
    }
}

void SimLogger::RemovedTrack::initData()
{
    thePlayer = nullptr;
    theEmission = nullptr;
    theManager = nullptr;
    theTrack = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
    tgtPos.set(0,0,0);
    tgtVel.set(0,0,0);
    tgtAngles.set(0,0,0);
    sn = 0.0;
}

// Copy data function
void SimLogger::RemovedTrack::copyData(const RemovedTrack& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    theEmission = org.theEmission;
    theManager = org.theManager;
    theTrack = org.theTrack;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
    tgtPos = org.tgtPos;
    tgtVel = org.tgtVel;
    tgtAngles = org.tgtAngles;
    sn = org.sn;
}

// Delete data function
void SimLogger::RemovedTrack::deleteData()
{
    theManager = nullptr;
    theTrack = nullptr;
    thePlayer = nullptr;
    theEmission = nullptr;
}

// Get the description
const char* SimLogger::RemovedTrack::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " REMOVE_TRACK:\n";

        // Player information
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout, thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Target Information
        if (theEmission != nullptr) {
            if (theEmission->getTarget() != nullptr) {
                sout << "\tTarget";
                makePlayerIdMsg(sout, theEmission->getTarget());
                makePlayerDataMsg(sout,tgtPos,tgtVel,tgtAngles);
                sout << "\n";
            }
        }

        // General track information
        if (theTrack != nullptr) {
            makeTrackDataMsg(sout, theTrack);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::RemovedTrack::captureData()
{
    if (thePlayer != nullptr) {
        pos = thePlayer->getPosition();
        vel = thePlayer->getVelocity();
        angles = thePlayer->getEulerAngles();
    }
    if (theEmission != nullptr) {
        if (theEmission->getTarget() != nullptr) {
            tgtPos = theEmission->getTarget()->getPosition();
            tgtVel = theEmission->getTarget()->getVelocity();
            tgtAngles = theEmission->getTarget()->getEulerAngles();
        }
    }
}

//==============================================================================
// Class SimLogger::NewRwrTrack
//==============================================================================
SIMLOGEVENT_B(NewRwrTrack,"SimLogger::NewRwrTrack")
EMPTY_SERIALIZER(SimLogger::NewRwrTrack)

// Constructor
SimLogger::NewRwrTrack::NewRwrTrack(TrackManager* const mgr, Track* const trk)
{
    STANDARD_CONSTRUCTOR()

    initData();

    theManager = mgr;
    theTrack = trk;
    if (theManager != nullptr) {
        thePlayer = dynamic_cast<const Player*>( theManager->findContainerByType(typeid(Player)) );
    }
    if (theTrack != nullptr) {
        const RfTrack* const rfTrk = dynamic_cast<const RfTrack*>(trk);
        if (rfTrk != nullptr) {
            theEmission = rfTrk->getLastEmission();
        }
    }
}

void SimLogger::NewRwrTrack::initData()
{
    thePlayer = nullptr;
    theEmission = nullptr;
    theManager = nullptr;
    theTrack = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
    tgtPos.set(0,0,0);
    tgtVel.set(0,0,0);
    tgtAngles.set(0,0,0);
    sn = 0.0;
}

// Copy data function
void SimLogger::NewRwrTrack::copyData(const NewRwrTrack& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    theEmission = org.theEmission;
    theManager = org.theManager;
    theTrack = org.theTrack;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
    tgtPos = org.tgtPos;
    tgtVel = org.tgtVel;
    tgtAngles = org.tgtAngles;
    sn = org.sn;
}

// Delete data function
void SimLogger::NewRwrTrack::deleteData()
{
    theManager = nullptr;
    theTrack = nullptr;
    thePlayer = nullptr;
    theEmission = nullptr;
}

// Get the description
const char* SimLogger::NewRwrTrack::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " ADDED_RWR_TRACK:\n";

        // Player information
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout, thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Target Information
        if (theEmission != nullptr) {
            if (theEmission->getOwnship() != nullptr) {
                sout << "\tTarget";
                makePlayerIdMsg(sout, theEmission->getOwnship());
                makePlayerDataMsg(sout,tgtPos,tgtVel,tgtAngles);
                sout << "\n";
            }
            // General emission information
            makeEmissionDataMsg(sout, theEmission);
        }

        // General track information
        if (theTrack != nullptr) {
            makeTrackDataMsg(sout, theTrack);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::NewRwrTrack::captureData()
{
    if (thePlayer != nullptr) {
        {
            pos = thePlayer->getPosition();
            vel = thePlayer->getVelocity();
            angles = thePlayer->getEulerAngles();
        }
    }
    if (theEmission != nullptr) {
        // The emission's ownship is our target!
        if (theEmission->getOwnship() != nullptr) {
            {
                tgtPos = theEmission->getOwnship()->getPosition();
                tgtVel = theEmission->getOwnship()->getVelocity();
                tgtAngles = theEmission->getOwnship()->getEulerAngles();
            }
        }
    }
}

//==============================================================================
// Class SimLogger::UpdateRwrTrack
//==============================================================================
SIMLOGEVENT_B(UpdateRwrTrack,"SimLogger::UpdateRwrTrack")
EMPTY_SERIALIZER(SimLogger::UpdateRwrTrack)

// Constructor
SimLogger::UpdateRwrTrack::UpdateRwrTrack(TrackManager* const mgr, Track* const trk)
{
    STANDARD_CONSTRUCTOR()

    initData();

    theManager = mgr;
    theTrack = trk;
    if (theManager != nullptr) {
        thePlayer = dynamic_cast<const Player*>( theManager->findContainerByType(typeid(Player)) );
    }
    if (theTrack != nullptr) {
        const RfTrack* const rfTrk = dynamic_cast<const RfTrack*>(trk);
        if (rfTrk != nullptr) {
            theEmission = rfTrk->getLastEmission();
        }
    }
}

void SimLogger::UpdateRwrTrack::initData()
{
    thePlayer = nullptr;
    theEmission = nullptr;
    theManager = nullptr;
    theTrack = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
    tgtPos.set(0,0,0);
    tgtVel.set(0,0,0);
    tgtAngles.set(0,0,0);
    sn = 0.0;
}

// Copy data function
void SimLogger::UpdateRwrTrack::copyData(const UpdateRwrTrack& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    theEmission = org.theEmission;
    theManager = org.theManager;
    theTrack = org.theTrack;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
    tgtPos = org.tgtPos;
    tgtVel = org.tgtVel;
    tgtAngles = org.tgtAngles;
    sn = org.sn;
}

// Delete data function
void SimLogger::UpdateRwrTrack::deleteData()
{
    theManager = nullptr;
    theTrack = nullptr;
    thePlayer = nullptr;
    theEmission = nullptr;
}

// Get the description
const char* SimLogger::UpdateRwrTrack::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " UPDATE_RWR_TRACK:\n";

        // Player information
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout, thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Target Information
        if (theEmission != nullptr) {
            if (theEmission->getOwnship() != nullptr) {
                sout << "\tTarget";
                makePlayerIdMsg(sout, theEmission->getOwnship());
                makePlayerDataMsg(sout,tgtPos,tgtVel,tgtAngles);
                sout << "\n";
            }
            // General emission information
            makeEmissionDataMsg(sout, theEmission);
        }

        // General track information
        if (theTrack != nullptr) {
            makeTrackDataMsg(sout, theTrack);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::UpdateRwrTrack::captureData()
{
    if (thePlayer != nullptr) {
        {
            pos = thePlayer->getPosition();
            vel = thePlayer->getVelocity();
            angles = thePlayer->getEulerAngles();
        }
    }
    if (theEmission != nullptr) {
        // The emission's ownship is our target!
        if (theEmission->getOwnship() != nullptr) {
            {
                tgtPos = theEmission->getOwnship()->getPosition();
                tgtVel = theEmission->getOwnship()->getVelocity();
                tgtAngles = theEmission->getOwnship()->getEulerAngles();
            }
        }
    }
}

//==============================================================================
// Class SimLogger::RemovedRwrTrack
//==============================================================================
SIMLOGEVENT_B(RemovedRwrTrack,"SimLogger::RemovedRwrTrack")
EMPTY_SERIALIZER(SimLogger::RemovedRwrTrack)

// Constructor
SimLogger::RemovedRwrTrack::RemovedRwrTrack(TrackManager* const mgr, Track* const trk)
{
    STANDARD_CONSTRUCTOR()

    initData();

    theManager = mgr;
    theTrack = trk;
    if (theManager != nullptr) {
        thePlayer = dynamic_cast<const Player*>( theManager->findContainerByType(typeid(Player)) );
    }
    if (theTrack != nullptr) {
        const RfTrack* const rfTrk = dynamic_cast<const RfTrack*>(trk);
        if (rfTrk != nullptr) {
            theEmission = rfTrk->getLastEmission();
        }
    }
}

void SimLogger::RemovedRwrTrack::initData()
{
    thePlayer = nullptr;
    theEmission = nullptr;
    theManager = nullptr;
    theTrack = nullptr;
    pos.set(0,0,0);
    vel.set(0,0,0);
    angles.set(0,0,0);
    tgtPos.set(0,0,0);
    tgtVel.set(0,0,0);
    tgtAngles.set(0,0,0);
    sn = 0.0;
}

// Copy data function
void SimLogger::RemovedRwrTrack::copyData(const RemovedRwrTrack& org, const bool cc)
{
    BaseClass::copyData(org);
    if (cc) initData();

    thePlayer = org.thePlayer;
    theEmission = org.theEmission;
    theManager = org.theManager;
    theTrack = org.theTrack;
    pos = org.pos;
    vel = org.vel;
    angles = org.angles;
    tgtPos = org.tgtPos;
    tgtVel = org.tgtVel;
    tgtAngles = org.tgtAngles;
    sn = org.sn;
}

// Delete data function
void SimLogger::RemovedRwrTrack::deleteData()
{
    theManager = nullptr;
    theTrack = nullptr;
    thePlayer = nullptr;
    theEmission = nullptr;
}

// Get the description
const char* SimLogger::RemovedRwrTrack::getDescription()
{
    if (msg == nullptr) {

        std::stringstream sout;

        // Time & Event message
        makeTimeMsg(sout);
        sout << " REMOVE_RWR_TRACK:\n";

        // Player information
        if (thePlayer != nullptr) {
            sout << "\tPlayer";
            makePlayerIdMsg(sout, thePlayer);
            makePlayerDataMsg(sout,pos,vel,angles);
            sout << "\n";
        }

        // Target Information
        if (theEmission != nullptr) {
            if (theEmission->getOwnship() != nullptr) {
                sout << "\tTarget";
                makePlayerIdMsg(sout, theEmission->getOwnship());
                makePlayerDataMsg(sout,tgtPos,tgtVel,tgtAngles);
                sout << "\n";
            }
            // General emission information
            makeEmissionDataMsg(sout, theEmission);
        }

        // General track information
        if (theTrack != nullptr) {
            makeTrackDataMsg(sout, theTrack);
        }

        // Complete the description
        const int len = static_cast<int>(sout.str().size());
        msg = new char[len+1];
        lcStrncpy(msg, (len+1), sout.str().c_str(), len);
    }
    return msg;
}

// Capture the data
void SimLogger::RemovedRwrTrack::captureData()
{
    if (thePlayer != nullptr) {
        pos = thePlayer->getPosition();
        vel = thePlayer->getVelocity();
        angles = thePlayer->getEulerAngles();
    }
    if (theEmission != nullptr) {
        // The emission's ownship is our target!
        if (theEmission->getOwnship() != nullptr) {
            tgtPos = theEmission->getOwnship()->getPosition();
            tgtVel = theEmission->getOwnship()->getVelocity();
            tgtAngles = theEmission->getOwnship()->getEulerAngles();
        }
    }
}

} // End Simulation namespace
} // End Eaagles namespace
