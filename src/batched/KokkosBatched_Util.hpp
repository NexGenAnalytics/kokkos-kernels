#ifndef __KOKKOSBATCHED_UTIL_HPP__
#define __KOKKOSBATCHED_UTIL_HPP__

/// \author Kyungjoo Kim (kyukim@sandia.gov)

// no experimental name space guard for trilinos
#define __KOKKOSBATCHED_PROMOTION__ 1

#include <iomanip>
#include <random>
#include <string>

#include <cassert>
#include <limits>
#include <cmath>
#include <ctime>

#include <complex>

#include "Kokkos_Core.hpp"
#include "Kokkos_Complex.hpp"
#include "Kokkos_ArithTraits.hpp"
#include "Kokkos_Timer.hpp"

#include "KokkosKernels_config.h"

// TPL macros
#if defined (KOKKOSKERNELS_ENABLE_TPL_MKL) 
#define __KOKKOSBATCHED_ENABLE_INTEL_MKL__ 1
#include "mkl_version.h"
#if __INTEL_MKL__ >= 2018
#define __KOKKOSBATCHED_ENABLE_INTEL_MKL_BATCHED__ 1    
#define __KOKKOSBATCHED_ENABLE_INTEL_MKL_COMPACT_BATCHED__ 1
#include "mkl.h"
  //#include "mkl_types.h"
#endif
#endif

#if defined(KOKKOSKERNELS_ENABLE_TPL_LAPACKE)
#define __KOKKOSBATCHED_ENABLE_LAPACKE__ 1
#include "lapacke.h"
#endif

namespace KokkosBatched {

//////// Helper macros, functions, and classes ////////
#define Int2StringHelper(A) #A
#define Int2String(A) Int2StringHelper(A)
#define StringCat(A,B) A B

  void print_compiler_info();

  template<typename T> struct is_vector : public std::false_type {};

  template<typename Ta, typename Tb>
  struct is_same_mag_type {
    static const bool is_specialized = ( Kokkos::Details::ArithTraits<Ta>::is_specialized &&
                                         Kokkos::Details::ArithTraits<Tb>::is_specialized );
      
    static const bool is_mag_type_same = std::is_same<typename Kokkos::Details::ArithTraits<Ta>::mag_type,
                                                      typename Kokkos::Details::ArithTraits<Tb>::mag_type>::value;
      
    static const bool value = is_specialized && is_mag_type_same;
  };
    
  // to use double, std::complex<double>, Kokkos::complex<double>
  using std::abs;
  using std::min;
  using std::max;

  // view manipulation
  template <typename MemoryTraitsType, Kokkos::MemoryTraitsFlags flag>
  using MemoryTraits = Kokkos::MemoryTraits<MemoryTraitsType::Unmanaged |
                                            MemoryTraitsType::RandomAccess |
                                            //  MemoryTraitsType::Atomic |
                                            flag>;

  template <typename ViewType>
  using UnmanagedViewType
  = Kokkos::View<typename ViewType::data_type,
                 typename ViewType::array_layout,
                 typename ViewType::device_type,
                 MemoryTraits<typename ViewType::memory_traits, Kokkos::Unmanaged> >;

  template <typename ViewType>
  using ConstViewType = Kokkos::View<typename ViewType::const_data_type,
                                     typename ViewType::array_layout,
                                     typename ViewType::device_type,
                                     typename ViewType::memory_traits>;
  template <typename ViewType>
  using ConstUnmanagedViewType = ConstViewType<UnmanagedViewType<ViewType> >;

  template <typename ViewType>
  using ScratchViewType
  = Kokkos::View<typename ViewType::data_type,
                 typename ViewType::array_layout,
                 typename ViewType::execution_space::scratch_memory_space,
                 MemoryTraits<typename ViewType::memory_traits, Kokkos::Unmanaged> >;


  // helper for vector type
  template<typename T>
  KOKKOS_INLINE_FUNCTION
  typename std::enable_if<std::is_fundamental<T>::value,size_t>::type
  adjustDimension(const size_t &m) {
    return m;
  }

  template<typename T>
  KOKKOS_INLINE_FUNCTION
  typename std::enable_if<!std::is_fundamental<T>::value,size_t>::type
  adjustDimension(const size_t &m) {
    return (m/T::vector_length + (m%T::vector_length > 0));
  }

