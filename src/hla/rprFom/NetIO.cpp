//------------------------------------------------------------------------------
// Class: NetIO
//------------------------------------------------------------------------------

#include "openeaagles/hla/rprFom/NetIO.h"
#include "openeaagles/hla/rprFom/Nib.h"
#include "openeaagles/hla/rprFom/Ntm.h"
#include "openeaagles/hla/rprFom/RprFom.h"
#include "openeaagles/hla/Ambassador.h"

#include "openeaagles/simulation/AirVehicle.h"
#include "openeaagles/simulation/GroundVehicle.h"
#include "openeaagles/simulation/LifeForms.h"
#include "openeaagles/simulation/Missile.h"
#include "openeaagles/simulation/Player.h"
#include "openeaagles/simulation/Ships.h"
#include "openeaagles/simulation/Simulation.h"
#include "openeaagles/simulation/Signatures.h"
#include "openeaagles/simulation/Weapon.h"
#include "openeaagles/basic/Pair.h"
#include "openeaagles/basic/PairStream.h"
#include "openeaagles/basic/String.h"
#include "openeaagles/basic/Nav.h"
#include "openeaagles/basic/NetHandler.h"
#include "openeaagles/basic/Number.h"

namespace Eaagles {
namespace Network {
namespace Hla {
namespace RprFom {

//==============================================================================
// Class: Hla::RprFom::NtmInputNode
// Description: RPR FOM incoming NTM class
//==============================================================================

class NtmInputNode : public Simulation::NetIO::NtmInputNode {
   DECLARE_SUBCLASS(NtmInputNode,Simulation::NetIO::NtmInputNode)

public:
   enum { ROOT_LVL, KIND_LVL, DOMAIN_LVL, COUNTRYCODE_LVL,
         CATEGORY_LVL, SUBCATEGORY_LVL, SPECIFIC_LVL, EXTRA_LVL };

public:
   NtmInputNode(const unsigned int level, const unsigned int code, const Ntm* ntm = 0);

   virtual const Ntm* findNtmByTypeCodes(
         const unsigned char  kind,
         const unsigned char  domain,
         const unsigned short countryCode,
         const unsigned char  category,
         const unsigned char  subcategory = 0,
         const unsigned char  specific = 0,
         const unsigned char  extra = 0
      ) const;

