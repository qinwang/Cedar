/*
 * This file is a part of TTMath Bignum Library
 * and is distributed under the (new) BSD licence.
 * Author: Tomasz Sowa <t.sowa@ttmath.org>
 */

/*
 * Copyright (c) 2006-2009, Tomasz Sowa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  * Neither the name Tomasz Sowa nor the names of contributors to this
 *    project may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef headerfilettmathtypes
#define headerfilettmathtypes

/*!
 \file ttmathtypes.h
 \brief constants used in the library

 As our library is written in header files (templates) we cannot use
 constants like 'const int' etc. because we should have some source files
 *.cpp to define this variables. Only what we can have are constants
 defined by #define preprocessor macros.

 All macros are preceded by TTMATH_ prefix
 */

#include <stdexcept>
#include <sstream>
#include <vector>
#include <stdint.h>

/*!
 the version of the library

 TTMATH_PRERELEASE_VER is either zero or one
 if zero that means this is the release version of the library
 */
#define TTMATH_MAJOR_VER        0
#define TTMATH_MINOR_VER        9
#define TTMATH_REVISION_VER     0
#define TTMATH_PRERELEASE_VER   0

/*!
 TTMATH_DEBUG
 this macro enables further testing during writing your code
 you don't have to define it in a release mode

 if this macro is set then macros TTMATH_ASSERT and TTMATH_REFERENCE_ASSERT
 are set as well and these macros can throw an exception if a condition in it
 is not fulfilled (look at the definition of TTMATH_ASSERT and TTMATH_REFERENCE_ASSERT)

 TTMATH_RELEASE
 if you are confident that your code is perfect you can define TTMATH_RELEASE
 macro for example by using -D option in gcc
 gcc -DTTMATH_RELEASE -o myprogram myprogram.cpp
 or by defining this macro in your code before using any header files of this library

 if TTMATH_RELEASE is not set then TTMATH_DEBUG is set automatically
 */
#ifndef TTMATH_RELEASE
#define TTMATH_DEBUG
#endif

namespace ttmath {

#if !defined _M_X64 && !defined __x86_64__

/*!
 we're using a 32bit platform
 */
#define TTMATH_PLATFORM32

#else

/*!
 we're using a 64bit platform
 */
#define TTMATH_PLATFORM64

#endif

/*!
 another compilers than MS VC or GCC by default use no asm version (TTMATH_NOASM)
 */
#if !defined _MSC_VER && !defined __GNUC__
#define TTMATH_NOASM
#endif

#ifdef TTMATH_PLATFORM32

/*!
 on 32bit platforms one word (uint, sint) will be equal 32bits
 */
typedef unsigned int uint;
typedef signed int sint;

/*!
 this type is twice bigger than uint
 (64bit on a 32bit platforms)

 although C++ Standard - ANSI ISO IEC 14882:2003 doesn't define such a type (long long)
 but it is defined in C99 and in upcoming C++0x /3.9.1 (2)/ and many compilers support it

 this type is used in UInt::MulTwoWords and UInt::DivTwoWords when macro TTMATH_NOASM is defined
 but only on a 32bit platform
 */
#ifdef TTMATH_NOASM
typedef unsigned long long int ulint;
#endif

/*!
 how many bits there are in the uint type
 */
#define TTMATH_BITS_PER_UINT 32u

/*!
 the mask for the highest bit in the unsigned 32bit word (2^31)
 */
#define TTMATH_UINT_HIGHEST_BIT 2147483648u

/*!
 the max value of the unsigned 32bit word (2^32 - 1)
 (all bits equal one)
 */
#define TTMATH_UINT_MAX_VALUE 4294967295u

/*!
 the number of words (32bit words on 32bit platform)
 which are kept in built-in variables for a Big<> type
 (these variables are defined in ttmathbig.h)
 */
#define TTMATH_BUILTIN_VARIABLES_SIZE 256u

/*!
 this macro returns the number of machine words
 capable to hold min_bits bits
 e.g. TTMATH_BITS(128) returns 4
 */
#define TTMATH_BITS(min_bits) ((min_bits-1)/32 + 1)

#else

/*!
 on 64bit platforms one word (uint, sint) will be equal 64bits
 */
#ifdef _MSC_VER
/* in VC 'long' type has 32 bits, __int64 is VC extension */
typedef unsigned __int64 uint;
typedef signed __int64 sint;
#else
typedef uint64_t uint;
typedef int64_t sint;
#endif

/*!
 on 64bit platform we do not define ulint
 sizeof(long long) is 8 (64bit) but we need 128bit

 on 64 bit platform (when there is defined TTMATH_NOASM macro)
 methods UInt::MulTwoWords and UInt::DivTwoWords are using other algorithms than those on 32 bit
 */
//typedef unsigned long long int ulint;

/*!
 how many bits there are in the uint type
 */
#define TTMATH_BITS_PER_UINT 64ul

/*!
 the mask for the highest bit in the unsigned 64bit word (2^63)
 */
#define TTMATH_UINT_HIGHEST_BIT 9223372036854775808ul

/*!
 the max value of the unsigned 64bit word (2^64 - 1)
 (all bits equal one)
 */
#define TTMATH_UINT_MAX_VALUE 18446744073709551615ul

/*!
 the number of words (64bit words on 64bit platforms)
 which are kept in built-in variables for a Big<> type
 (these variables are defined in ttmathbig.h)
 */
#define TTMATH_BUILTIN_VARIABLES_SIZE 128ul

/*!
 this macro returns the number of machine words
 capable to hold min_bits bits
 e.g. TTMATH_BITS(128) returns 2
 */
#define TTMATH_BITS(min_bits) ((min_bits-1)/64 + 1)

#endif
}