  template<size_t BufSize, typename SpaceType = Kokkos::DefaultExecutionSpace>
  struct Flush {
    typedef double value_type;

    // flush a large host buffer
    Kokkos::View<value_type*,SpaceType> _buf;
    Flush() : _buf("Flush::buf", BufSize/sizeof(double)) {
      Kokkos::deep_copy(_buf, 1);
    }

    KOKKOS_INLINE_FUNCTION
    void init(value_type &update) {
      update = 0;
    }

    KOKKOS_INLINE_FUNCTION
    void join(volatile value_type &update,
              const volatile value_type &input) {
      update += input;
    }

    KOKKOS_INLINE_FUNCTION
    void operator()(const int i, value_type &update) const {
      update += _buf[i];
    }

    void run() {
      double sum = 0;
      Kokkos::parallel_reduce(Kokkos::RangePolicy<SpaceType>(0,BufSize/sizeof(double)), *this, sum);
      SpaceType().fence();
      FILE *fp = fopen("/dev/null", "w");
      fprintf(fp, "%f\n", sum);
      fclose(fp);
    }

  };

  template<typename T, typename dummy = T>
  struct Random;

  template<typename T>
  struct Random<T, typename std::enable_if<std::is_same<T,double>::value ||
                                           std::is_same<T,float>::value, T>::type> {
    Random(const unsigned int seed = 0) { srand(seed); }
    T value() { 
      const auto val = (rand()/((T) RAND_MAX) - 0.5)*2.0;
      return val > 0 ? val + 1.0e-3 : val - 1.0e-3;
    }
  };

  template<typename T>
  struct Random<T, typename std::enable_if<std::is_same<T,std::complex<float> >::value ||
                                           std::is_same<T,std::complex<double> >::value ||
                                           std::is_same<T,Kokkos::complex<float> >::value ||
                                           std::is_same<T,Kokkos::complex<double> >::value, T>::type> {
    Random(const unsigned int seed = 0) { srand(seed); }
    T value() {
      const auto rval = (rand()/((double) RAND_MAX) - 0.5)*2.0;        
      const auto ival = (rand()/((double) RAND_MAX) - 0.5)*2.0;        
      return T(rval > 0 ? rval + 1.0e-3 : rval - 1.0e-3,
               ival > 0 ? ival + 1.0e-3 : ival - 1.0e-3);
    }
  };

  struct Timer {
    std::string _label;
    Kokkos::Timer _clock;
    Timer (const std::string label)
      : _label(label), _clock() {};

    void reset() { _clock.reset(); }
    double seconds() { return _clock.seconds(); }
    ~Timer() {
      Kokkos::fence();
      const double t = _clock.seconds();
      std::string label = _label; label.resize(24);
      std::cout << "KokkosKernels::Timer:: " << std::setw(26) << label
                << std::setw(15) << std::scientific << t << " [sec] " << std::endl;
    }
  };

  // Implicit vectorization
  template<typename T>
  struct SIMD {
    static_assert( std::is_same<T,bool>::value                     ||
                   std::is_same<T,int>::value                      ||
                   std::is_same<T,size_t>::value                   ||
                   std::is_same<T,double>::value                   ||
                   std::is_same<T,float>::value                    ||
                   std::is_same<T,Kokkos::complex<float> >::value  ||
                   std::is_same<T,std::complex<float> >::value     ||
                   std::is_same<T,Kokkos::complex<double> >::value ||
                   std::is_same<T,std::complex<double> >::value    ||
                   std::is_same<T,Kokkos::Experimental::half_t>::value,
                   "KokkosKernels:: Invalid SIMD<> type." );
    using value_type = T;
  };

  // Intel AVX instruction device (explicit vectorization)
  template<typename T>
  struct AVX {
    static_assert( std::is_same<T,double>::value                   ||
                   std::is_same<T,float>::value                    ||
                   std::is_same<T,Kokkos::complex<double> >::value ||
                   std::is_same<T,std::complex<double> >::value,
                   "KokkosKernels:: Invalid AVX<> type." );
    using value_type = T;
  };

  //////// Tags for BLAS ////////
  struct Trans {
    struct Transpose {};
    struct NoTranspose {};
    struct ConjTranspose {};
  };

