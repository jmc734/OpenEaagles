
#include "openeaagles/basic/units/AngularVelocity.h"
#include "openeaagles/basic/units/Angles.h"
#include "openeaagles/basic/units/Times.h"
#include "openeaagles/basic/SlotTable.h"

namespace Eaagles {
namespace Basic {

//------------------------------------------------------------------------------
// AngularVelocity
//------------------------------------------------------------------------------
IMPLEMENT_SUBCLASS(AngularVelocity, "AngularVelocity")
EMPTY_SERIALIZER(AngularVelocity)

//------------------------------------------------------------------------------
// Slot Table:
//------------------------------------------------------------------------------
BEGIN_SLOTTABLE(AngularVelocity)
    "angle",    // 1: angle
    "time",     // 2: time
END_SLOTTABLE(AngularVelocity)

// Map slot table to handles
BEGIN_SLOT_MAP(AngularVelocity)
    ON_SLOT(1, setSlotAngle, Angle)
    ON_SLOT(2, setSlotTime, Time)
END_SLOT_MAP()

//------------------------------------------------------------------------------
// Constructors:
//------------------------------------------------------------------------------
AngularVelocity::AngularVelocity()
{
    STANDARD_CONSTRUCTOR()

    //Set a default angle, time, and angularVelocity
    angle = 1;
    time = 1;
    val = 1;
}

AngularVelocity::AngularVelocity(const LCreal newAngularVelocityRadiansPerSec)
{
    STANDARD_CONSTRUCTOR()

    //Set the angle to the input and angularVelocity to the input and make seconds 1 to get radians per second:
    angle = newAngularVelocityRadiansPerSec;
    time = 1;
    val = newAngularVelocityRadiansPerSec;
}

AngularVelocity::AngularVelocity(const Angle* const newAngle, const Time* const newTime)
{
    STANDARD_CONSTRUCTOR()

    //Set a default angle, time, and angularVelocity
    angle = 1;
    time = 1;
    val = 1;

    //Set Checks to false:
    bool okAngle = false;
    bool okTime = false;

    //Check and convert the angle to radians
    if (newAngle != nullptr)
    {
        LCreal finalAngle = static_cast<LCreal>(Radians::convertStatic(*newAngle));
        okAngle = setRadians(finalAngle);
    }

    //Check and convert the time to seconds
    if (newTime != nullptr)
    {
        LCreal finaltime = Seconds::convertStatic( *newTime );
        okTime = setSeconds(finaltime);
    }

    //Check that both were set correctly - if not give error:
    if ( !okTime || !okAngle )
    {
        //Give error if something was not set correctly:
        std::cerr << "Angle or Time not set correctly - new AngularVelocity Object bad." << std::endl;

    }

}

//------------------------------------------------------------------------------
// getAngularVelocity() -- returns Angular Velocity:
//------------------------------------------------------------------------------
LCreal AngularVelocity::getRadiansPerSecond() const
{
    return static_cast<LCreal>(val);
}

//------------------------------------------------------------------------------
// convert() -- converts from radians/sec to desired angle/time ratio:
//    NOTE: This ignores values of input - only the object type is used
//------------------------------------------------------------------------------
LCreal AngularVelocity::convert(Angle* newAngleUnit, Time* newTimeUnit)
{

    //Init a num to -1 as a check:
    LCreal desiredAngle = -1.0f;
    LCreal desiredTime = -1.0f;
    LCreal desiredResult = -1.0f;

    //Set input opject's internal value to 1 as a precaution:
    newAngleUnit->setValue(1);
    newTimeUnit->setValue(1);

    //Take the internal unit and create an object of Angle to convert angles:
    Radians* internalRadians = new Radians(static_cast<LCreal>(angle));

    //Find out what units the angle is in:
    if (dynamic_cast<Degrees*>(newAngleUnit) != nullptr)
    {
        //New angle is in degrees:
        Degrees* degrees = new Degrees;
        desiredAngle = static_cast<LCreal>(degrees->convert(*internalRadians));
        degrees->unref();
    }
    else if (dynamic_cast<Radians*>(newAngleUnit) != nullptr)
    {
        //New angle is in radians:
        desiredAngle = angle;
    }
    else if (dynamic_cast<Semicircles*>(newAngleUnit) != nullptr)
    {
        //New angle is in semicircles:
        Semicircles* semicircles = new Semicircles;
        desiredAngle = static_cast<LCreal>(semicircles->convert(*internalRadians));
        semicircles->unref();
    }
    else
    {
        //Give Error - Not sure what type it is:
        std::cerr << "Angle Conversion Type Not Found." << std::endl;
    }
    internalRadians->unref();

    //Find out what units the time input is in - do not use built in convert - very easy to do by hand:
    Seconds* q = dynamic_cast<Seconds*>(newTimeUnit);
    if(q != nullptr)
    {
        desiredTime = time;
    }
    else if(dynamic_cast<MilliSeconds*>(newTimeUnit) != nullptr)
    {
        //Time in milliseconds:
        desiredTime = time*1000;
    }
    else if(dynamic_cast<Minutes*>(newTimeUnit) != nullptr)
    {
        //Time in minutes:
        desiredTime = time/60;
    }
    else if(dynamic_cast<Hours*>(newTimeUnit) != nullptr)
    {
        //Time in hours:
        desiredTime = time/3600;
    }
    else if(dynamic_cast<Days*>(newTimeUnit) != nullptr)
    {
        //Time in days:
        desiredTime = time/86400;
    }
    else
    {
        //Give Error - Not sure what type it is:
        std::cerr << "Time Conversion Type Not Found." << std::endl;
    };

    desiredResult = desiredAngle/desiredTime;

    return desiredResult;

}

//------------------------------------------------------------------------------
// setRadiansPerSecond() -- sets our angularVelocity:
//------------------------------------------------------------------------------
bool AngularVelocity::setRadiansPerSecond(const LCreal newAngularVelocity)
{

    //Set angle and time - units in radians per second -> num = input; den = 1:
    bool ok1 = setRadians(newAngularVelocity);
    bool ok2 = setSeconds(1);

    //Check both values for ok:
    ok1 = (ok1)&&(ok2);

    return ok1;
}

//------------------------------------------------------------------------------
// setSlotAngle() -- sets angle based on input object and its value:
//------------------------------------------------------------------------------
bool AngularVelocity::setSlotAngle(const Angle* const msg)
{
    bool ok = false;

    //Try to convert Number to an angle:
    if(msg != nullptr)
    {
        LCreal finalNumber = static_cast<LCreal>(Radians::convertStatic(*msg));
        ok = setRadians(finalNumber);
    }
    return ok;
}

//------------------------------------------------------------------------------
// setSlotTime() -- sets time based on input object and its value:
//------------------------------------------------------------------------------
bool AngularVelocity::setSlotTime(const Time* const msg)
{
    bool ok = false;

    //Try to convert Number to a time:
    if(msg != nullptr)
    {
        LCreal finalNumber = Seconds::convertStatic(*msg);
        ok = setSeconds(finalNumber);
    }
    return ok;
}

//------------------------------------------------------------------------------
// setDegrees() -- sets angle of object in (degrees):
//------------------------------------------------------------------------------
bool AngularVelocity::setDegrees(const LCreal newAngle)
{
    //Set the angle in radians:
    bool ok = setRadians( newAngle * static_cast<LCreal>(Angle::D2RCC) );

    return ok;
}

//------------------------------------------------------------------------------
// setRadians() -- sets angle of object in (radians):
//------------------------------------------------------------------------------
bool AngularVelocity::setRadians(const LCreal newAngle)
{
    angle = newAngle;
    //Update new angular velocity:
    val = angle/time;
    return true;
}

//------------------------------------------------------------------------------
// setTime() -- sets time of object in (seconds):
//------------------------------------------------------------------------------
bool AngularVelocity::setSeconds(const LCreal newTime)
{
    //Set Time:
    time = newTime;
    //Update new angular velocity:
    val = angle/time;
    return true;
}

//------------------------------------------------------------------------------
// getSlotByIndex() for AngularVelocity
//------------------------------------------------------------------------------
Object* AngularVelocity::getSlotByIndex(const int si)
{
    return BaseClass::getSlotByIndex(si);
}

//------------------------------------------------------------------------------
// copyData() -- copy this object's data:
//------------------------------------------------------------------------------
void AngularVelocity::copyData(const AngularVelocity& org, const bool)
{
    BaseClass::copyData(org);

    angle = org.angle;
    time = org.time;
}

//------------------------------------------------------------------------------
// deleteData() -- delete this object's data:
//------------------------------------------------------------------------------
void AngularVelocity::deleteData()
{
}

} // End Basic namespace
} // End Eaagles namespace
