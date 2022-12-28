MESSAGE( "ENter in FindTPLROCBLAS.cmake")

# MPL: 12/26/2022: This bloc is not necessary anymore since ROCBLAS installation provide a cmake config file.
# Should we verify for different version of ROCBLAS ?
# GOAL: The code is commented for now and we aim to remove it
#IF (ROCBLAS_LIBRARY_DIRS AND ROCBLAS_LIBRARIES)
#  KOKKOSKERNELS_FIND_IMPORTED(ROCBLAS INTERFACE LIBRARIES ${ROCBLAS_LIBRARIES} LIBRARY_PATHS ${ROCBLAS_LIBRARY_DIRS})
#ELSEIF (ROCBLAS_LIBRARIES)
#  KOKKOSKERNELS_FIND_IMPORTED(ROCBLAS INTERFACE LIBRARIES ${ROCBLAS_LIBRARIES})
#ELSEIF (ROCBLAS_LIBRARY_DIRS)

## MPL: 12/26/2022: USE FIND_PACKAGE and check if the requested target is the more modern way to do it
#FIND_PACKAGE(ROCBLAS)
#if(TARGET roc::rocblas)
#    MESSAGE( "TARGET roc::rocblas FOUND")
#ELSE()
#    MESSAGE( "TARGET roc::rocblas NOT FOUND")
#endif()

## MPL: 12/26/2022: USING FIND_PACKAGE_HANDLE_STANDARD_ARGS can be ok in modern CMAKE but with a Find module
## if the package is found, we can verify that some variables are defined using FIND_PACKAGE_HANDLE_STANDARD_ARGS
## and set TPLROCBLAS_FOUND to true, that can be useful to
## if the package is not found, we can try to find it using find_path and find_library (e.g KOKKOSKERNELS_FIND_IMPORTED)
#INCLUDE(FindPackageHandleStandardArgs)
#IF (NOT ROC_FOUND)
#  message("NOT ROC_FOUND")
#  #Important note here: this find Module is named TPLROCBLAS
#  #The eventual target is named ROCBLAS. To avoid naming conflicts
#  #the find module is called TPLROCBLAS. This call will cause
#  #the find_package call to fail in a "standard" CMake way
#  message("TPLROCBLAS BEFORE FIND ${TPLROCBLAS_FOUND}")
#  FIND_PACKAGE_HANDLE_STANDARD_ARGS(TPLROCBLAS REQUIRED_VARS ROCBLAS_LIBRARIES ROCBLAS_INCLUDE_DIRS)
#  message("TPLROCBLAS AFTER FIND ${TPLROCBLAS_FOUND}")
#  message("TPLROCBLAS LIBRARIES VARIABLE IS ${ROCBLAS_LIBRARIES}")
#  message("NOT ROC_FOUND")
#  KOKKOSKERNELS_CREATE_IMPORTED_TPL(ROCBLAS INTERFACE LINK_LIBRARIES )
#ELSE()
#  message("ROC_FOUND")
#  #The libraries might be empty - OR they might explicitly be not found
#  IF("${ROCBLAS_LIBRARIES}" MATCHES "NOTFOUND")
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(TPLROCBLAS REQUIRED_VARS ROCBLAS_LIBRARIES)
#  ELSE()
#    KOKKOSKERNELS_CREATE_IMPORTED_TPL(ROCBLAS INTERFACE
#      LINK_LIBRARIES "${ROCBLAS_LIBRARIES}")
#  ENDIF()
#ENDIF()

IF (ROCBLAS_LIBRARY_DIRS AND ROCBLAS_LIBRARIES)
  KOKKOSKERNELS_FIND_IMPORTED(ROCBLAS INTERFACE LIBRARIES ${ROCBLAS_LIBRARIES} LIBRARY_PATHS ${ROCBLAS_LIBRARY_DIRS})
ELSEIF (ROCBLAS_LIBRARIES)
  KOKKOSKERNELS_FIND_IMPORTED(ROCBLAS INTERFACE LIBRARIES ${ROCBLAS_LIBRARIES})
ELSEIF (ROCBLAS_LIBRARY_DIRS)
  KOKKOSKERNELS_FIND_IMPORTED(ROCBLAS INTERFACE LIBRARIES rocblas LIBRARY_PATHS ${ROCBLAS_LIBRARY_DIRS})
ELSEIF (KokkosKernels_ROCBLAS_ROOT)
  KOKKOSKERNELS_FIND_IMPORTED(ROCBLAS INTERFACE
    LIBRARIES
      rocblas
    LIBRARY_PATHS
      ${KokkosKernels_ROCBLAS_ROOT}/lib
    HEADERS
      rocblas.h
    HEADER_PATHS
      ${KokkosKernels_ROCBLAS_ROOT}/include
  )
  message("V1: TPLROCBLAS LIBRARIES VARIABLE IS ${ROCBLAS_FOUND_LIBRARIES}")
ELSEIF (DEFINED ENV{ROCM_PATH})
  MESSAGE(STATUS "Detected ROCM_PATH: ENV{ROCM_PATH}")
  SET(ROCBLAS_ROOT "$ENV{ROCM_PATH}/rocblas")
  KOKKOSKERNELS_FIND_IMPORTED(ROCBLAS INTERFACE
    LIBRARIES
      rocblas
    LIBRARY_PATHS
      ${ROCBLAS_ROOT}/lib
    HEADERS
      rocblas.h
    HEADER_PATHS
      ${ROCBLAS_ROOT}/include
  )
ELSE()
  MESSAGE(ERROR "rocBLAS was not detected properly, please disable it or provide sufficient information at configure time.")
  # Todo: figure out how to use the target defined during rocblas installation
  # FIND_PACKAGE(ROCBLAS REQUIRED)
  # KOKKOSKERNELS_CREATE_IMPORTED_TPL(ROCBLAS INTERFACE LINK_LIBRARIES ${ROCBLAS_LIBRARIES})
  # GET_TARGET_PROPERTY(ROCBLAS_LINK_LIBRARIES ${ROCBLAS_LIBRARIES} IMPORTED_LINK_INTERFACE_LIBRARIES)
ENDIF()
