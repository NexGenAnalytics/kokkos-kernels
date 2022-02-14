/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Siva Rajamanickam (srajama@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <algorithm>
#include <gtest/gtest.h>
#include <Kokkos_Core.hpp>
#include <stdexcept>
#include "KokkosSparse_spmv.hpp"
#include "KokkosSparse_BsrMatrix.hpp"
#include "KokkosSparse_CrsMatrix.hpp"

#include <KokkosKernels_TestUtils.hpp>
#include <KokkosKernels_Test_Structured_Matrix.hpp>
#include <KokkosKernels_IOUtils.hpp>
#include <KokkosKernels_Utils.hpp>

#include "KokkosKernels_Controls.hpp"
#include "KokkosKernels_default_types.hpp"

typedef Kokkos::complex<double> kokkos_complex_double;
typedef Kokkos::complex<float> kokkos_complex_float;

namespace Test_Bsr {

/// Random generator
template <typename Scalar>
inline Scalar random() {
  auto const max = static_cast<Scalar>(RAND_MAX) + static_cast<Scalar>(1);
  return static_cast<Scalar>(std::rand()) / max;
}

template <typename Scalar>
inline void set_random_value(Scalar &v) {
  v = random<Scalar>();
}

template <typename Scalar>
inline void set_random_value(Kokkos::complex<Scalar> &v) {
  Scalar vre = random<Scalar>();
  Scalar vim = random<Scalar>();
  v          = Kokkos::complex<Scalar>(vre, vim);
}

template <typename Scalar>
inline void set_random_value(std::complex<Scalar> &v) {
  Scalar vre = random<Scalar>();
  Scalar vim = random<Scalar>();
  v          = std::complex<Scalar>(vre, vim);
}

/// \brief Routine to make CRS-style entries of the block matrix
///
/// \tparam scalar_t Template type for the numerical values
/// \param mat_b1  Sparse matrix whose graph will be used
/// \param blockSize  Block size for each entries
/// \param mat_rowmap[out]  CRS-style row map for the block matrix
/// \param mat_colidx[out]  CRS-style column entries for the block matrix
/// \param mat_val[out]  Numerical (random) values
template <typename scalar_t, typename lno_t, typename size_type>
void make_block_entries(
    const KokkosSparse::CrsMatrix<scalar_t, lno_t, Kokkos::HostSpace, void,
                                  size_type> &mat_b1,
    int blockSize, std::vector<size_type> &mat_rowmap,
    std::vector<lno_t> &mat_colidx, std::vector<scalar_t> &mat_val) {
  lno_t nRow = blockSize * mat_b1.numRows();
  size_t nnz = static_cast<size_t>(blockSize) * static_cast<size_t>(blockSize) *
               mat_b1.nnz();

  mat_val.resize(nnz);
  for (size_t ii = 0; ii < nnz; ++ii) set_random_value(mat_val[ii]);

  //
  // Create graph for CrsMatrix
  //

  mat_rowmap.assign(nRow + 1, 0);
  mat_colidx.assign(nnz, 0);

  for (lno_t ir = 0; ir < mat_b1.numRows(); ++ir) {
    const size_type jbeg = mat_b1.graph.row_map(ir);
    const size_type jend = mat_b1.graph.row_map(ir + 1);
    for (lno_t ib = 0; ib < blockSize; ++ib) {
      const lno_t my_row     = ir * blockSize + ib;
      mat_rowmap[my_row + 1] = mat_rowmap[my_row] + (jend - jbeg) * blockSize;
      for (size_type ijk = jbeg; ijk < jend; ++ijk) {
        const auto col0 = mat_b1.graph.entries(ijk);
        for (lno_t jb = 0; jb < blockSize; ++jb) {
          mat_colidx[mat_rowmap[my_row] + (ijk - jbeg) * blockSize + jb] =
              col0 * blockSize + jb;
        }
      }
    }
  }  // for (lno_t ir = 0; ir < mat_b1.numRows(); ++ir)
}

/// \brief Driver routine for checking BsrMatrix times vector
template <typename scalar_t, typename lno_t, typename size_type,
          typename device>
void check_bsrm_times_v(const char fOp[], scalar_t alpha, scalar_t beta,
                        const lno_t bMax, int &num_errors) {
  // The mat_structure view is used to generate a matrix using
  // finite difference (FD) or finite element (FE) discretization
  // on a cartesian grid.
  Kokkos::View<lno_t * [3], Kokkos::HostSpace> mat_structure("Matrix Structure",
                                                             3);
  mat_structure(0, 0) = 8;  // Request 8 grid point in 'x' direction
  mat_structure(0, 1) = 0;  // Add BC to the left
  mat_structure(0, 2) = 0;  // Add BC to the right
  mat_structure(1, 0) = 7;  // Request 7 grid point in 'y' direction
  mat_structure(1, 1) = 0;  // Add BC to the bottom
  mat_structure(1, 2) = 0;  // Add BC to the top
  mat_structure(2, 0) = 9;  // Request 9 grid point in 'z' direction
  mat_structure(2, 1) = 0;  // Add BC to the bottom
  mat_structure(2, 2) = 0;  // Add BC to the top

  typedef
      typename KokkosSparse::CrsMatrix<scalar_t, lno_t, device, void, size_type>
          crsMat_t;
  typedef typename KokkosSparse::CrsMatrix<scalar_t, lno_t, Kokkos::HostSpace,
                                           void, size_type>
      h_crsMat_t;
  typedef typename crsMat_t::values_type::non_const_type scalar_view_t;
  typedef scalar_view_t x_vector_type;
  typedef scalar_view_t y_vector_type;

  h_crsMat_t mat_b1 =
      Test::generate_structured_matrix3D<h_crsMat_t>("FD", mat_structure);

  num_errors = 0;
  for (lno_t blockSize = 1; blockSize <= bMax; ++blockSize) {
    //
    // Fill blocks with random values
    //

    lno_t nRow    = blockSize * mat_b1.numRows();
    lno_t nCol    = blockSize * mat_b1.numCols();
    size_type nnz = static_cast<size_type>(blockSize) *
                    static_cast<size_type>(blockSize) * mat_b1.nnz();

    std::vector<size_type> mat_rowmap(nRow + 1, 0);
    std::vector<lno_t> mat_colidx(nnz, 0);
    std::vector<scalar_t> mat_val(nnz);

    // Create the entries
    make_block_entries<scalar_t, lno_t, size_type>(
        mat_b1, blockSize, mat_rowmap, mat_colidx, mat_val);

    // Create the CrsMatrix for the reference computation
    //////
    std::vector<lno_t> lno_rowmap(nRow + 1);
    for (size_t ijk = 0; ijk < mat_rowmap.size(); ++ijk)
      lno_rowmap[ijk] = static_cast<lno_t>(mat_rowmap[ijk]);
    //////
    crsMat_t Acrs("new_crs_matr", nRow, nCol, nnz, mat_val.data(),
                  lno_rowmap.data(), mat_colidx.data());

    x_vector_type xref("new_right_hand_side", nRow);
    auto h_xref = Kokkos::create_mirror_view(xref);
    for (lno_t ir = 0; ir < nRow; ++ir) {
      set_random_value(h_xref(ir));
    }
    Kokkos::deep_copy(xref, h_xref);

    y_vector_type y0("y_init", nRow);
    auto h_y0 = Kokkos::create_mirror_view(y0);
    for (lno_t ir = 0; ir < nRow; ++ir) set_random_value(h_y0(ir));
    Kokkos::deep_copy(y0, h_y0);

    y_vector_type ycrs("crs_product_result", nRow);
    auto h_ycrs = Kokkos::create_mirror_view(ycrs);
    for (lno_t ir = 0; ir < nRow; ++ir) h_ycrs(ir) = h_y0(ir);
    Kokkos::deep_copy(ycrs, h_ycrs);

    //
    // Make reference computation with a CrsMatrix variable
    //
    KokkosSparse::spmv(fOp, alpha, Acrs, xref, beta, ycrs);

    y_vector_type ybsr("bsr_product_result", nRow);
    auto h_ybsr = Kokkos::create_mirror_view(ybsr);
    for (lno_t ir = 0; ir < nRow; ++ir) h_ybsr(ir) = h_y0(ir);
    Kokkos::deep_copy(ybsr, h_ybsr);

    // Create the BsrMatrix for the check test
    KokkosSparse::Experimental::BsrMatrix<scalar_t, lno_t, device, void,
                                          size_type>
        Absr(Acrs, blockSize);

    //
    // Make computation with the BsrMatrix format
    //
    KokkosSparse::spmv(fOp, alpha, Absr, xref, beta, ybsr);

    //
    // Compare the two products
    //
    using KATS     = Kokkos::ArithTraits<scalar_t>;
    using mag_type = typename KATS::mag_type;

    const mag_type zero_mag = Kokkos::ArithTraits<mag_type>::zero();
    mag_type error = zero_mag, maxNorm = zero_mag;

    Kokkos::deep_copy(h_ycrs, ycrs);
    Kokkos::deep_copy(h_ybsr, ybsr);
    for (lno_t ir = 0; ir < nRow; ++ir) {
      error   = std::max<mag_type>(error, KATS::abs(h_ycrs(ir) - h_ybsr(ir)));
      maxNorm = std::max<mag_type>(maxNorm, KATS::abs(h_ycrs(ir)));
    }

    mag_type tmps = KATS::abs(alpha) + KATS::abs(beta);
    if ((tmps > zero_mag) && (maxNorm == zero_mag)) {
      std::cout << " BSR - SpMV times MV >> blockSize " << blockSize
                << " maxNorm " << maxNorm << " error " << error << " alpha "
                << alpha << " beta " << beta << "\n";
      num_errors += 1;
    }

    //
    // --- Factor ((nnz / nRow) + 1) = Average number of non-zeros per row
    //
    const mag_type tol = ((static_cast<mag_type>(nnz) / nRow) + 1) *
                         Kokkos::ArithTraits<mag_type>::epsilon();
    if (error > tol * maxNorm) {
      std::cout << " BSR - SpMV times V >> blockSize " << blockSize << " ratio "
                << error / maxNorm << " tol " << tol << " maxNorm " << maxNorm
                << " alpha " << alpha << " beta " << beta << "\n";
      num_errors += 1;
    }

  }  // for (int blockSize = 1; blockSize <= bMax; ++blockSize)
}

/// \brief Driver routine for checking BsrMatrix times multiple vector
template <typename scalar_t, typename lno_t, typename size_type,
          typename device>
void check_bsrm_times_mv(const char fOp[], scalar_t alpha, scalar_t beta,
                         const lno_t bMax, int &num_errors) {
  // The mat_structure view is used to generate a matrix using
  // finite difference (FD) or finite element (FE) discretization
  // on a cartesian grid.
  Kokkos::View<lno_t * [3], Kokkos::HostSpace> mat_structure("Matrix Structure",
                                                             3);
  mat_structure(0, 0) = 7;  // Request 7 grid point in 'x' direction
  mat_structure(0, 1) = 0;  // Add BC to the left
  mat_structure(0, 2) = 0;  // Add BC to the right
  mat_structure(1, 0) = 5;  // Request 11 grid point in 'y' direction
  mat_structure(1, 1) = 0;  // Add BC to the bottom
  mat_structure(1, 2) = 0;  // Add BC to the top
  mat_structure(2, 0) = 9;  // Request 13 grid point in 'y' direction
  mat_structure(2, 1) = 0;  // Add BC to the bottom
  mat_structure(2, 2) = 0;  // Add BC to the top

  typedef typename KokkosSparse::CrsMatrix<scalar_t, lno_t, Kokkos::HostSpace,
                                           void, size_type>
      h_crsMat_t;
  typedef
      typename KokkosSparse::CrsMatrix<scalar_t, lno_t, device, void, size_type>
          crsMat_t;
  typedef Kokkos::View<scalar_t **, Kokkos::LayoutLeft, device> block_vector_t;

  h_crsMat_t mat_b1 =
      Test::generate_structured_matrix3D<h_crsMat_t>("FD", mat_structure);

  num_errors     = 0;
  const int nrhs = 5;

  for (lno_t blockSize = 1; blockSize <= bMax; ++blockSize) {
    //
    // Fill blocks with random values
    //

    lno_t nRow    = blockSize * mat_b1.numRows();
    lno_t nCol    = blockSize * mat_b1.numCols();
    size_type nnz = static_cast<size_type>(blockSize) *
                    static_cast<size_type>(blockSize) * mat_b1.nnz();

    std::vector<size_type> mat_rowmap(nRow + 1, 0);
    std::vector<lno_t> mat_colidx(nnz, 0);
    std::vector<scalar_t> mat_val(nnz);

    // Create the entries
    make_block_entries<scalar_t, lno_t, size_type>(
        mat_b1, static_cast<int>(blockSize), mat_rowmap, mat_colidx, mat_val);

    // Create the CrsMatrix for the reference computation
    //////
    std::vector<lno_t> lno_rowmap(nRow + 1);
    for (size_t ijk = 0; ijk < mat_rowmap.size(); ++ijk)
      lno_rowmap[ijk] = static_cast<lno_t>(mat_rowmap[ijk]);
    //////
    crsMat_t Acrs("new_crs_matr", nRow, nCol, nnz, mat_val.data(),
                  lno_rowmap.data(), mat_colidx.data());

    block_vector_t xref("new_right_hand_side", nRow, nrhs);
    auto h_xref = Kokkos::create_mirror_view(xref);
    for (int jc = 0; jc < nrhs; ++jc)
      for (lno_t ir = 0; ir < nRow; ++ir) set_random_value(h_xref(ir, jc));
    Kokkos::deep_copy(xref, h_xref);

    block_vector_t y0("y_init", nRow, nrhs);
    auto h_y0 = Kokkos::create_mirror_view(y0);
    for (int jc = 0; jc < nrhs; ++jc)
      for (lno_t ir = 0; ir < nRow; ++ir) set_random_value(h_y0(ir, jc));
    Kokkos::deep_copy(y0, h_y0);

    block_vector_t ycrs("crs_product_result", nRow, nrhs);
    auto h_ycrs = Kokkos::create_mirror_view(ycrs);
    for (int jc = 0; jc < nrhs; ++jc)
      for (lno_t ir = 0; ir < nRow; ++ir) h_ycrs(ir, jc) = h_y0(ir, jc);
    Kokkos::deep_copy(ycrs, h_ycrs);

    //
    // Compute the reference product with a CrsMatrix variable
    //
    KokkosSparse::spmv(fOp, alpha, Acrs, xref, beta, ycrs);

    block_vector_t ybsr("bsr_product_result", nRow, nrhs);
    auto h_ybsr = Kokkos::create_mirror_view(ybsr);
    for (int jc = 0; jc < nrhs; ++jc)
      for (lno_t ir = 0; ir < nRow; ++ir) h_ybsr(ir, jc) = h_y0(ir, jc);
    Kokkos::deep_copy(ybsr, h_ybsr);

    // Create the BsrMatrix for the check test
    KokkosSparse::Experimental::BsrMatrix<scalar_t, lno_t, device, void,
                                          size_type>
        Absr(Acrs, blockSize);

    //
    // Compute the product with the BsrMatrix format
    //
    KokkosSparse::spmv(fOp, alpha, Absr, xref, beta, ybsr);

    Kokkos::deep_copy(h_ycrs, ycrs);
    Kokkos::deep_copy(h_ybsr, ybsr);

    //
    // Compare the two products
    //
    using KATS     = Kokkos::ArithTraits<scalar_t>;
    using mag_type = typename KATS::mag_type;

    const mag_type zero_mag = Kokkos::ArithTraits<mag_type>::zero();
    mag_type error = zero_mag, maxNorm = zero_mag;
    for (int jc = 0; jc < nrhs; ++jc) {
      for (int ir = 0; ir < nRow; ++ir) {
        error   = std::max<mag_type>(error,
                                   KATS::abs(h_ycrs(ir, jc) - h_ybsr(ir, jc)));
        maxNorm = std::max<mag_type>(maxNorm, KATS::abs(h_ycrs(ir, jc)));
      }
    }

    mag_type tmps = KATS::abs(alpha) + KATS::abs(beta);
    if ((tmps > zero_mag) && (maxNorm == zero_mag)) {
      std::cout << " BSR - SpMV times MV >> blockSize " << blockSize
                << " maxNorm " << maxNorm << " error " << error << " alpha "
                << alpha << " beta " << beta << "\n";
      num_errors += 1;
    }

    const mag_type tol = ((static_cast<mag_type>(nnz) / nRow) + 1) *
                         Kokkos::ArithTraits<mag_type>::epsilon();
    if (error > tol * maxNorm) {
      std::cout << " BSR - SpMV times MV >> blockSize " << blockSize
                << " ratio " << error / maxNorm << " tol " << tol << " maxNorm "
                << maxNorm << " alpha " << alpha << " beta " << beta << "\n";
      num_errors += 1;
    }

  }  // for (int blockSize = 1; blockSize <= bMax; ++blockSize)
}

}  // namespace Test_Bsr

template <typename scalar_t, typename lno_t, typename size_type,
          typename device>
void testSpMVBsrMatrix() {
  //
  // Check a few corner cases
  //

  // 0 x 0 case
  {
    typedef
        typename KokkosSparse::Experimental::BsrMatrix<scalar_t, lno_t, device,
                                                       void, size_type>
            bsrMat_t;
    bsrMat_t Absr("empty", 0, 0, 0, nullptr, nullptr, nullptr, 1);
    typedef typename bsrMat_t::values_type::non_const_type scalar_view_t;
    typedef scalar_view_t x_vector_type;
    typedef scalar_view_t y_vector_type;
    x_vector_type x("corner-case-x", Absr.numCols());
    y_vector_type y("corner-case-y", Absr.numRows());
    Kokkos::deep_copy(y, static_cast<scalar_t>(0));
    scalar_t alpha = static_cast<scalar_t>(1);
    scalar_t beta  = static_cast<scalar_t>(1);
    const char fOp = 'N';
    int num_errors = 0;
    try {
      KokkosSparse::spmv(&fOp, alpha, Absr, x, beta, y);
      Kokkos::fence();
    } catch (std::exception &e) {
      num_errors += 1;
      std::cout << e.what();
    }
    EXPECT_TRUE(num_errors == 0);
  }

  // 0 x 1 case
  {
    typedef
        typename KokkosSparse::Experimental::BsrMatrix<scalar_t, lno_t, device,
                                                       void, size_type>
            bsrMat_t;
    bsrMat_t Absr("empty", 0, 1, 0, nullptr, nullptr, nullptr, 1);
    typedef typename bsrMat_t::values_type::non_const_type scalar_view_t;
    typedef scalar_view_t x_vector_type;
    typedef scalar_view_t y_vector_type;
    x_vector_type x("corner-case-x", Absr.numCols());
    y_vector_type y("corner-case-y", Absr.numRows());
    Kokkos::deep_copy(y, static_cast<scalar_t>(0));
    scalar_t alpha = static_cast<scalar_t>(1);
    scalar_t beta  = static_cast<scalar_t>(1);
    const char fOp = 'N';
    int num_errors = 0;
    try {
      KokkosSparse::spmv(&fOp, alpha, Absr, x, beta, y);
      Kokkos::fence();
    } catch (std::exception &e) {
      num_errors += 1;
      std::cout << e.what();
    }
    EXPECT_TRUE(num_errors == 0);
  }

  // 1 x 0 case
  {
    typedef
        typename KokkosSparse::Experimental::BsrMatrix<scalar_t, lno_t, device,
                                                       void, size_type>
            bsrMat_t;
    bsrMat_t Absr("empty", 1, 0, 0, nullptr, nullptr, nullptr, 1);
    typedef typename bsrMat_t::values_type::non_const_type scalar_view_t;
    typedef scalar_view_t x_vector_type;
    typedef scalar_view_t y_vector_type;
    x_vector_type x("corner-case-x", Absr.numCols());
    y_vector_type y("corner-case-y", Absr.numRows());
    Kokkos::deep_copy(y, static_cast<scalar_t>(0));
    scalar_t alpha = static_cast<scalar_t>(1);
    scalar_t beta  = static_cast<scalar_t>(1);
    const char fOp = 'N';
    int num_errors = 0;
    try {
      KokkosSparse::spmv(&fOp, alpha, Absr, x, beta, y);
      Kokkos::fence();
    } catch (std::exception &e) {
      num_errors += 1;
      std::cout << e.what();
    }
    EXPECT_TRUE(num_errors == 0);
  }

  //
  // Test for the operation y <- alpha * Op(A) * x + beta * y
  //

  // Define the function Op: Op(A) = A, Op(A) = conj(A), Op(A) = A^T, Op(A) =
  // A^H
  std::vector<char> modes = {'N', 'C', 'T', 'H'};

  // Define a set of pairs (alpha, beta)
  std::vector<double> testAlphaBeta = {0.0, 0.0, -1.0, 0.0,
                                       0.0, 1.0, 3.1,  -2.5};

  //
  // Set the largest block size for the block matrix
  // The code will create matrices with block sizes 1, .., bMax
  //
  constexpr lno_t bMax = 13;

  //
  //--- Test single vector case
  //
  for (const auto mode : modes) {
    int num_errors = 0;
    for (size_t ii = 0; ii < testAlphaBeta.size(); ii += 2) {
      auto alpha_s = static_cast<scalar_t>(testAlphaBeta[ii]);
      auto beta_s  = static_cast<scalar_t>(testAlphaBeta[ii + 1]);
      num_errors   = 0;
      Test_Bsr::check_bsrm_times_v<scalar_t, lno_t, size_type, device>(
          &mode, alpha_s, beta_s, bMax, num_errors);
      if (num_errors > 0) {
        printf(
            "KokkosSparse::Test::spmv_bsr: %i errors of %i with params: "
            "%c %lf %lf\n",
            num_errors, bMax, mode, Kokkos::ArithTraits<scalar_t>::abs(alpha_s),
            Kokkos::ArithTraits<scalar_t>::abs(beta_s));
      }
      EXPECT_TRUE(num_errors == 0);
    }
  }
}

template <typename scalar_t, typename lno_t, typename size_type,
          typename device>
void testBsrMatrix_SpM_MV() {
  //
  // Test for the operation Y <- alpha * Op(A) * X + beta * Y
  //

  // Define the function Op: Op(A) = A, Op(A) = conj(A), Op(A) = A^T, Op(A) =
  // A^H
  std::vector<char> modes = {'N', 'C', 'T', 'H'};

  // Define a set of pairs (alpha, beta)
  std::vector<double> testAlphaBeta = {0.0, 0.0, -1.0, 0.0,
                                       0.0, 1.0, 3.1,  -2.5};

  //
  // Set the largest block size for the block matrix
  // The code will create matrices with block sizes 1, .., bMax
  //
  const lno_t bMax = 13;

  //--- Test multiple vector case
  for (auto mode : modes) {
    int num_errors = 0;
    for (size_t ii = 0; ii < testAlphaBeta.size(); ii += 2) {
      auto alpha_s = static_cast<scalar_t>(testAlphaBeta[ii]);
      auto beta_s  = static_cast<scalar_t>(testAlphaBeta[ii + 1]);
      num_errors   = 0;
      Test_Bsr::check_bsrm_times_mv<scalar_t, lno_t, size_type, device>(
          &mode, alpha_s, beta_s, bMax, num_errors);
      if (num_errors > 0) {
        printf(
            "KokkosSparse::Test::spm_mv_bsr: %i errors of %i with params: "
            "%c %lf %lf\n",
            num_errors, bMax, mode, Kokkos::ArithTraits<scalar_t>::abs(alpha_s),
            Kokkos::ArithTraits<scalar_t>::abs(beta_s));
      }
      EXPECT_TRUE(num_errors == 0);
    }
  }
}

//////////////////////////

#define EXECUTE_BSR_TIMES_VEC_TEST(SCALAR, ORDINAL, OFFSET, DEVICE)               \
  TEST_F(                                                                         \
      TestCategory,                                                               \
      sparse##_##bsrmat_times_vec##_##SCALAR##_##ORDINAL##_##OFFSET##_##DEVICE) { \
    testSpMVBsrMatrix<SCALAR, ORDINAL, OFFSET, DEVICE>();                         \
  }

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&      \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&        \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(double, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&          \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||     \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(double, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&         \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&    \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&           \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(double, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&          \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||  \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(double, int64_t, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&       \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&        \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(float, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&           \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||     \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(float, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&          \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&    \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&           \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(float, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&           \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||  \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(float, int64_t, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&            \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||            \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_double, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&        \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||            \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_double, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&            \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||         \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_double, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&        \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||         \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_double, int64_t, size_t,
                           TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&           \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||           \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_float, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&       \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||           \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_float, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&           \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||        \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_float, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&       \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||        \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_VEC_TEST(kokkos_complex_float, int64_t, size_t, TestExecSpace)
#endif

#undef EXECUTE_BSR_TIMES_VEC_TEST

//////////////////////////

#define EXECUTE_BSR_TIMES_MVEC_TEST(SCALAR, ORDINAL, OFFSET, DEVICE)                   \
  TEST_F(                                                                              \
      TestCategory,                                                                    \
      sparse##_##bsrmat_times_multivec##_##SCALAR##_##ORDINAL##_##OFFSET##_##DEVICE) { \
    testBsrMatrix_SpM_MV<SCALAR, ORDINAL, OFFSET, DEVICE>();                           \
  }

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&      \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&        \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(double, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&          \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||     \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(double, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&         \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&    \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&           \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(double, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_DOUBLE) &&          \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||  \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(double, int64_t, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&       \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&        \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(float, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&           \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||     \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(float, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&          \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&    \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) || \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&           \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(float, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_FLOAT) &&           \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) && \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||  \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&            \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(float, int64_t, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&            \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||            \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_double, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&        \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||            \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_double, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&            \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||         \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_double, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_DOUBLE_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&        \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||         \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                   \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_double, int64_t, size_t,
                            TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&           \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||           \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_float, int, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&       \
     defined(KOKKOSKERNELS_INST_OFFSET_INT)) ||           \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_float, int64_t, int, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT) &&           \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||        \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_float, int, size_t, TestExecSpace)
#endif

#if (defined(KOKKOSKERNELS_INST_KOKKOS_COMPLEX_FLOAT_) && \
     defined(KOKKOSKERNELS_INST_ORDINAL_INT64_T) &&       \
     defined(KOKKOSKERNELS_INST_OFFSET_SIZE_T)) ||        \
    (!defined(KOKKOSKERNELS_ETI_ONLY) &&                  \
     !defined(KOKKOSKERNELS_IMPL_CHECK_ETI_CALLS))
EXECUTE_BSR_TIMES_MVEC_TEST(kokkos_complex_float, int64_t, size_t,
                            TestExecSpace)
#endif

#undef EXECUTE_BSR_TIMES_MVEC_TEST
