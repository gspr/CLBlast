
// =================================================================================================
// This file is part of the CLBlast project. The project is licensed under Apache Version 2.0. This
// project loosely follows the Google C++ styleguide and uses a tab-size of two spaces and a max-
// width of 100 characters per line.
//
// Author(s):
//   Cedric Nugteren <www.cedricnugteren.nl>
//
// This file implements the Xgemm routine. The precision is implemented using a template argument.
//
// =================================================================================================

#ifndef CLBLAST_ROUTINES_XGEMM_H_
#define CLBLAST_ROUTINES_XGEMM_H_

#include "routine.hpp"

namespace clblast {
// =================================================================================================

// See comment at top of file for a description of the class
template <typename T>
class Xgemm: public Routine {
 public:

  // Defines the assumptions of the GEMM kernels
  static const bool a_want_rotated_;
  static const bool b_want_rotated_;
  static const bool c_want_rotated_;

  // Selects which version of GEMM to run
  static bool UseDirectKernel(const size_t m, const size_t n, const size_t k,
                              const size_t min_indirect_size) {
    const auto m_n_k = static_cast<unsigned long long>(m) * static_cast<unsigned long long>(n) *
                       static_cast<unsigned long long>(k);
    const auto min_indirect_size_ll = static_cast<unsigned long long>(min_indirect_size);
    const auto min_indirect_size_e3 = min_indirect_size_ll * min_indirect_size_ll * min_indirect_size_ll;
    return (m_n_k < min_indirect_size_e3);
  }

  // Computes the sizes and offsets for (optional) temporary buffers for the 3 matrices
  static size_t ComputeTempSize(const bool a_no_temp, const bool b_no_temp, const bool c_no_temp,
                                const size_t a_size, const size_t b_size, const size_t c_size,
                                size_t &b_temp_offset, size_t &c_temp_offset) {
    auto temp_size = size_t{0};
    if (!a_no_temp) { temp_size += a_size; }
    if (!b_no_temp) { b_temp_offset = temp_size; temp_size += b_size; }
    if (!c_no_temp) { c_temp_offset = temp_size; temp_size += c_size; }
    return temp_size;
  }

  // Determines whether or not temporary matrices are needed
  static bool NoTempBuffer(const size_t one, const size_t one_i, const size_t two, const size_t two_i,
                           const size_t ld, const size_t offset,
                           const bool do_transpose, const bool conjugate) {
    return one == one_i && two == two_i && ld == one && offset == 0 && !do_transpose && !conjugate;
  }


  // Computes the first and second "internal" (ceiled) dimensions of the 3 matrices taking into account
  // whether the matrices need to be rotated or not for the kernel.
  static void CalculateInternalDimensions(const size_t m, const size_t n, const size_t k,
                                          const size_t mwg, const size_t nwg, const size_t kwg,
                                          size_t& a_one_i, size_t& a_two_i, size_t& b_one_i,
                                          size_t& b_two_i, size_t& c_one_i, size_t& c_two_i) {
    const auto m_ceiled = Ceil(m, mwg);
    const auto n_ceiled = Ceil(n, nwg);
    const auto k_ceiled = Ceil(k, kwg);
    a_one_i = (a_want_rotated_) ? k_ceiled : m_ceiled;
    a_two_i = (a_want_rotated_) ? m_ceiled : k_ceiled;
    b_one_i = (b_want_rotated_) ? n_ceiled : k_ceiled;
    b_two_i = (b_want_rotated_) ? k_ceiled : n_ceiled;
    c_one_i = (c_want_rotated_) ? n_ceiled : m_ceiled;
    c_two_i = (c_want_rotated_) ? m_ceiled : n_ceiled;
  }

  // Constructor
  Xgemm(Queue &queue, EventPointer event, const std::string &name = "GEMM");

  // Templated-precision implementation of the routine
  void DoGemm(const Layout layout, const Transpose a_transpose, const Transpose b_transpose,
              const size_t m, const size_t n, const size_t k,
              const T alpha,
              const Buffer<T> &a_buffer, const size_t a_offset, const size_t a_ld,
              const Buffer<T> &b_buffer, const size_t b_offset, const size_t b_ld,
              const T beta,
              const Buffer<T> &c_buffer, const size_t c_offset, const size_t c_ld);

  // Indirect version of GEMM (with pre and post-processing kernels)
  void GemmIndirect(const size_t m, const size_t n, const size_t k,
                    const T alpha,
                    const Buffer<T> &a_buffer, const size_t a_offset, const size_t a_ld,
                    const Buffer<T> &b_buffer, const size_t b_offset, const size_t b_ld,
                    const T beta,
                    const Buffer<T> &c_buffer, const size_t c_offset, const size_t c_ld,
                    const bool a_do_transpose, const bool b_do_transpose, const bool c_do_transpose,
                    const bool a_conjugate, const bool b_conjugate,
                    const size_t a_one, const size_t a_two,
                    const size_t b_one, const size_t b_two,
                    const size_t c_one, const size_t c_two);

  // Direct version of GEMM (no pre and post-processing kernels)
  void GemmDirect(const size_t m, const size_t n, const size_t k,
                  const T alpha,
                  const Buffer<T> &a_buffer, const size_t a_offset, const size_t a_ld,
                  const Buffer<T> &b_buffer, const size_t b_offset, const size_t b_ld,
                  const T beta,
                  const Buffer<T> &c_buffer, const size_t c_offset, const size_t c_ld,
                  const bool a_do_transpose, const bool b_do_transpose, const bool c_do_transpose,
                  const bool a_conjugate, const bool b_conjugate);
};

// =================================================================================================
} // namespace clblast

// CLBLAST_ROUTINES_XGEMM_H_
#endif