#if defined(TTMATH_MULTITHREADS) && !defined(TTMATH_MULTITHREADS_NOSYNC)
#if !defined(TTMATH_POSIX_THREADS) && !defined(TTMATH_WIN32_THREADS)

#if defined(_WIN32)
#define TTMATH_WIN32_THREADS
#elif defined(unix) || defined(__unix__) || defined(__unix)
#define TTMATH_POSIX_THREADS
#endif

#endif
#endif

/*!
 this variable defines how many iterations are performed
 during some kind of calculating when we're making any long formulas
 (for example Taylor series)

 it's used in ExpSurrounding0(...), LnSurrounding1(...), Sin0pi05(...), etc.

 note! there'll not be so many iterations, iterations are stopped when
 there is no sense to continue calculating (for example when the result
 still remains unchanged after adding next series and we know that the next
 series are smaller than previous ones)
 */
#define TTMATH_ARITHMETIC_MAX_LOOP 10000

/*!
 this is a limit when calculating Karatsuba multiplication
 if the size of a vector is smaller than TTMATH_USE_KARATSUBA_MULTIPLICATION_FROM_SIZE
 the Karatsuba algorithm will use standard schoolbook multiplication
 */
#ifdef TTMATH_DEBUG_LOG
// if TTMATH_DEBUG_LOG is defined then we should use the same size regardless of the compiler
#define TTMATH_USE_KARATSUBA_MULTIPLICATION_FROM_SIZE 3
#else
#ifdef __GNUC__
#define TTMATH_USE_KARATSUBA_MULTIPLICATION_FROM_SIZE 3
#else
#define TTMATH_USE_KARATSUBA_MULTIPLICATION_FROM_SIZE 5
#endif
#endif

/*!
 this is a special value used when calculating the Gamma(x) function
 if x is greater than this value then the Gamma(x) will be calculated using
 some kind of series

 don't use smaller values than about 100
 */
#define TTMATH_GAMMA_BOUNDARY 2000

