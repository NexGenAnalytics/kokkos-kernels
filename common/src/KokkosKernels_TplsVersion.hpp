//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#ifndef _KOKKOSKERNELS_TPLS_VERSIONS_HPP
#define _KOKKOSKERNELS_TPLS_VERSIONS_HPP

#include "KokkosKernels_config.h"
#include <iostream>

#if defined(KOKKOSKERNELS_ENABLE_TPL_ROCSPARSE)
#include <Kokkos_Core.hpp>
#include <rocsparse/rocsparse.h>
#include "KokkosSparse_Utils_rocsparse.hpp"
#endif

#if defined(KOKKOSKERNELS_ENABLE_TPL_ROCBLAS)
#include "KokkosBlas_tpl_spec.hpp"
#include <rocblas.h>
#endif

#if defined(KOKKOSKERNELS_ENABLE_TPL_CUBLAS)
#include "cublas_v2.h"

#endif

#if defined(KOKKOSKERNELS_ENABLE_TPL_CUSPARSE)
#include "cusparse.h"
#endif

namespace KokkosKernels {

#if defined(KOKKOSKERNELS_ENABLE_TPL_ROCSPARSE)
inline std::string get_rocsparse_version() {
  // Print version
  rocsparse_handle handle;
  rocsparse_create_handle(&handle);

  int ver;
  char rev[64];

  rocsparse_get_version(handle, &ver);
  rocsparse_get_git_rev(handle, rev);

  rocsparse_destroy_handle(handle);
  std::stringstream ss;

  ss << ver / 100000 << "." << ver / 100 % 1000 << "." << ver % 100 << "-"
     << rev;

  return ss.str();
}
#endif

#if defined(KOKKOSKERNELS_ENABLE_TPL_ROCBLAS)
inline std::string get_rocblas_version() {
  // Print version
  size_t size = 128;
  // rocblas_get_version_string_size(&size);
  std::string version(size - 1, '\0');
  rocblas_get_version_string(const_cast<char*>(version.data()), size);

  return version;
}
#endif

#if defined(KOKKOSKERNELS_ENABLE_TPL_CUBLAS)
inline std::string get_cublas_version() {
  // Print version
  std::stringstream ss;

  ss << CUBLAS_VER_MAJOR << "." << CUBLAS_VER_MINOR << "." << CUBLAS_VER_PATCH
     << CUBLAS_VER_BUILD;

  return ss.str();
}
#endif

#if defined(KOKKOSKERNELS_ENABLE_TPL_CUSPARSE)
inline std::string get_cusparse_version() {
  // Print version
  std::stringstream ss;

  ss << CUSPARSE_VER_MAJOR << "." << CUSPARSE_VER_MINOR << "."
     << CUSPARSE_VER_PATCH << CUSPARSE_VER_BUILD;

  return ss.str();
}
#endif

}  // namespace KokkosKernels
#endif  // _KOKKOSKERNELS_PRINT_CONFIGURATION_HPP