   // NetIO::NtmOutputNode class functions
   virtual const Simulation::Ntm* findNetworkTypeMapper(const Simulation::Nib* const nib) const;
   virtual bool add2OurLists(Simulation::Ntm* const ntm);
   virtual void print(std::ostream& sout, const int icnt) const;

private:
   unsigned int level;        // Level
   unsigned int code;         // Code for this level
   const Ntm* ourNtm;         // Our default NTM
   Basic::List* subnodeList;  // List of NtmInputNode nodes below this level
};


//==============================================================================
// Class: Hla::RprFom::NetIO
//==============================================================================

IMPLEMENT_SUBCLASS(NetIO, "RprFomNetIO")
EMPTY_SLOTTABLE(NetIO)
EMPTY_SERIALIZER(NetIO)

//------------------------------------------------------------------------------
// Constructors, destructor, copy operator and clone()
//------------------------------------------------------------------------------
NetIO::NetIO()
{
   STANDARD_CONSTRUCTOR()
}

//------------------------------------------------------------------------------
// copyData() -- copy member data
//------------------------------------------------------------------------------
void NetIO::copyData(const NetIO& org, const bool)
{
    BaseClass::copyData(org);
}

//------------------------------------------------------------------------------
// deleteData() -- delete member data
//------------------------------------------------------------------------------
void NetIO::deleteData()
{
}

//------------------------------------------------------------------------------
// getNumberOfObjectClasses() -- Number of object classes we're using
//------------------------------------------------------------------------------
unsigned int NetIO::getNumberOfObjectClasses() const
{
    return NUM_OBJECT_CLASSES;
}

//------------------------------------------------------------------------------
// getNumberOfObjectAttributes() -- Number of object attributes we're using
//------------------------------------------------------------------------------
unsigned int NetIO::getNumberOfObjectAttributes() const
{
    return NUM_OBJECT_ATTRIBUTES;
}

//------------------------------------------------------------------------------
// getNumberOfOInteractionClasses() -- Number of interaction classes we're using
//------------------------------------------------------------------------------
unsigned int NetIO::getNumberOfOInteractionClasses() const
{
    return NUM_INTERACTION_CLASSES;
}

//------------------------------------------------------------------------------
// getNumberOfInteractionParameters() -- Number of interaction parameters we're using
//------------------------------------------------------------------------------
unsigned int NetIO::getNumberOfInteractionParameters() const
{
    return NUM_INTERACTION_PARAMETER;
}

//------------------------------------------------------------------------------
// Create a new output NIB
//------------------------------------------------------------------------------
Simulation::Nib* NetIO::createNewOutputNib(Simulation::Player* const player)
{
    // ---
    // Check if we are enabled to register this class of objects and 
    // create the proper FOM class structure
    // ---
    unsigned int idx = 0;
    BaseEntity* baseEntity = 0;
    if (player->isClassType(typeid(Simulation::AirVehicle))) {
        if (isObjectClassRegistrationEnabled( AIRCRAFT_CLASS )) {
            baseEntity = new Aircraft();
            idx = AIRCRAFT_CLASS;
        }
    }
    else if (player->isClassType(typeid(Simulation::Missile))) {
        if (isObjectClassRegistrationEnabled( MUNITION_CLASS )) {
            baseEntity = new Munition();
            idx = MUNITION_CLASS;
        }
    }
    else if (player->isClassType(typeid(Simulation::LifeForm))) {
        if (isObjectClassRegistrationEnabled( HUMAN_CLASS )) {
            baseEntity = new Human();
            idx = HUMAN_CLASS;
        }
    }
    else if (player->isClassType(typeid(Simulation::GroundVehicle))) {
        if (isObjectClassRegistrationEnabled( GROUND_VEHICLE_CLASS )) {
            baseEntity = new GroundVehicle();
            idx = GROUND_VEHICLE_CLASS;
        }
    }
    else if (player->isClassType(typeid(Simulation::Ship))) {
        if (isObjectClassRegistrationEnabled( SURFACE_VESSEL_CLASS )) {
            baseEntity = new SurfaceVessel();
            idx = SURFACE_VESSEL_CLASS;
        }
    }

    // ---
    // If enabled, create and set the output NIB
    // ---
    Nib* nib = 0;
    if (baseEntity != 0) {
        nib = (Nib*) nibFactory(Simulation::NetIO::OUTPUT_NIB);
        if (nib != 0) {
           nib->setBaseEntity(baseEntity);
           nib->setNetIO(this);
           nib->setPlayer(player);
           nib->setClassIndex(idx);
           nib->setOutputPlayerType(player);
        }
        baseEntity->unref();  // the NIB should have it
    }
    return nib;
}


//------------------------------------------------------------------------------
// publishAndSubscribe()
//------------------------------------------------------------------------------
bool NetIO::publishAndSubscribe()
{
   bool ok = true;

   // ----------
   // We want the callbacks for attribute level advisories
   // ----------
   try {
      std::cout << "*** Enable Attribute Advisories" << std::endl;
      getRTIambassador()->enableAttributeRelevanceAdvisorySwitch();
   }
   catch (RTI::Exception& e) {
      std::cerr << &e << std::endl;
      ok = false;
   }

   // ----------
   // Get handles to the class, attributes and parameters
   // and publish & Subscribe to class attributes
   // ----------

   if (ok) ok = publishAndSubscribePlatforms();
   if (ok) ok = publishAndSubscribeWeaponFire();
   if (ok) ok = publishAndSubscribeMunitionDetonation();

   return ok;
}

//------------------------------------------------------------------------------
// processInputList() -- Update players/systems from the Input-list
//------------------------------------------------------------------------------
void NetIO::processInputList()
{
   for (unsigned int idx = 0; idx < getInputListSize(); idx++) {
      Nib* nib = (Nib*)( getInputNib(idx) );
      if (nib != 0) nib->updateTheIPlayer();
   }
}

//------------------------------------------------------------------------------
// nibFactory() -- Create a new NIB
//------------------------------------------------------------------------------
Simulation::Nib* NetIO::nibFactory(const Simulation::NetIO::IoType ioType)
{
    return new Nib(ioType);
}

//------------------------------------------------------------------------------
// discoverObjectInstance() -- handle the discover of an object
//------------------------------------------------------------------------------
void NetIO::discoverObjectInstance(
        const RTI::ObjectHandle theObject,
        const RTI::ObjectClassHandle theObjectClass,
        const char* theObjectName)
{
   BaseEntity* baseEntity = 0;

   unsigned int idx = findObjectClassIndex(theObjectClass);
   switch (idx) {
      case AIRCRAFT_CLASS :
         baseEntity = new Aircraft();
         break;
      case MUNITION_CLASS :
         baseEntity = new Munition();
         break;
      case HUMAN_CLASS :
         baseEntity = new Human();
         break;
      case GROUND_VEHICLE_CLASS :
         baseEntity = new GroundVehicle();
         break;
      case SURFACE_VESSEL_CLASS :
         baseEntity = new SurfaceVessel();
         break;
   };

   if (baseEntity != 0) {
      Nib* nib = dynamic_cast<Nib*>( createNewInputNib() );
      if (nib != 0) {
         nib->setObjectHandle(theObject);
         nib->setObjectName(theObjectName);
         nib->setClassIndex(idx);
         nib->setTimeExec( (LCreal) getCurrentTime() );
         nib->setBaseEntity(baseEntity);
         addNib2InputList(nib);
         addNibToObjectTables(nib, INPUT_NIB);
         nib->unref();
      }
      baseEntity->unref();  // (NIB has it now)
   }
}

//------------------------------------------------------------------------------
// receiveInteraction() -- handle the discover of an object
//------------------------------------------------------------------------------
void NetIO::receiveInteraction(
    const RTI::InteractionClassHandle theInteraction,
    const RTI::ParameterHandleValuePairSet& theParameters)
{
    // Select the proper method to handle this interaction
    switch( findInteractionClassIndex(theInteraction) ) {
    
        case WEAPON_FIRE_INTERACTION :
            receiveWeaponFire(theParameters);
            break;
            
        case MUNITION_DETONATION_INTERACTION :
            receiveMunitionDetonation(theParameters);
            break;
            
    };
}

//------------------------------------------------------------------------------
// Finds the Ntm by the entity type codes
//------------------------------------------------------------------------------
const Ntm* NetIO::findNtmByTypeCodes(
         const unsigned char  kind,
         const unsigned char  domain,
         const unsigned short countryCode,
         const unsigned char  category,
         const unsigned char  subcategory,
         const unsigned char  specific,
         const unsigned char  extra
      ) const
{
   const Hla::RprFom::Ntm* result = 0;

   const Hla::RprFom::NtmInputNode* root = dynamic_cast<const Hla::RprFom::NtmInputNode*>( getRootNtmInputNode() );
   if (root != 0) {

      result = root->findNtmByTypeCodes(kind, domain, countryCode, category, subcategory, specific, extra);

   }
   return result;
}

//==============================================================================
// Class: Dis::NtmInputNode
// Description: DIS incoming NTM node
//==============================================================================

IMPLEMENT_SUBCLASS(NtmInputNode,"NtmInputNode")
EMPTY_SLOTTABLE(NtmInputNode)
EMPTY_SERIALIZER(NtmInputNode)

//------------------------------------------------------------------------------
// root incoming NTM node factory
//------------------------------------------------------------------------------
Simulation::NetIO::NtmInputNode* NetIO::rootNtmInputNodeFactory() const
{
   return new Hla::RprFom::NtmInputNode(Hla::RprFom::NtmInputNode::ROOT_LVL,0); // root level
}

//------------------------------------------------------------------------------
// Class support functions
//------------------------------------------------------------------------------
NtmInputNode::NtmInputNode(const unsigned int l, const unsigned int c, const Ntm* ntm)
   : level(l), code(c), ourNtm(0), subnodeList(0)
{
   STANDARD_CONSTRUCTOR()

   if (ntm != 0) {
      ourNtm = ntm;
      ourNtm->ref();
   }
   subnodeList = new Basic::List();
}

void NtmInputNode::copyData(const NtmInputNode& org, const bool cc)
{
   BaseClass::copyData(org);

   if (cc) {
      ourNtm = 0;
      subnodeList = 0;
   }

   level = org.level;
   code = org.code;

   if (ourNtm != 0) {
      ourNtm->unref();
      ourNtm = 0;
   }
   if (org.ourNtm != 0) {
      ourNtm = org.ourNtm->clone();
   }

   if (subnodeList != 0) {
      subnodeList->unref();
      subnodeList = 0;
   }
   if (org.subnodeList != 0) {
      subnodeList = org.subnodeList->clone();
   }
}

void NtmInputNode::deleteData()
{
   if (ourNtm != 0) {
      ourNtm->unref();
      ourNtm = 0;
   }

   if (subnodeList != 0) {
      subnodeList->unref();
      subnodeList = 0;
   }
}

//------------------------------------------------------------------------------
// Find the NTM based on the incoming entity type codes in the NIB
//------------------------------------------------------------------------------
const Simulation::Ntm* NtmInputNode::findNetworkTypeMapper(const Simulation::Nib* const nib) const
{
   const Simulation::Ntm* result = 0;

   const Hla::RprFom::Nib* rprFomNib = dynamic_cast<const Hla::RprFom::Nib*>( nib );
   if (rprFomNib != 0) {
      result = findNtmByTypeCodes(
            rprFomNib->getEntityKind(),
            rprFomNib->getEntityDomain(),
            rprFomNib->getEntityCountry(),
            rprFomNib->getEntityCategory(),
            rprFomNib->getEntitySubcategory(),
            rprFomNib->getEntitySpecific(),
            rprFomNib->getEntityExtra()
         );
   }
   return result;
}

//------------------------------------------------------------------------------
// Find the NTM based on the incoming entity type codes in the NIB
//------------------------------------------------------------------------------
const Ntm* NtmInputNode::findNtmByTypeCodes(
      const unsigned char  kind,
      const unsigned char  domain,
      const unsigned short countryCode,
      const unsigned char  category,
      const unsigned char  subcategory,
      const unsigned char  specific,
      const unsigned char  extra
   ) const
{
   const Ntm* result = 0;

   {
      // Yes we have the proper type of NIB ...

      // Make sure that the NIB's code for this level matches our code
      bool match = true;
      switch (level) {
         case ROOT_LVL :         match = true; break; // the 'root' node always matches
         case KIND_LVL :         match = (code == kind);         break;
         case DOMAIN_LVL :       match = (code == domain);       break;
         case COUNTRYCODE_LVL :  match = (code == countryCode);  break;
         case CATEGORY_LVL :     match = (code == category);     break;
         case SUBCATEGORY_LVL :  match = (code == subcategory);  break;
         case SPECIFIC_LVL :     match = (code == specific);     break;
         case EXTRA_LVL :        match = (code == extra);        break;
      }

      if (match) {

         // First, if we're not the last 'extra' level then search
         // our subnodes to see if they can find a match
         if (level < EXTRA_LVL) {
            const Basic::List::Item* item = subnodeList->getFirstItem();
            while (item != 0 && result == 0) {
               const NtmInputNode* subnode = (const NtmInputNode*) item->getValue();
               result = subnode->findNtmByTypeCodes(kind, domain, countryCode, category, subcategory, specific, extra);
               item = item->getNext();
            }
         }

         // Second, we can use our NTM object, but only if we're at the category
         // level or higher (i.e., we must have match at the kind, domain, country
         // code and category levels)
         if (result == 0 && level >= CATEGORY_LVL) {
            result = ourNtm;
         }
      }

   }

   return result;
}


//------------------------------------------------------------------------------
// Add the NTM to our sublist of nodes.
//------------------------------------------------------------------------------
bool NtmInputNode::add2OurLists(Simulation::Ntm* const ntm)
{
   bool ok = false;

   // Make sure we have the correct kind of NTM ...
   Hla::RprFom::Ntm* disNtm = dynamic_cast<Hla::RprFom::Ntm*>( ntm );
   if (disNtm != 0) {

      // Make sure that the NTM's code for this level matches our code
      unsigned int currLevelCode = 0;
      unsigned int nextLevelCode = 0;
      switch (level) {
         case ROOT_LVL : {
            currLevelCode = 0;
            nextLevelCode = disNtm->getEntityKind();
            break;
          }
         case KIND_LVL : {
            currLevelCode = disNtm->getEntityKind();
            nextLevelCode = disNtm->getEntityDomain();
            break;
          }
         case DOMAIN_LVL : {
            currLevelCode = disNtm->getEntityDomain();
            nextLevelCode = disNtm->getEntityCountry();
            break;
          }
         case COUNTRYCODE_LVL : {
            currLevelCode = disNtm->getEntityCountry();
            nextLevelCode = disNtm->getEntityCategory();
            break;
          }
         case CATEGORY_LVL : {
            currLevelCode = disNtm->getEntityCategory();
            nextLevelCode = disNtm->getEntitySubcategory();
            break;
          }
         case SUBCATEGORY_LVL : {
            currLevelCode = disNtm->getEntitySubcategory();
            nextLevelCode = disNtm->getEntitySpecific();
            break;
          }
         case SPECIFIC_LVL : {
            currLevelCode = disNtm->getEntitySpecific();
            nextLevelCode = disNtm->getEntityExtra();
            break;
          }
         case EXTRA_LVL : {
            currLevelCode = disNtm->getEntityExtra();
            nextLevelCode = 0;
            break;
          }
      }

      // Does our code match the NIB's entity type code for this level?
      // And the 'root' node always matches.
      bool match = (code == currLevelCode) || (level == ROOT_LVL);

      if (match) {
         bool err = false;

         // Case #1; if we're at the 'category' level or above, and all remaining codes are
         // zero, then this becomes a wild card terminal node.
         {
            bool wild = (level >= CATEGORY_LVL);

            if (wild && level < EXTRA_LVL)        wild = (disNtm->getEntityExtra() == 0);
            if (wild && level < SPECIFIC_LVL)     wild = (disNtm->getEntitySpecific() == 0);
            if (wild && level < SUBCATEGORY_LVL)  wild = (disNtm->getEntitySubcategory() == 0);

            if (wild) {
               // wild card terminal node
               if (ourNtm == 0) {
                  ourNtm = disNtm;
                  ourNtm->ref();
                  ok = true;
               }
               else if (isMessageEnabled(MSG_WARNING)) {
                  std::cerr << "Warning: duplicate incoming NTM(";
                  std::cerr << int(disNtm->getEntityKind()) << ",";
                  std::cerr << int(disNtm->getEntityDomain()) << ",";
                  std::cerr << int(disNtm->getEntityCountry()) << ",";
                  std::cerr << int(disNtm->getEntityCategory()) << ",";
                  std::cerr << int(disNtm->getEntitySubcategory()) << ",";
                  std::cerr << int(disNtm->getEntitySpecific()) << ",";
                  std::cerr << int(disNtm->getEntityExtra()) << ")";
                  std::cerr << ", second ignored" << std::endl;
                  err = true;
               }
            }
         }

         // Case #2; if we're at the 'specific' level, then create a terminal node
         // for the Ntm.  The wild card case was handle in case #1.
         if (!ok && !err && level == SPECIFIC_LVL) {

            // make sure the terminal node doesn't already exist.
            bool alreadyExists = false;
            const Basic::List::Item* item = subnodeList->getFirstItem();
            while (item != 0 && !alreadyExists) {
               NtmInputNode* subnode = (NtmInputNode*) item->getValue();
               alreadyExists = (nextLevelCode == subnode->code);
               item = item->getNext();
            }

            if (!alreadyExists) {
               NtmInputNode* newNode = new NtmInputNode( (level+1), nextLevelCode, disNtm );
               subnodeList->put(newNode);
               newNode->unref();   // ref()'d when put into the subnode list
               ok = true;
            }
            else if (isMessageEnabled(MSG_WARNING)) {
               std::cerr << "Warning: duplicate incoming NTM(";
               std::cerr << int(disNtm->getEntityKind()) << ",";
               std::cerr << int(disNtm->getEntityDomain()) << ",";
               std::cerr << int(disNtm->getEntityCountry()) << ",";
               std::cerr << int(disNtm->getEntityCategory()) << ",";
               std::cerr << int(disNtm->getEntitySubcategory()) << ",";
               std::cerr << int(disNtm->getEntitySpecific()) << ",";
               std::cerr << int(disNtm->getEntityExtra()) << ")";
               std::cerr << ", second ignored" << std::endl;
               err = true;
            }
         }

         // Case #3; if we're at a level less than the 'specific' level, so try
         // to add the NTM to one of our existing subnodes.
         if (!ok && !err && level < SPECIFIC_LVL) {
            const Basic::List::Item* item = subnodeList->getFirstItem();
            while (item != 0 && !ok) {
               NtmInputNode* subnode = (NtmInputNode*) item->getValue();
               if (nextLevelCode == subnode->code) {
                  ok = subnode->add2OurLists(disNtm);
               }
               item = item->getNext();
            }
         }

         // Case #4; We didn't create a terminal node, and the NTM was added to
         // one of our existing subnodes, then create a new subnode for it.
         if (!ok && !err) {
            // Create a new node and add the NTM
            NtmInputNode* newNode = new NtmInputNode( (level+1), nextLevelCode );
            subnodeList->put(newNode);
            ok = newNode->add2OurLists(disNtm);
            newNode->unref();   // ref()'d when put into the subnode list
         }
      }

   }

   return ok;
}

//------------------------------------------------------------------------------
// print our data and our subnodes
//------------------------------------------------------------------------------
void NtmInputNode::print(std::ostream& sout, const int icnt) const
{
   // Print our node's form name
   indent(sout,icnt);
   sout << "( NtmInputNode: level=" << level << ", code=" << code;
   sout << std::endl;

   // Print our Ntm object
   if (ourNtm != 0) {
      ourNtm->serialize(sout, icnt+4);
   }

   // Print our subnodes
   {
      const Basic::List::Item* item = subnodeList->getFirstItem();
      while (item != 0) {
         const NtmInputNode* subnode = (const NtmInputNode*) item->getValue();
         subnode->print(sout,icnt+4);
         item = item->getNext();
      }
   }

   indent(sout,icnt);
   sout << ")" << std::endl;
}


} // End RprFom namespace
} // End Hla namespace
} // End Network namespace
} // End Eaagles namespace
