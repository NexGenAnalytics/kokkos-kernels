
KOKKOSKERNELS_CONFIGURE_FILE(KokkosKernels_config.h)

KOKKOSKERNELS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
KOKKOSKERNELS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(KK_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

#I don't want the complexity of sometimes interface/sometimes not
#Just minimally force an empty dummy file
SET(SOURCES dummy.cpp)

#-----------------------------------------------------------------------------

LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/impl)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/impl)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/impl/tpls)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/blas)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/blas/impl)

#Include Sparse
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/sparse)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/sparse/impl)

#Include graph
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/graph)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/graph/impl)

#Include batched
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/batched)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/batched/dense)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/batched/dense/impl)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/batched/sparse)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/batched/sparse/impl)

#Include Common
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/common)
LIST(APPEND KK_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/impl)


FOREACH(DIR ${KK_INCLUDE_DIRS})
  KOKKOSKERNELS_INCLUDE_DIRECTORIES(${DIR})
  APPEND_GLOB(HEADERS ${DIR}/*.hpp)
ENDFOREACH()


#Include BLAS, Blas host wrapper
IF (KOKKOSKERNELS_ENABLE_TPL_BLAS OR KOKKOSKERNELS_ENABLE_TPL_MKL)
  #Do NOT add this to include path
  APPEND_GLOB(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/impl/tpls/KokkosBlas_Host_tpl.cpp)
ENDIF()

include(kokkoskernels_eti.cmake)
SET(ETI_HEADERS)

#Build up a list of DECL, AVAIL, and INST macros
#that should be instantiated based on input options
#Generate @X@ variables in the template X.hpp.in and X.cpp.in
#files containing the list of all needed macros
KOKKOSKERNELS_GENERATE_ETI(Blas1_abs abs
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_abs_mv abs
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_scal scal
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_scal_mv scal
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_dot dot
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_dot_mv dot
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas_gesv gesv
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_axpby axpby
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_axpby_mv axpby
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_update update
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_update_mv update
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_sum sum
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_sum_mv sum
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrm1 nrm1
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrm1_mv nrm1
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrm2w nrm2w
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrm2w_mv nrm2w
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrminf nrminf
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrminf_mv nrminf
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_iamax iamax
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_iamax_mv iamax
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrm2 nrm2
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_nrm2_mv nrm2
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_mult mult
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_mult_mv mult
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_reciprocal reciprocal
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas1_reciprocal_mv reciprocal
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas2_gemv gemv
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas3_gemm gemm
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas3_trsm trsm
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas3_trmm trmm
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Blas_trtri trtri
  COMPONENTS  blas
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_sptrsv_solve sptrsv_solve
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spmv_struct spmv
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spmv_mv_struct spmv
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spmv spmv
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spmv_mv spmv
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spgemm_symbolic spgemm_symbolic
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES_W_SLOW_SPACE
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spgemm_numeric spgemm_numeric
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES_W_SLOW_SPACE
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spgemm_jacobi spgemm_jacobi
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES_W_SLOW_SPACE
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spiluk_symbolic spiluk_symbolic
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_spiluk_numeric spiluk_numeric
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_sptrsv_symbolic sptrsv_symbolic
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_trsv trsv
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_gauss_seidel_symbolic gauss_seidel_symbolic
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES_W_SLOW_SPACE
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_gauss_seidel_numeric gauss_seidel_numeric
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES_W_SLOW_SPACE
)

KOKKOSKERNELS_GENERATE_ETI(Sparse_gauss_seidel_apply gauss_seidel_apply
  COMPONENTS  sparse
  HEADER_LIST ETI_HEADERS
  SOURCE_LIST SOURCES
  TYPE_LISTS  FLOATS ORDINALS OFFSETS LAYOUTS DEVICES_W_SLOW_SPACE
)

LIST(APPEND HEADERS ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}_config.h)

#Add a few other utility files
LIST(APPEND SOURCES
 batched/KokkosBatched_Util.cpp
 impl/tpls/KokkosBlas_Host_tpl.cpp
 impl/tpls/KokkosBlas_Cuda_tpl.cpp
 impl/tpls/KokkosKernels_tpl_handles.cpp
)

# For now, don't add the ETI headers to complete list of headers
# This will break some IDEs and won't properly display headers in the file list
# However, because of the behavior of TRIBITS_ADD_LIBRARY
# I can't add the ETI headers with them getting double-installed
# with a bunch of headers in the wrong location
# This doesn't change pre-existing behavior before the ETI changes
#LIST(APPEND HEADERS ${ETI_HEADERS})
#-----------------------------------------------------------------------------

KOKKOSKERNELS_ADD_LIBRARY(
  kokkoskernels
  HEADERS ${HEADERS}
  SOURCES ${SOURCES}
)

# It does not seem to be possible to automatically have headers installed
# in the correct subdirectory structure using HEADERS and ADD_LIBRARY with tribits
# Force the specializations to be installed in a subdirectory
INSTALL(FILES ${ETI_HEADERS} DESTINATION ${KOKKOSKERNELS_HEADER_INSTALL_DIR}/generated_specializations_hpp/)

IF (KOKKOSKERNELS_HAS_TRILINOS)
  #no linking commands required - tribits does this
ELSE()
  ADD_LIBRARY(Kokkos::kokkoskernels ALIAS kokkoskernels)
  TARGET_LINK_LIBRARIES(kokkoskernels PUBLIC Kokkos::kokkos)
  FOREACH(DIR ${KK_INCLUDE_DIRS})
    TARGET_INCLUDE_DIRECTORIES(kokkoskernels PUBLIC $<BUILD_INTERFACE:${DIR}>)
  ENDFOREACH()
  TARGET_INCLUDE_DIRECTORIES(kokkoskernels PUBLIC
    $<INSTALL_INTERFACE:${KOKKOSKERNELS_HEADER_INSTALL_DIR}>)
ENDIF()


KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC LAPACK)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC BLAS)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC CBLAS)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC LAPACKE)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC SUPERLU)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC CHOLMOD)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC MKL)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC CUBLAS)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC CUSPARSE)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC METIS)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC ARMPL)
KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC MAGMA)
# Not yet here KOKKOSKERNELS_LINK_TPL(kokkoskernels PUBLIC MAGMA)
