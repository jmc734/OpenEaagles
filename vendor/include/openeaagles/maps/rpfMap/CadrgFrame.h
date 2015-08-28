//------------------------------------------------------------------------------
// Class: CadrgFrame
//
// Description: One frame of a TocEntry, holds the color table information for the
// texture. Load a frame's data and decompress it.
//
// load() - Load a frame's data.
//        virtual void load(CadrgFrameEntry* entry);
//
// decompressSubframe() - Take our frame and decompress the subframe image
//        virtual int decompressSubframe(const int x, const int y, Subframe& subFrame);
//
//------------------------------------------------------------------------------
#ifndef __Eaagles_Maps_Rpf_CadrgFrame_H__
#define __Eaagles_Maps_Rpf_CadrgFrame_H__

#include "openeaagles/basic/Object.h"
#include "openeaagles/maps/rpfMap/Support.h"

namespace Eaagles {
namespace Maps {
namespace Rpf {

class CadrgFrameEntry;

class CadrgFrame : public Basic::Object
{
    DECLARE_SUBCLASS(CadrgFrame, Basic::Object)

public:
    CadrgFrame();

    // Load our entry for each frame
    virtual void load(CadrgFrameEntry* entry);
    // Decompress our subframe
    virtual int decompressSubframe(const int x, const int y, Subframe& subFrame);

private:
    static const int frameSize = 6144;              // Total frame size
    CadrgFrameEntry* frameEntry;                    // Pointer to our frame entry parent
    unsigned char subFrameTable[6][6][frameSize];   // Subframe table array
    int nitfHdrLength;                              // Nitf header length
    int masked[6][6];                               // Subframe masked array
    unsigned char lookupTable[4096][4][4];          // Lookup table
};

} // End Rpf namespace
} // End Maps namespace
} // End Eaagles namespace

#endif

