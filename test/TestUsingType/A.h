#ifndef A_H
#define A_H

#include <vector>
#include <string>

class G4VPhysicsConstructor
{
  public:
    G4VPhysicsConstructor(const std::string& n = "") : name(n) {}
    virtual ~G4VPhysicsConstructor() {}
  private:
    std::string name;
};

class G4VMPLData
{
  public:
    using G4PhysConstVectorData = std::vector<G4VPhysicsConstructor*>;
    void initialize() { physicsVector = new G4PhysConstVectorData; }
    G4PhysConstVectorData* physicsVector = nullptr;
};

#endif //A_H not defined

