int f(int x){
  return 2*x;
}

double g(double x){
  return 2*x;
}

typedef int (*f_ptr)(int);

f_ptr getf(){
  return f;
}

double (*getg())(double){
  return g;
}

int apply(f_ptr f, int x){
  return f(x);
}

