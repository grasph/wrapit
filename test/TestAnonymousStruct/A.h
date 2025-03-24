struct {
  int i;
} s_global;

class A {
  struct {
    int i;
  } s_private;

public:
  struct {
    int i;
  } s_public;

  struct {
    int j;
  };

  struct S_public_named_type {
    int k = 5;
  } s_public_named;
};
