//------------------------------------------------------------------------------
// Form function for the guiGlut object library
//
//    The form functions are class factories that will create an object
//    by its 'form' name and will return a pre-ref() pointer to the new
//    object or zero if the form name wasn't found. 
//
//    (see Object.h, parser.y, and various OpenEaagles examples)
//------------------------------------------------------------------------------
#ifndef __Eaagles_Glut_FormFunc_H__
#define __Eaagles_Glut_FormFunc_H__

namespace Eaagles {
   namespace Basic { class Object; }
   namespace Glut {
      extern Basic::Object* glutFormFunc(const char* name);
   }
}

#endif

