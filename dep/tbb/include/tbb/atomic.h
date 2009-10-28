/*
    Copyright 2005-2009 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef __TBB_atomic_H
#define __TBB_atomic_H

#include <cstddef>
#include "tbb_stddef.h"

#if _MSC_VER 
#define __TBB_LONG_LONG __int64
#else
#define __TBB_LONG_LONG long long
#endif /* _MSC_VER */

#include "tbb_machine.h"

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    // Workaround for overzealous compiler warnings 
    #pragma warning (push)
    #pragma warning (disable: 4244 4267)
#endif

namespace tbb {

//! Specifies memory fencing.
enum memory_semantics {
    //! For internal use only.
    __TBB_full_fence,
    //! Acquire fence
    acquire,
    //! Release fence
    release
};

//! @cond INTERNAL
namespace internal {

#if __GNUC__ || __SUNPRO_CC
#define __TBB_DECL_ATOMIC_FIELD(t,f,a) t f  __attribute__ ((aligned(a)));
#elif defined(__INTEL_COMPILER)||_MSC_VER >= 1300
#define __TBB_DECL_ATOMIC_FIELD(t,f,a) __declspec(align(a)) t f;
#else 
#error Do not know syntax for forcing alignment.
#endif /* __GNUC__ */

template<size_t S>
struct atomic_rep;           // Primary template declared, but never defined.

template<>
struct atomic_rep<1> {       // Specialization
    typedef int8_t word;
    int8_t value;
};
template<>
struct atomic_rep<2> {       // Specialization
    typedef int16_t word;
    __TBB_DECL_ATOMIC_FIELD(int16_t,value,2)
};
template<>
struct atomic_rep<4> {       // Specialization
#if _MSC_VER && __TBB_WORDSIZE==4
    // Work-around that avoids spurious /Wp64 warnings
    typedef intptr_t word;
#else
    typedef int32_t word;
#endif
    __TBB_DECL_ATOMIC_FIELD(int32_t,value,4)
};
template<>
struct atomic_rep<8> {       // Specialization
    typedef int64_t word;
    __TBB_DECL_ATOMIC_FIELD(int64_t,value,8)
};

template<size_t Size, memory_semantics M>
struct atomic_traits;        // Primary template declared, but not defined.

#define __TBB_DECL_FENCED_ATOMIC_PRIMITIVES(S,M)                         \
    template<> struct atomic_traits<S,M> {                               \
        typedef atomic_rep<S>::word word;                               \
        inline static word compare_and_swap( volatile void* location, word new_value, word comparand ) {\
            return __TBB_CompareAndSwap##S##M(location,new_value,comparand);    \
        }                                                                       \
        inline static word fetch_and_add( volatile void* location, word addend ) { \
            return __TBB_FetchAndAdd##S##M(location,addend);                    \
        }                                                                       \
        inline static word fetch_and_store( volatile void* location, word value ) {\
            return __TBB_FetchAndStore##S##M(location,value);                   \
        }                                                                       \
    };

#define __TBB_DECL_ATOMIC_PRIMITIVES(S)                                  \
    template<memory_semantics M>                                         \
    struct atomic_traits<S,M> {                                          \
        typedef atomic_rep<S>::word word;                               \
        inline static word compare_and_swap( volatile void* location, word new_value, word comparand ) {\
            return __TBB_CompareAndSwap##S(location,new_value,comparand);       \
        }                                                                       \
        inline static word fetch_and_add( volatile void* location, word addend ) { \
            return __TBB_FetchAndAdd##S(location,addend);                       \
        }                                                                       \
        inline static word fetch_and_store( volatile void* location, word value ) {\
            return __TBB_FetchAndStore##S(location,value);                      \
        }                                                                       \
    };

#if __TBB_DECL_FENCED_ATOMICS
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(1,__TBB_full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(2,__TBB_full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(4,__TBB_full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(8,__TBB_full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(1,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(2,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(4,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(8,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(1,release)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(2,release)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(4,release)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(8,release)
#else
__TBB_DECL_ATOMIC_PRIMITIVES(1)
__TBB_DECL_ATOMIC_PRIMITIVES(2)
__TBB_DECL_ATOMIC_PRIMITIVES(4)
__TBB_DECL_ATOMIC_PRIMITIVES(8)
#endif

//! Additive inverse of 1 for type T.
/** Various compilers issue various warnings if -1 is used with various integer types.
    The baroque expression below avoids all the warnings (we hope). */
#define __TBB_MINUS_ONE(T) (T(T(0)-T(1)))

//! Base class that provides basic functionality for atomic<T> without fetch_and_add.
/** Works for any type T that has the same size as an integral type, has a trivial constructor/destructor, 
    and can be copied/compared by memcpy/memcmp. */
template<typename T>
struct atomic_impl {
protected:
    atomic_rep<sizeof(T)> rep;
private:
    //! Union type used to convert type T to underlying integral type.
    union converter {
        T value;
        typename atomic_rep<sizeof(T)>::word bits;
    };
public:
    typedef T value_type;

    template<memory_semantics M>
    value_type fetch_and_store( value_type value ) {
        converter u, w;
        u.value = value;
        w.bits = internal::atomic_traits<sizeof(value_type),M>::fetch_and_store(&rep.value,u.bits);
        return w.value;
    }

    value_type fetch_and_store( value_type value ) {
        return fetch_and_store<__TBB_full_fence>(value);
    }

    template<memory_semantics M>
    value_type compare_and_swap( value_type value, value_type comparand ) {
        converter u, v, w;
        u.value = value;
        v.value = comparand;
        w.bits = internal::atomic_traits<sizeof(value_type),M>::compare_and_swap(&rep.value,u.bits,v.bits);
        return w.value;
    }

    value_type compare_and_swap( value_type value, value_type comparand ) {
        return compare_and_swap<__TBB_full_fence>(value,comparand);
    }

    operator value_type() const volatile {                // volatile qualifier here for backwards compatibility 
        converter w;
        w.bits = __TBB_load_with_acquire( rep.value );
        return w.value;
    }

protected:
    value_type store_with_release( value_type rhs ) {
        converter u;
        u.value = rhs;
        __TBB_store_with_release(rep.value,u.bits);
        return rhs;
    }
};

//! Base class that provides basic functionality for atomic<T> with fetch_and_add.
/** I is the underlying type.
    D is the difference type.
    StepType should be char if I is an integral type, and T if I is a T*. */
template<typename I, typename D, typename StepType>
struct atomic_impl_with_arithmetic: atomic_impl<I> {
public:
    typedef I value_type;

    template<memory_semantics M>
    value_type fetch_and_add( D addend ) {
        return value_type(internal::atomic_traits<sizeof(value_type),M>::fetch_and_add( &this->rep.value, addend*sizeof(StepType) ));
    }

    value_type fetch_and_add( D addend ) {
        return fetch_and_add<__TBB_full_fence>(addend);
    }

    template<memory_semantics M>
    value_type fetch_and_increment() {
        return fetch_and_add<M>(1);
    }

    value_type fetch_and_increment() {
        return fetch_and_add(1);
    }

    template<memory_semantics M>
    value_type fetch_and_decrement() {
        return fetch_and_add<M>(__TBB_MINUS_ONE(D));
    }

    value_type fetch_and_decrement() {
        return fetch_and_add(__TBB_MINUS_ONE(D));
    }

public:
    value_type operator+=( D addend ) {
        return fetch_and_add(addend)+addend;
    }

    value_type operator-=( D addend ) {
        // Additive inverse of addend computed using binary minus,
        // instead of unary minus, for sake of avoiding compiler warnings.
        return operator+=(D(0)-addend);    
    }

    value_type operator++() {
        return fetch_and_add(1)+1;
    }

    value_type operator--() {
        return fetch_and_add(__TBB_MINUS_ONE(D))-1;
    }

    value_type operator++(int) {
        return fetch_and_add(1);
    }

    value_type operator--(int) {
        return fetch_and_add(__TBB_MINUS_ONE(D));
    }
};

#if __TBB_WORDSIZE == 4
// Plaforms with 32-bit hardware require special effort for 64-bit loads and stores.
#if defined(__INTEL_COMPILER)||!defined(_MSC_VER)||_MSC_VER>=1400

template<>
inline atomic_impl<__TBB_LONG_LONG>::operator atomic_impl<__TBB_LONG_LONG>::value_type() const volatile {
    return __TBB_Load8(&rep.value);
}

template<>
inline atomic_impl<unsigned __TBB_LONG_LONG>::operator atomic_impl<unsigned __TBB_LONG_LONG>::value_type() const volatile {
    return __TBB_Load8(&rep.value);
}

template<>
inline atomic_impl<__TBB_LONG_LONG>::value_type atomic_impl<__TBB_LONG_LONG>::store_with_release( value_type rhs ) {
    __TBB_Store8(&rep.value,rhs);
    return rhs;
}

template<>
inline atomic_impl<unsigned __TBB_LONG_LONG>::value_type atomic_impl<unsigned __TBB_LONG_LONG>::store_with_release( value_type rhs ) {
    __TBB_Store8(&rep.value,rhs);
    return rhs;
}

#endif /* defined(__INTEL_COMPILER)||!defined(_MSC_VER)||_MSC_VER>=1400 */
#endif /* __TBB_WORDSIZE==4 */

} /* Internal */
//! @endcond

//! Primary template for atomic.
/** See the Reference for details.
    @ingroup synchronization */
template<typename T>
struct atomic: internal::atomic_impl<T> {
    T operator=( T rhs ) {
        // "this" required here in strict ISO C++ because store_with_release is a dependent name
        return this->store_with_release(rhs);
    }
    atomic<T>& operator=( const atomic<T>& rhs ) {this->store_with_release(rhs); return *this;}
};

#define __TBB_DECL_ATOMIC(T) \
    template<> struct atomic<T>: internal::atomic_impl_with_arithmetic<T,T,char> {  \
        T operator=( T rhs ) {return store_with_release(rhs);}  \
        atomic<T>& operator=( const atomic<T>& rhs ) {store_with_release(rhs); return *this;}  \
    };

#if defined(__INTEL_COMPILER)||!defined(_MSC_VER)||_MSC_VER>=1400
__TBB_DECL_ATOMIC(__TBB_LONG_LONG)
__TBB_DECL_ATOMIC(unsigned __TBB_LONG_LONG)
#else
// Some old versions of MVSC cannot correctly compile templates with "long long".
#endif /* defined(__INTEL_COMPILER)||!defined(_MSC_VER)||_MSC_VER>=1400 */

__TBB_DECL_ATOMIC(long)
__TBB_DECL_ATOMIC(unsigned long)

#if defined(_MSC_VER) && __TBB_WORDSIZE==4
/* Special version of __TBB_DECL_ATOMIC that avoids gratuitous warnings from cl /Wp64 option. 
   It is identical to __TBB_DECL_ATOMIC(unsigned) except that it replaces operator=(T) 
   with an operator=(U) that explicitly converts the U to a T.  Types T and U should be
   type synonyms on the platform.  Type U should be the wider variant of T from the
   perspective of /Wp64. */
#define __TBB_DECL_ATOMIC_ALT(T,U) \
    template<> struct atomic<T>: internal::atomic_impl_with_arithmetic<T,T,char> {  \
        T operator=( U rhs ) {return store_with_release(T(rhs));}  \
        atomic<T>& operator=( const atomic<T>& rhs ) {store_with_release(rhs); return *this;}  \
    };
__TBB_DECL_ATOMIC_ALT(unsigned,size_t)
__TBB_DECL_ATOMIC_ALT(int,ptrdiff_t)
#else
__TBB_DECL_ATOMIC(unsigned)
__TBB_DECL_ATOMIC(int)
#endif /* defined(_MSC_VER) && __TBB_WORDSIZE==4 */

__TBB_DECL_ATOMIC(unsigned short)
__TBB_DECL_ATOMIC(short)
__TBB_DECL_ATOMIC(char)
__TBB_DECL_ATOMIC(signed char)
__TBB_DECL_ATOMIC(unsigned char)

#if !defined(_MSC_VER)||defined(_NATIVE_WCHAR_T_DEFINED) 
__TBB_DECL_ATOMIC(wchar_t)
#endif /* _MSC_VER||!defined(_NATIVE_WCHAR_T_DEFINED) */

//! Specialization for atomic<T*> with arithmetic and operator->.
template<typename T> struct atomic<T*>: internal::atomic_impl_with_arithmetic<T*,ptrdiff_t,T> {
    T* operator=( T* rhs ) {
        // "this" required here in strict ISO C++ because store_with_release is a dependent name
        return this->store_with_release(rhs);
    }
    atomic<T*>& operator=( const atomic<T*>& rhs ) {
        this->store_with_release(rhs); return *this;
    }
    T* operator->() const {
        return (*this);
    }
};

//! Specialization for atomic<void*>, for sake of not allowing arithmetic or operator->.
template<> struct atomic<void*>: internal::atomic_impl<void*> {
    void* operator=( void* rhs ) {
        // "this" required here in strict ISO C++ because store_with_release is a dependent name
        return this->store_with_release(rhs);
    }
    atomic<void*>& operator=( const atomic<void*>& rhs ) {
        this->store_with_release(rhs); return *this;
    }
};

} // namespace tbb

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #pragma warning (pop)
#endif // warnings 4244, 4267 are back

#endif /* __TBB_atomic_H */
