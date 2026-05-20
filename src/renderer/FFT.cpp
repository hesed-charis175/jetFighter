#include <cmath>
#include <iostream>
#include <vector>

#include "FFT.hpp"

FFT::FFT(const int p_n, std::vector<double> *const p_real,
         std::vector<double> *const p_imag)
    : n(p_n), p(log2(p_n)), real(p_real), imag(p_imag) {}

void FFT::radix_direct() {
  int n_copy = n;
  std::vector<double> real_copy;
  real_copy.resize(n);
  std::vector<double> imag_copy;
  imag_copy.resize(n);
  for (int i = 0; i < p; i++) {
    for (int j = 0; j < n_copy / 2; j++) {
      for (int k = 0; k < pow(2, i); k++) {
        const int index1 = k + j * pow(2, i + 1);
        const int index2 = index1 + pow(2, i);
        const double var =
            static_cast<double>(-(2 * M_PI) / pow(2, i + 1)) * index1;
        const double v_cos = cos(var);
        const double v_sin = sin(var);
        const double imag2 = imag->at(index2);
        const double real2 = real->at(index2);
        const double real1 = real->at(index1);
        const double imag1 = imag->at(index1);
        real_copy[index1] = real1 + v_cos * real2 - v_sin * imag2;
        real_copy[index2] = real1 - v_cos * real2 + v_sin * imag2;
        imag_copy[index1] = imag1 + v_cos * imag2 + v_sin * real2;
        imag_copy[index2] = imag1 - v_cos * imag2 - v_sin * real2;
      }
    }
    swap(real_copy, *real);
    swap(imag_copy, *imag);
    n_copy /= 2;
  }
}

void FFT::radix_reverse() {
  int n_copy = n;
  std::vector<double> real_copy;
  real_copy.resize(n);
  std::vector<double> imag_copy;
  imag_copy.resize(n);
  for (int i = 0; i < p; i++) {
    for (int j = 0; j < n_copy / 2; j++) {
      for (int k = 0; k < pow(2, i); k++) {
        const int index1 = k + j * pow(2, i + 1);
        const int index2 = index1 + pow(2, i);
        const double var =
            static_cast<double>((2 * M_PI) / pow(2, i + 1)) * index1;
        const double v_cos = cos(var);
        const double v_sin = sin(var);
        const double imag2 = imag->at(index2);
        const double real2 = real->at(index2);
        const double real1 = real->at(index1);
        const double imag1 = imag->at(index1);
        real_copy[index1] = real1 + v_cos * real2 - v_sin * imag2;
        real_copy[index2] = real1 - v_cos * real2 + v_sin * imag2;
        imag_copy[index1] = imag1 + v_cos * imag2 + v_sin * real2;
        imag_copy[index2] = imag1 - v_cos * imag2 - v_sin * real2;
      }
    }
    swap(real_copy, *real);
    swap(imag_copy, *imag);
    n_copy /= 2;
  }
}

void FFT::sort() {
  int n_copy = n;
  for (int i = 0; i < p - 1; i++) {
    std::vector<double> sorted_R;
    sorted_R.reserve(n);
    std::vector<double> sorted_I;
    sorted_I.reserve(n);
    std::vector<double> vectRp;
    vectRp.resize(n_copy / 2);
    std::vector<double> vectIp;
    vectIp.resize(n_copy / 2);
    std::vector<double> vectRi;
    vectRi.resize(n_copy / 2);
    std::vector<double> vectIi;
    vectIi.resize(n_copy / 2);
    std::vector<double>::iterator itR(sorted_R.begin());
    std::vector<double>::iterator itI(sorted_I.begin());
    for (int j = 0; j < n / n_copy; j++) {
      for (int k = 0; k < n_copy / 2; k++) {
        const double index = 2 * k + j * n_copy;
        vectRp[k] = real->at(index);
        vectIp[k] = imag->at(index);
        vectRi[k] = real->at(index + 1);
        vectIi[k] = imag->at(index + 1);
      }
      sorted_R.insert(itR, vectRp.begin(), vectRp.end());
      sorted_I.insert(itI, vectIp.begin(), vectIp.end());
      sorted_R.insert(sorted_R.end(), vectRi.begin(), vectRi.end());
      sorted_I.insert(sorted_I.end(), vectIi.begin(), vectIi.end());
      itR = sorted_R.end();
      itI = sorted_I.end();
    }
    swap(sorted_R, *real);
    swap(sorted_I, *imag);
    n_copy /= 2;
  }
}