namespace ttmath {

/*!
 lib type codes:
 asm_vc_32   - with asm code designed for Microsoft Visual C++ (32 bits)
 asm_gcc_32  - with asm code designed for GCC (32 bits)
 asm_vc_64   - with asm for VC (64 bit)
 asm_gcc_64  - with asm for GCC (64 bit)
 no_asm_32   - pure C++ version (32 bit) - without any asm code
 no_asm_64   - pure C++ version (64 bit) - without any asm code
 */
enum LibTypeCode {
    asm_vc_32 = 0, asm_gcc_32, asm_vc_64, asm_gcc_64, no_asm_32, no_asm_64
};

/*!
 error codes
 */
enum ErrorCode {
    err_ok = 0,
    err_nothing_has_read,
    err_unknown_character,
    err_unexpected_final_bracket,
    err_stack_not_clear,
    err_unknown_variable,
    err_division_by_zero,
    err_interrupt,
    err_overflow,
    err_unknown_function,
    err_unknown_operator,
    err_unexpected_semicolon_operator,
    err_improper_amount_of_arguments,
    err_improper_argument,
    err_unexpected_end,
    err_internal_error,
    err_incorrect_name,
    err_incorrect_value,
    err_variable_exists,
    err_variable_loop,
    err_functions_loop,
    err_must_be_only_one_value,
    err_object_exists,
    err_unknown_object,
    err_still_calculating,
    err_in_short_form_used_function,
    err_percent_from
};

/*!
 this struct is used when converting to/from a string
 /temporarily only in Big::ToString() and Big::FromString()/
 */
struct Conv {
    /*!
     base (radix) on which the value will be shown (or read)
     default: 10
     */
    uint base;

    /*!
     used only in Big::ToString()
     if true the value will be always shown in the scientific mode, e.g: 123e+30
     default: false
     */
    bool scient;

    /*!
     used only in Big::ToString()
     if scient is false then the value will be print in the scientific mode
     only if the exponent is greater than scien_from
     default: 15
     */
    sint scient_from;

    /*!
     if 'base_round' is true and 'base' is different from 2, 4, 8, or 16
     and the result value is not an integer then we make an additional rounding
     (after converting the last digit from the result is skipped)
     default: true

     e.g.
     Conv c;
     c.base_round = false;
     Big<1, 1> a = "0.1";                       // decimal input
     std::cout << a.ToString(c) << std::endl;   // the result is: 0.099999999
     */
    bool base_round;

    /*!
     used only in Big::ToString()
     tells how many digits after comma are possible
     default: -1 which means all digits are printed

     set it to zero if you want integer value only

     for example when the value is:
     12.345678 and 'round' is 4
     then the result will be
     12.3457   (the last digit was rounded)
     */
    sint round;

    /*!
     if true that not mattered digits in the mantissa will be cut off
     (zero characters at the end -- after the comma operator)
     e.g. 1234,78000 will be: 1234,78
     default: true
     */
    bool trim_zeroes;

    /*!
     the main comma operator (used when reading and writing)
     default is a dot '.'
     */
    uint comma;

    /*!
     additional comma operator (used only when reading)
     if you don't want it just set it to zero
     default is a comma ','

     this allowes you to convert from a value:
     123.45 as well as from 123,45
     */
    uint comma2;

    /*!
     it sets the character which is used for grouping
     if group=' ' then: 1234,56789 will be printed as: 1 234,567 89

     if you don't want grouping just set it to zero (which is default)
     */
    uint group;

    /*!
     */
    uint group_exp; // not implemented yet


    Conv() {
        // default values
        base = 10;
        scient = false;
        scient_from = 15;
        base_round = true;
        round = -1;
        trim_zeroes = true;
        comma = '.';
        comma2 = ',';
        group = 0;
        group_exp = 0;
    }
};

/*!
 this simple class can be used in multithreading model
 (you can write your own class derived from this one)

 for example: in some functions like Factorial()
 /at the moment only Factorial/ you can give a pointer to
 the 'stop object', if the method WasStopSignal() of this
 object returns true that means we should break the calculating
 and return
 */
class StopCalculating {
public:
    virtual bool WasStopSignal() const volatile {
        return false;
    }
    virtual ~StopCalculating() {
    }
};

/*!
 a small class which is useful when compiling with gcc

 object of this type holds the name and the line of a file
 in which the macro TTMATH_ASSERT or TTMATH_REFERENCE_ASSERT was used
 */
class ExceptionInfo {
    const char * file;
    int line;

public:
    ExceptionInfo() :
        file(0), line(0) {
    }
    ExceptionInfo(const char * f, int l) :
        file(f), line(l) {
    }