  struct Side {
    struct Left {};
    struct Right {};
  };

  struct Uplo {
    struct Upper {};
    struct Lower {};
  };

  struct Diag {
    struct Unit    { static const bool use_unit_diag = true;  };
    struct NonUnit { static const bool use_unit_diag = false; };
  };

  
  /// \brief: BatchLayout class used to specify where the batch dimension is allocated in
  ///         the input views for host-level Batched BLAS/LAPACK routines.
  /// \var Left  Batch dimension is the leftmost dimension within input views
  /// \var Right Batch dimension is the rightmost dimension within input views
  struct BatchLayout {
    struct Left {};
    struct Right {};
  };

  /// \brief ResultsPerThread class used to specify how to divide a given
  ///        BLAS/LAPACK operation among Kokkos threads
  /// \var Rank0 Each Kokkos thread calculates a 0-rank result
  /// \var Rank1 Each Kokkos thread calculates a 1-rank result
  /// \var Rank2 Each Kokkos thread calculates a 2-rank result
  struct ResultsPerThread {
    struct Rank0 {};
    struct Rank1 {};
    struct Rank2 {};
  };

  /// \brief BoundsCheck class used to specify whether to check view bounds in
  ///        BLAS/LAPACK DblBuf algorithms.
  /// /var Yes Use functor with    bounds check
  /// /var No  Use functor without bound checks
  struct BoundsCheck {
    struct Yes {};
    struct No {};
  };

  struct Direct {
    struct Forward {};
    struct Backward {};
  };

  struct Mode {
    struct Serial {
      static const char *name() { return "Serial"; }
    };
    struct Team {
      static const char *name() { return "Team"; }
    };
    struct TeamVector {
      static const char *name() { return "TeamVector"; }
    };
  };

  struct Algo {      
    struct Level3 {
      struct Unblocked {
        static const char* name() { return "Unblocked"; }
      };
      struct Blocked {
        static const char* name() { return "Blocked"; }
        // TODO:: for now harwire the blocksizes; this should reflect
        // regieter blocking (not about team parallelism).
        // this mb should vary according to
        // - team policy (smaller) or range policy (bigger)
        // - space (gpu vs host)
        // - blocksize input (blk <= 4 mb = 2, otherwise mb = 4), etc.
#if defined(KOKKOS_ENABLE_CUDA)
        template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
        typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::CudaSpace>::value,int>
        ::type mb() { return 2; }
#endif
#if defined(KOKKOS_ENABLE_HIP)
        template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
        typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::Experimental::HIPSpace>::value,int>
        ::type mb() { return 2; }
#endif
#if defined(KOKKOS_ENABLE_SYCL)
        template <typename ActiveMemorySpaceType>
        KOKKOS_INLINE_FUNCTION static constexpr typename std::enable_if<
            std::is_same<ActiveMemorySpaceType,
                         Kokkos::Experimental::SYCLDeviceUSMSpace>::value,
            int>::type
        mb() {
          return 2;
        }
#endif
        template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
        typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::HostSpace>::value,int>
        ::type mb() { return 4; }
      };
      struct MKL {
        static const char* name() { return "MKL"; }
      };
      struct CompactMKL {
        static const char* name() { return "CompactMKL"; }
      };

      // When this is first developed, unblocked algorithm is a naive implementation 
      // and blocked algorithm uses register blocking variant of algorithm (manual unrolling). 
      // This distinction is almost meaningless and it just adds more complications. 
      // Eventually, the blocked version will be removed and we only use the default 
      // algorithm. For testing and development purpose, we still leave algorithm tag
      // in the template arguments.
      using Default = Unblocked;
    };

    using Gemm = Level3;
    using Trsm = Level3;
    using Trmm = Level3;
    using Trtri = Level3;
    using LU   = Level3;
    using InverseLU = Level3;
    using SolveLU   = Level3;
    using QR = Level3;
    using UTV = Level3;

