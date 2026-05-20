#ifndef FFTHPP
#define FFTHPP

#include <iostream>
#include <vector>

class FFT {

public:
  FFT(const int, std::vector<double> *const, std::vector<double> *const);

  void direct() {
    sort();
    radix_direct();
  }
  void reverse() {
    sort();
    radix_reverse();
  }

private:
  typedef std::vector<double> *vec_d_p;

  void radix_direct();
  void radix_reverse();
  void sort();

  const int n;
  const int p;
  vec_d_p real;
  vec_d_p imag;
};

#endif