    std::string Where() const {
        if (!file)
            return "unknown";

        std::ostringstream result;
        result << file << ":" << line;

        return result.str();
    }
};

/*!
 A small class used for reporting 'reference' errors

 In the library is used macro TTMATH_REFERENCE_ASSERT which
 can throw an exception of this type

 If you compile with gcc you can get a small benefit
 from using method Where() (it returns std::string) with
 the name and the line of a file where the macro TTMATH_REFERENCE_ASSERT
 was used)

 What is the 'reference' error?
 Some kind of methods use a reference as their argument to another object,
 and the another object not always can be the same which is calling, e.g.
 Big<1,2> foo(10);
 foo.Mul(foo); // this is incorrect
 above method Mul is making something more with 'this' object and
 'this' cannot be passed as the argument because the result will be undefined

 macro TTMATH_REFERENCE_ASSERT helps us to solve the above problem

 note! some methods can use 'this' object as the argument
 for example this code is correct:
 UInt<2> foo(10);
 foo.Add(foo);
 but there are only few methods which can do that
 */
class ReferenceError: public std::logic_error, public ExceptionInfo {
public:

    ReferenceError() :
        std::logic_error("reference error") {
    }

    ReferenceError(const char * f, int l) :
        std::logic_error("reference error"), ExceptionInfo(f, l) {
    }

    std::string Where() const {
        return ExceptionInfo::Where();
    }
};

/*!
 a small class used for reporting errors

 in the library is used macro TTMATH_ASSERT which
 (if the condition in it is false) throw an exception
 of this type

 if you compile with gcc you can get a small benefit
 from using method Where() (it returns std::string) with
 the name and the line of a file where the macro TTMATH_ASSERT
 was used)
 */
class RuntimeError: public std::runtime_error, public ExceptionInfo {
public:

    RuntimeError() :
        std::runtime_error("internal error") {
    }

    RuntimeError(const char * f, int l) :
        std::runtime_error("internal error"), ExceptionInfo(f, l) {
    }

    std::string Where() const {
        return ExceptionInfo::Where();
    }
};

/*!
 look at the description of macros TTMATH_RELEASE and TTMATH_DEBUG
 */
#ifdef TTMATH_DEBUG

#if defined(__FILE__) && defined(__LINE__)

#define TTMATH_REFERENCE_ASSERT(expression) \
                if( &(expression) == this ) throw ttmath::ReferenceError(__FILE__, __LINE__);

#define TTMATH_ASSERT(expression) \
                if( !(expression) ) throw ttmath::RuntimeError(__FILE__, __LINE__);

#else

#define TTMATH_REFERENCE_ASSERT(expression) \
                  if( &(expression) == this ) throw ReferenceError();

#define TTMATH_ASSERT(expression) \
                  if( !(expression) ) throw RuntimeError();
#endif

#else
#define TTMATH_REFERENCE_ASSERT(expression)
#define TTMATH_ASSERT(expression)
#endif

#ifdef TTMATH_DEBUG_LOG
#define TTMATH_LOG(msg)                             PrintLog(msg, std::cout);
#define TTMATH_LOGC(msg, carry)                     PrintLog(msg, carry, std::cout);
#define TTMATH_VECTOR_LOG(msg, vector, len)         PrintVectorLog(msg, std::cout, vector, len);
#define TTMATH_VECTOR_LOGC(msg, carry, vector, len) PrintVectorLog(msg, carry, std::cout, vector, len);
#else
#define TTMATH_LOG(msg)
#define TTMATH_LOGC(msg, carry)
#define TTMATH_VECTOR_LOG(msg, vector, len)
#define TTMATH_VECTOR_LOGC(msg, carry, vector, len)
#endif

} // namespace


#endif