    struct Level2 {
      struct Unblocked {};
      struct Blocked {
        // TODO:: for now harwire the blocksizes; this should reflect
        // regieter blocking (not about team parallelism).
        // this mb should vary according to
        // - team policy (smaller) or range policy (bigger)
        // - space (cuda vs host)
        // - blocksize input (blk <= 4 mb = 2, otherwise mb = 4), etc.
#if defined(KOKKOS_ENABLE_CUDA)
        template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
        typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::CudaSpace>::value,int>
        ::type mb() { return 1; }
#endif
#if defined(KOKKOS_ENABLE_HIP)
        template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
        typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::Experimental::HIPSpace>::value,int>
        ::type mb() { return 1; }
#endif
#if defined(KOKKOS_ENABLE_SYCL)
        template <typename ActiveMemorySpaceType>
        KOKKOS_INLINE_FUNCTION static constexpr typename std::enable_if<
            std::is_same<ActiveMemorySpaceType,
                         Kokkos::Experimental::SYCLDeviceUSMSpace>::value,
            int>::type
        mb() {
          return 1;
        }
#endif
        template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
        typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::HostSpace>::value,int>
        ::type mb() { return 4; }
      };
      struct MKL {};
      struct CompactMKL {};

      // When this is first developed, unblocked algorithm is a naive implementation 
      // and blocked algorithm uses register blocking variant of algorithm (manual unrolling). 
      // This distinction is almost meaningless and it just adds more complications. 
      // Eventually, the blocked version will be removed and we only use the default 
      // algorithm. For testing and development purpose, we still leave algorithm tag
      // in the template arguments.
      using Default = Unblocked;
    };

    using Gemv = Level2;
    using Trsv = Level2;
    using ApplyQ = Level2;

    //         struct Level1 {
    //           struct Unblocked {};
    //           struct Blocked {
    //             // TODO:: for now harwire the blocksizes; this should reflect
    //             // regieter blocking (not about team parallelism).
    //             // this mb should vary according to
    //             // - team policy (smaller) or range policy (bigger)
    //             // - space (cuda vs host)
    //             // - blocksize input (blk <= 4 mb = 2, otherwise mb = 4), etc.
    // #if defined(KOKKOS_ENABLE_CUDA)
    //             template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
    //             typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::CudaSpace>::value,int>
    //             ::type mb() { return 4; }
    // #endif
    //             template<typename ActiveMemorySpaceType> KOKKOS_INLINE_FUNCTION static constexpr
    //             typename std::enable_if<std::is_same<ActiveMemorySpaceType,Kokkos::HostSpace>::value,int>
    //             ::type mb() { return 4; }
    //           };
    //           //struct MKL {};
    //           //struct CompactMKL {};
    //         };

  };

  struct Util {

    template<typename ValueType>
    KOKKOS_INLINE_FUNCTION
    static void
    packColMajor(ValueType *__restrict__ A,
                 const int m,
                 const int n,
                 const ValueType *__restrict__ B,
                 const int bs0,
                 const int bs1) {
      for (int j=0;j<n;++j)
        for (int i=0;i<m;++i)
          A[i+j*m] = B[i*bs0+j*bs1];
    }

    template<typename ValueType>
    KOKKOS_INLINE_FUNCTION
    static void
    packRowMajor(ValueType *__restrict__ A,
                 const int m,
                 const int n,
                 const ValueType *__restrict__ B,
                 const int bs0,
                 const int bs1) {
      for (int i=0;i<m;++i)
        for (int j=0;j<n;++j)
          A[i*n+j] = B[i*bs0+j*bs1];
    }

  };

  template<typename ValueType> struct Partition1x2;
  template<typename ValueType> struct Partition1x3;
    
  template<typename ValueType>
  struct Partition1x2 {      
    const int as1;
    ValueType *AL, *AR;

    KOKKOS_INLINE_FUNCTION
    Partition1x2(const int arg_as1)
      : as1(arg_as1), AL(NULL), AR(NULL) {}

    KOKKOS_INLINE_FUNCTION
    void partWithAL(ValueType *A, const int /* nA */, const int nAL) {
      AL = A; AR = AL+nAL*as1;
    }

    KOKKOS_INLINE_FUNCTION
    void partWithAR(ValueType *A, const int nA, const int nAR) {
      AL = A; AR = AL+(nA-nAR)*as1;
    }

    // A0 A1 are merged into AL
    KOKKOS_INLINE_FUNCTION
    void mergeToAL(const Partition1x3<ValueType> &part) {
      AL = part.A0; AR = part.A2;
    }      

