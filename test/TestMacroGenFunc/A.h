#ifndef A_H
#define A_H

#define DECLARE_ACCESSORS(Type, Name)                 \
  void Set##Name(Type value) { Name##_ = value; }     \
  Type Get##Name() const { return Name##_; }

class A {
public:
  DECLARE_ACCESSORS(int, Value)

private:
  int Value_ = 0;
};

#endif //A_H not defined
