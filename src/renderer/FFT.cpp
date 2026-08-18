#include <cmath>
#include <iostream>
#include <vector>

#include "FFT.hpp"

FFT::FFT(const int p_n, std::vector<double> *const p_real,
         std::vector<double> *const p_imag)
    : n(p_n), p(log2(p_n)), real(p_real), imag(p_imag) {
  scratchR.resize(n);
  scratchI.resize(n);
}
void FFT::radix_direct() {
  int n_copy = n;
  for (int i = 0; i < p; i++) {
    const int pow2i = 1 << i;
    const int pow2i1 = 1 << (i + 1);
    for (int j = 0; j < n_copy / 2; j++) {
      for (int k = 0; k < pow2i; k++) {
        const int index1 = k + j * pow2i1;
        const int index2 = index1 + pow2i;
        const double var = static_cast<double>(-(2 * M_PI) / pow2i1) * index1;
        const double v_cos = cos(var);
        const double v_sin = sin(var);
        const double imag2 = imag->at(index2);
        const double real2 = real->at(index2);
        const double real1 = real->at(index1);
        const double imag1 = imag->at(index1);
        scratchR[index1] = real1 + v_cos * real2 - v_sin * imag2;
        scratchR[index2] = real1 - v_cos * real2 + v_sin * imag2;
        scratchI[index1] = imag1 + v_cos * imag2 + v_sin * real2;
        scratchI[index2] = imag1 - v_cos * imag2 - v_sin * real2;
      }
    }
    swap(scratchR, *real);
    swap(scratchI, *imag);
    n_copy /= 2;
  }
}
void FFT::radix_reverse() {
  int n_copy = n;
  for (int i = 0; i < p; i++) {
    const int pow2i = 1 << i;
    const int pow2i1 = 1 << (i + 1);
    for (int j = 0; j < n_copy / 2; j++) {
      for (int k = 0; k < pow2i; k++) {
        const int index1 = k + j * pow2i1;
        const int index2 = index1 + pow2i;
        const double var = static_cast<double>((2 * M_PI) / pow2i1) * index1;
        const double v_cos = cos(var);
        const double v_sin = sin(var);
        const double imag2 = imag->at(index2);
        const double real2 = real->at(index2);
        const double real1 = real->at(index1);
        const double imag1 = imag->at(index1);
        scratchR[index1] = real1 + v_cos * real2 - v_sin * imag2;
        scratchR[index2] = real1 - v_cos * real2 + v_sin * imag2;
        scratchI[index1] = imag1 + v_cos * imag2 + v_sin * real2;
        scratchI[index2] = imag1 - v_cos * imag2 - v_sin * real2;
      }
    }
    swap(scratchR, *real);
    swap(scratchI, *imag);
    n_copy /= 2;
  }
}
void FFT::sort() {
  int n_copy = n;
  for (int i = 0; i < p - 1; i++) {
    const int half = n_copy / 2;
    for (int j = 0; j < n / n_copy; j++) {
      const int base = j * n_copy;
      for (int k = 0; k < half; k++) {
        const int srcEven = base + 2 * k;
        const int srcOdd = srcEven + 1;
        scratchR[base + k] = real->at(srcEven);
        scratchI[base + k] = imag->at(srcEven);
        scratchR[base + half + k] = real->at(srcOdd);
        scratchI[base + half + k] = imag->at(srcOdd);
      }
    }
    swap(scratchR, *real);
    swap(scratchI, *imag);
    n_copy /= 2;
  }
}
