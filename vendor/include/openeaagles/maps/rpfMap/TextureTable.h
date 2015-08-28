// ------------------------------------------------------------------------------
// Class: TextureTable
//
// Description: Table to hold our texture information for viewing.  This is
// called upon for data by the TexturePager, which is driven by the MapDrawer, which
// is driven by the CadrgMap
//
// Subroutines:
// setCenterRowTexture() - Sets our center row.
//      bool TextureTable::setCenterRowTexture(const int x)
//
// setCenterColumnTexture() - Sets our center column.
//      bool TextureTable::setCenterColumnTexture(const int x)
//
// isInBounds() - Are we in the texture boundaries?
//      bool TextureTable::isInBounds(int row, int col)
//
// setSize() - Sets the size of our table, and clears our texture object array
// NOTE: This will work correctly with only odd number sizes, which will
// split the table evenly.
//      bool TextureTable::setSize(const int newSize)
//
// setTextureObject() - Sets the texture at the given row, column.
//      bool TextureTable::setTextureObject(int row, int col, BasicGL::Texture* newObj)
//
// getTexture() - Returns the texture object at the given row / column.
//      BasicGL::Texture* TextureTable::getTexture(int row, int col)
//
// ------------------------------------------------------------------------------
#ifndef __Eaagles_Maps_Rpf_TextureTable_H__
#define __Eaagles_Maps_Rpf_TextureTable_H__

#include "openeaagles/basic/Object.h"

namespace Eaagles {
namespace BasicGL { class Texture; }
namespace Maps {
namespace Rpf {

class TextureTable : public Basic::Object
{
    DECLARE_SUBCLASS(TextureTable, Basic::Object)

public:
    TextureTable();

    // Get functions
    int getMaxTableSize()       { return maxTableSize; }
    int getLowerBoundIndex()    { return lowerBound; }
    int getUpperBoundIndex()    { return upperBound; }
    int centerRowTexture()      { return row; }
    int centerColumnTexture()   { return col; }

    BasicGL::Texture* getTexture(int row, int col);

    bool isInBounds(const int row, const int col);

    // Set functions
    virtual bool setSize(const int newSize);
    virtual bool setCenterRowTexture(const int x);
    virtual bool setCenterColumnTexture(const int x);
    virtual bool setTextureObject(int row, int col, BasicGL::Texture* newObj);

private:
    static const int MAX_TABLE_SIZE = 25;    // Maximum number of data in our tables

    int maxTableSize;
    int centerTablePos;
    int lowerBound;
    int upperBound;
    int row;
    int col;
    int size;
    BasicGL::Texture* texes[MAX_TABLE_SIZE][MAX_TABLE_SIZE];    // Holds our textures.
};

} // End Rpf namespace
} // End Maps namespace
} // End Eaagles namespace

#endif
