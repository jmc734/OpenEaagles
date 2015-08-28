//------------------------------------------------------------------------------
// Classes: Sphere, Cylinder, Cone, Cube, Torus, Dodecahedron, Tetrahedron,
// Icosahedron, Octahedron, Teapot
//
// Description:  3-dimensional shapes that can be drawn.  NOTE - these shapes
// all use GLUT or GLU, so you have to call the basicGlutFF to serialize them
// from the input, and you also have to link to glut or freeglut to draw them.
//------------------------------------------------------------------------------
#ifndef __Eaagles_Glut_Shapes3D_H__
#define __Eaagles_Glut_Shapes3D_H__

#include "openeaagles/basicGL/Shapes.h"

namespace Eaagles {
namespace Basic {
    class Number;
}

namespace Glut {

//------------------------------------------------------------------------------
// Class: Circle -> Sphere
// Description - draws a sphere with X slices and Y stacks
//
// Factory name: Sphere
// Slots:
//    stacks   <Number>    ! Number of stacks on the circle (default: 10)
//
// Note - all sphere and derived classes are automatically normalized at
// rendering (GLU does it)
//------------------------------------------------------------------------------
class Sphere : public BasicGL::Circle {
    DECLARE_SUBCLASS(Sphere, BasicGL::Circle)

public:
    Sphere();

    void drawFunc() override;

    virtual bool setStacks(const int x)       { stacks = x; return true; }
    int getStacks()                           { return stacks; }

protected:
    bool setSlotStacks(const Basic::Number* const x);

private:
    int stacks;
};

//------------------------------------------------------------------------------
// Class: Circle -> Sphere -> Cylinder
// Description - adds a top radius and height and draws a cylinder
//
// Factory name: Cylinder
// Slots:
//    topRadius  <Number>  ! Radius at the top of the cylinder (default: 1)
//    height     <Number>  ! Height of the cylinder (default: 1)
//
//------------------------------------------------------------------------------
class Cylinder : public Sphere {
    DECLARE_SUBCLASS(Cylinder, Sphere)
public:
    Cylinder();
    void drawFunc() override;

    virtual bool setTopRadius(const LCreal x)   { topRadius = x; return true; }
    virtual bool setHeight(const LCreal x)      { height = x; return true; }
    LCreal getTopRadius()                       { return topRadius; }
    LCreal getHeight()                          { return height; }

protected:
    bool setSlotTopRadius(const Basic::Number* const x);
    bool setSlotHeight(const Basic::Number* const x);

private:
    LCreal topRadius;
    LCreal height;
};

//------------------------------------------------------------------------------
// Class: Circle -> Sphere -> Cylinder -> Cone
// Description - uses everything but top radius to draw a cone
//
// Factory name: Cone
//
//------------------------------------------------------------------------------
class Cone : public Cylinder {
    DECLARE_SUBCLASS(Cone, Cylinder)
public:
    Cone();
    void drawFunc() override;
};

//------------------------------------------------------------------------------
// Class: Circle -> Cube
// Description - Draws a simple cube.  I derived it from Circle to use the filled
// flag only.  All other flags are ignored.
//
// Factory name: Cube
// Slots:
//    size    <Number>    ! Size you want the cube to be (default: 1)
//
//------------------------------------------------------------------------------
class Cube : public BasicGL::Circle {
    DECLARE_SUBCLASS(Cube, BasicGL::Circle)
public:
    Cube();
    void drawFunc() override;

    virtual bool setSize(const LCreal x)   { size = x; return true; }
    LCreal getSize()                       { return size; }

protected:
    bool setSlotSize(const Basic::Number* const srobj);

private:
    LCreal size;
};

//------------------------------------------------------------------------------
// Class: Sphere -> Torus
// Description - Uses filled slot to determine if wireframe or not.  Uses slices
// as number of sides, and stacks as number of rings
//
// Factory name: Torus
// Slots:
//   outerRadius    <Number>          ! Radius of the outer ring (default: 2)
//
//------------------------------------------------------------------------------
class Torus : public Sphere {
    DECLARE_SUBCLASS(Torus, Sphere)
public:
    Torus();
    void drawFunc() override;

    virtual bool setOuterRadius(const LCreal x)    { oRadius = x; return true; }
    LCreal getOuterRadius()                        { return oRadius; }

protected:
    bool setSlotOuterRadius(const Basic::Number* const x);

private:
    LCreal oRadius;
};


//------------------------------------------------------------------------------
// Class: Circle -> Dodecahedron
// Description - Draws a 12 faced figure which can be scaled
//
// Factory name: Dodecahedron
//
//------------------------------------------------------------------------------
class Dodecahedron : public BasicGL::Circle {
    DECLARE_SUBCLASS(Dodecahedron, BasicGL::Circle)
public:
    Dodecahedron();
    void drawFunc() override;
};

//------------------------------------------------------------------------------
// Class: Circle -> Tetrahedron
// Description - Draws a 4 faced figure which can be scaled
//
// Factory name: Tetrahedron
//
//------------------------------------------------------------------------------
class Tetrahedron : public BasicGL::Circle {
    DECLARE_SUBCLASS(Tetrahedron, BasicGL::Circle)
public:
    Tetrahedron();
    void drawFunc() override;
};

//------------------------------------------------------------------------------
// Class: Circle -> Icosahedron
// Description - Draws a 20 faced figure which can be scaled
//
// Factory name: Icosahedron
//
//------------------------------------------------------------------------------
class Icosahedron : public BasicGL::Circle {
    DECLARE_SUBCLASS(Icosahedron, BasicGL::Circle)
public:
    Icosahedron();
    void drawFunc() override;
};

//------------------------------------------------------------------------------
// Class: Circle -> Octahedron
// Description - Draws a 8 faced figure which can be scaled
//
// Factory name: Octahedron
//
//------------------------------------------------------------------------------
class Octahedron : public BasicGL::Circle {
    DECLARE_SUBCLASS(Octahedron, BasicGL::Circle)
public:
    Octahedron();
    void drawFunc() override;
};

//------------------------------------------------------------------------------
// Class: Cube -> Teapot
// Description - Draws the Utah Teapot, with a given size
//
// Factory name: Teapot
//
//------------------------------------------------------------------------------
class Teapot : public Cube {
    DECLARE_SUBCLASS(Teapot, Cube)
public:
    Teapot();
    void drawFunc() override;
};

} // End BasicGL namespace
} // End Eaagles namespace

#endif
