template<typename A, typename B>
struct TemplateType
{
  typedef typename A::val_type first_val_type;
  typedef typename B::val_type second_val_type;

  first_val_type get_first()
  {
    return A::value();
  }

  second_val_type get_second()
  {
    return B::value();
  }
};

struct P1 {
  typedef int val_type;
  static val_type value(){ return 2;}
};

struct P2 {
  typedef float val_type;
  static val_type value(){ return 2.5;}
};

//Instantiate the templated type
template class TemplateType<P1, P2>;