    // A0 A1 are merged into AL
    KOKKOS_INLINE_FUNCTION
    void mergeToAR(const Partition1x3<ValueType> &part) {
      AL = part.A0; AR = part.A1;
    }      
  };

  template<typename ValueType>
  struct Partition1x3 {      
    const int as1;
    ValueType *A0, *A1, *A2;

    KOKKOS_INLINE_FUNCTION
    Partition1x3(const int arg_as1)
      : as1(arg_as1), A0(NULL), A1(NULL), A2(NULL) {}

    KOKKOS_INLINE_FUNCTION
    void partWithAL(const Partition1x2<ValueType> &part, const int mA1) {
      A0 = part.AL; A2 = part.AR; A1 = A2 - mA1*as1;
    }
    KOKKOS_INLINE_FUNCTION
    void partWithAR(const Partition1x2<ValueType> &part, const int mA1) {
      A0 = part.AL; A1 = part.AR; A2 = A1 + mA1*as1;
    }
  };


  template<typename ValueType> struct Partition2x1;
  template<typename ValueType> struct Partition3x1;
    
  template<typename ValueType>
  struct Partition2x1 {      
    const int as0;
    ValueType *AT, *AB;

    KOKKOS_INLINE_FUNCTION
    Partition2x1(const int arg_as0)
      : as0(arg_as0), AT(NULL), AB(NULL) {}

    KOKKOS_INLINE_FUNCTION
    void partWithAT(ValueType *A, const int /* mA */, const int mAT) {
      AT = A;
      AB = AT+mAT*as0;
    }

    KOKKOS_INLINE_FUNCTION
    void partWithAB(ValueType *A, const int mA, const int mAB) {
      partWithAT(A, mA, mA-mAB);
    }

    // A0
    // A1 is merged into AT
    KOKKOS_INLINE_FUNCTION
    void mergeToAT(const Partition3x1<ValueType> &part) {
      AT = part.A0;
      AB = part.A2;
    }      

    KOKKOS_INLINE_FUNCTION
    void mergeToAB(const Partition3x1<ValueType> &part) {
      AT = part.A0;
      AB = part.A1;
    }      
  };

  template<typename ValueType>
  struct Partition3x1 {      
    const int as0;
    ValueType *A0,
      /* */   *A1,
      /* */   *A2;
      
    KOKKOS_INLINE_FUNCTION
    Partition3x1(const int arg_as0)
      : as0(arg_as0), 
        A0(NULL),  
        A1(NULL),  
        A2(NULL) {}
      
    KOKKOS_INLINE_FUNCTION
    void partWithAB(const Partition2x1<ValueType> &part, const int mA1) {
      A0 = part.AT;      
      A1 = part.AB;      
      A2 = A1 + mA1*as0;
    }

    KOKKOS_INLINE_FUNCTION
    void partWithAT(const Partition2x1<ValueType> &part, const int mA1) {
      A0 = part.AT;      
      A1 = part.AB - mA1*as0;
      A2 = part.AB;
    }
  };

  template<typename ValueType> struct Partition2x2;
  template<typename ValueType> struct Partition3x3;

  template<typename ValueType>
  struct Partition2x2 {      
    const int as0, as1;
    ValueType *ATL, *ATR, *ABL, *ABR;

    KOKKOS_INLINE_FUNCTION
    Partition2x2(const int arg_as0, const int arg_as1) 
      : as0(arg_as0), as1(arg_as1), ATL(NULL), ATR(NULL), ABL(NULL), ABR(NULL) {}

    KOKKOS_INLINE_FUNCTION
    void partWithATL(ValueType *A, 
                     const int /* mA */, const int /* nA */, 
                     const int mATL, const int nATL) {
      ATL = A;            ATR = ATL+nATL*as1; 
      ABL = ATL+mATL*as0; ABR = ABL+nATL*as1;
    }

    KOKKOS_INLINE_FUNCTION
    void partWithABR(ValueType *A, 
                     const int mA, const int nA, 
                     const int mABR, const int nABR) {
      partWithATL(A, mA, nA, mA-mABR, nA-nABR);
    }

