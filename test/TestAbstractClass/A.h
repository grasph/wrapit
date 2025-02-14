#ifndef A_H
#define A_H

#include <string>

struct G4ThreeVector
{
    G4ThreeVector(double x, double y, double z) : x(x), y(y), z(z) {}
    double x, y, z;
};

struct G4VSolid   // Pure abstract class
{
    G4VSolid(const std::string& name) { fshapeName = name; }
    virtual ~G4VSolid() {}
    virtual bool Inside(const G4ThreeVector& p) const = 0;
    std::string fshapeName;
};

struct G4BooleanSolid : public G4VSolid // also abstract class because it does not implement Inside()
{
    G4BooleanSolid( const std::string& pName, G4VSolid* pSolidA , G4VSolid* pSolidB) : G4VSolid(pName), fSolidA(pSolidA), fSolidB(pSolidB) {}
    virtual ~G4BooleanSolid() {}
    G4VSolid* fSolidA;
    G4VSolid* fSolidB;
};

struct G4UnionSolid : public G4BooleanSolid // concrete class
{
    G4UnionSolid( const std::string& pName, G4VSolid* pSolidA, G4VSolid* pSolidB) : G4BooleanSolid(pName, pSolidA, pSolidB) {}
    virtual ~G4UnionSolid() {}
    bool Inside(const G4ThreeVector& p) const override { return fSolidA->Inside(p) || fSolidB->Inside(p);}
};

struct G4Box : public G4VSolid  // concrete class
{
    G4Box(const std::string& name, double x, double y, double z) : G4VSolid(name), fX(x), fY(y), fZ(z) {}
    virtual ~G4Box() {}
    bool Inside(const G4ThreeVector& p) const override { return p.x < fX && p.y < fY && p.z < fZ; }
    double fX, fY, fZ;
};

#endif //A_H not defined