    // A00 A01
    // A10 A11 is merged into ATL
    KOKKOS_INLINE_FUNCTION
    void mergeToATL(const Partition3x3<ValueType> &part) {
      ATL = part.A00; ATR = part.A02;
      ABL = part.A20; ABR = part.A22;
    }      

    KOKKOS_INLINE_FUNCTION
    void mergeToABR(const Partition3x3<ValueType> &part) {
      ATL = part.A00; ATR = part.A01;
      ABL = part.A10; ABR = part.A11;
    }      
  };

  template<typename ValueType>
  struct Partition3x3 {      
    const int as0, as1;
    ValueType *A00, *A01, *A02,
      /* */   *A10, *A11, *A12,
      /* */   *A20, *A21, *A22;

      
    KOKKOS_INLINE_FUNCTION
    Partition3x3(const int arg_as0, const int arg_as1)
      : as0(arg_as0), as1(arg_as1), 
        A00(NULL), A01(NULL), A02(NULL), 
        A10(NULL), A11(NULL), A12(NULL), 
        A20(NULL), A21(NULL), A22(NULL) {}
      
    KOKKOS_INLINE_FUNCTION
    void partWithABR(const Partition2x2<ValueType> &part, const int mA11, const int nA11) {
      A00 = part.ATL;
      A01 = part.ATR;
      A02 = part.ATR + nA11 * as1;
      A10 = part.ABL;
      A11 = part.ABR;
      A12 = part.ABR + nA11 * as1;
      A20 = part.ABL + mA11 * as0;
      A21 = part.ABR + mA11 * as0;
      A22 = part.ABR + mA11 * as0 + nA11 * as1;
    }

    KOKKOS_INLINE_FUNCTION
    void partWithATL(const Partition2x2<ValueType> &part, const int mA11,
                     const int nA11) {
      A00 = part.ATL;
      A01 = part.ATR - nA11 * as1;
      A02 = part.ATR;
      A10 = part.ABL - mA11 * as0;
      A11 = part.ABR - mA11 * as0 - nA11 * as1;
      A12 = part.ABR - mA11 * as0;
      A20 = part.ABL;
      A21 = part.ABR - nA11 * as1;
      A22 = part.ABR;
    }
  };

  template <class ViewType>
  KOKKOS_INLINE_FUNCTION auto transpose_2d_view(ViewType v, const int *order) {
    constexpr int rank             = 2;
    const int dim[]            = {v.extent_int(1), v.extent_int(0)};
    using view_value_type      = typename ViewType::value_type;
    using execution_space_type = typename ViewType::execution_space;
    using view_type = Kokkos::View<view_value_type **, Kokkos::LayoutStride,
                                   execution_space_type>;
    Kokkos::LayoutStride stride =
        Kokkos::LayoutStride::order_dimensions(rank, order, dim);

    return view_type(v.data(), stride);
  }

  template <class ViewType>
  KOKKOS_INLINE_FUNCTION auto transpose_2d_view(ViewType v,
                                                const BatchLayout::Left &) {
    const int order[] = {0, 1};  // v is LayoutRight
    return transpose_2d_view(v, order);
  }

  template <class ViewType>
  KOKKOS_INLINE_FUNCTION auto transpose_2d_view(ViewType v,
                                                const BatchLayout::Right &) {
    const int order[] = {1, 0};  // v is LayoutLeft
    return transpose_2d_view(v, order);
  }

  ///// subview_wrapper overloads for handling 3-rank BatchLayout::Left views
  template <class ViewType, class IdxType1, class IdxType2, class IdxType3>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(ViewType v, IdxType1 i1,
                                              IdxType2 i2, IdxType3 i3,
                                              const BatchLayout::Left &) {
    return Kokkos::subview(v, i1, i2, i3);
  }
  template <class ViewType, class IdxType1, class IdxType2, class IdxType3>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(
      ViewType v, IdxType1 i1, IdxType2 i2, IdxType3 i3,
      const BatchLayout::Left &layout_tag, const Trans::NoTranspose) {
    return subview_wrapper(v, i1, i2, i3, layout_tag);
  }
  template <class ViewType, class IdxType1>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(
      ViewType v, IdxType1 i1, Kokkos::Impl::ALL_t i2, Kokkos::Impl::ALL_t i3,
      const BatchLayout::Left &layout_tag, const Trans::Transpose) {
    auto sv_nt = subview_wrapper(v, i1, i3, i2, layout_tag);

    return transpose_2d_view(sv_nt, layout_tag);
  }
  template <class ViewType, class IdxType1, class IdxType2, class IdxType3>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(
      ViewType v, IdxType1 i1, IdxType2 i2, IdxType3 i3,
      const BatchLayout::Left &layout_tag, const Trans::Transpose) {
    auto sv_nt = subview_wrapper(v, i1, i3, i2, layout_tag);

    return sv_nt;
  }

  //// subview_wrapper overloads for handling 3-rank BatchLayout::Right views
  template <class ViewType, class IdxType1, class IdxType2, class IdxType3>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(ViewType v, IdxType1 i1,
                                              IdxType2 i2, IdxType3 i3,
                                              const BatchLayout::Right &) {
    return Kokkos::subview(v, i2, i3, i1);
  }
  template <class ViewType, class IdxType1, class IdxType2, class IdxType3>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(
      ViewType v, IdxType1 i1, IdxType2 i2, IdxType3 i3,
      const BatchLayout::Right &layout_tag, const Trans::NoTranspose &) {
    return subview_wrapper(v, i1, i2, i3, layout_tag);
  }
  template <class ViewType, class IdxType1>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(
      ViewType v, IdxType1 i1, Kokkos::Impl::ALL_t i2, Kokkos::Impl::ALL_t i3,
      const BatchLayout::Right &layout_tag, const Trans::Transpose &) {
    auto sv_nt = subview_wrapper(v, i1, i3, i2, layout_tag);

    return transpose_2d_view(sv_nt, layout_tag);
  }
  template <class ViewType, class IdxType1, class IdxType2, class IdxType3>
  KOKKOS_INLINE_FUNCTION auto subview_wrapper(
      ViewType v, IdxType1 i1, IdxType2 i2, IdxType3 i3,
      const BatchLayout::Right &layout_tag, const Trans::Transpose &) {
    auto sv_nt = subview_wrapper(v, i1, i3, i2, layout_tag);

    return sv_nt;
  }

  template <class ViewValueType, class ViewType>
  KOKKOS_INLINE_FUNCTION ViewValueType
  access_view_bounds_check(ViewType v, int m, int n, const BoundsCheck::Yes &) {
    if (m < v.extent_int(0) && n < v.extent_int(1)) return v(m, n);
    return (ViewValueType)0.0F;
  }

  template <class ViewValueType, class ViewType>
  KOKKOS_INLINE_FUNCTION ViewValueType
  access_view_bounds_check(ViewType v, int m, int n, const BoundsCheck::No &) {
    return v(m, n);
  }

  template <class ViewType, class SizeType, class ViewValueType,
            class ScalarType>
  KOKKOS_INLINE_FUNCTION void fma_bounds_check(ViewType v, SizeType m,
                                               SizeType n, ViewValueType reg_c,
                                               ScalarType beta,
                                               const BoundsCheck::Yes &) {
    if (m < v.extent_int(0) && n < v.extent_int(1))
      v(m, n) = reg_c + v(m, n) * beta;
  }

  template <class ViewType, class SizeType, class ViewValueType,
            class ScalarType>
  KOKKOS_INLINE_FUNCTION void fma_bounds_check(ViewType v, SizeType m,
                                               SizeType n, ViewValueType reg_c,
                                               ScalarType beta,
                                               const BoundsCheck::No &) {
    v(m, n) = reg_c + v(m, n) * beta;
  }

  template <class ViewType, class SizeType, class ScalarType>
  KOKKOS_INLINE_FUNCTION void fma_bounds_check(ViewType v, SizeType m,
                                               SizeType n, ScalarType reg_c,
                                               const BoundsCheck::Yes &) {
    if (m < v.extent_int(0) && n < v.extent_int(1)) v(m, n) = reg_c;
  }

  template <class ViewType, class SizeType, class ScalarType>
  KOKKOS_INLINE_FUNCTION void fma_bounds_check(ViewType v, SizeType m,
                                               SizeType n, ScalarType reg_c,
                                               const BoundsCheck::No &) {
    v(m, n) = reg_c;
  }

  }  // namespace KokkosBatched
#endif  // __KOKKOSBATCHED_UTIL_HPP__
