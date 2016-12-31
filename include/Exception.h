/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_EXCEPTION_H_
#define SRC_EXCEPTION_H_

#include <string>
#include <stdexcept>

#include "Error.h"

// Compatibility with non-clang compilers.
#ifndef __has_feature
#define __has_feature(x) 0
#endif

// Detect whether the compiler supports C++11 noexcept exception specifications.
#if (  defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ > 4)) \
    && defined(__GXX_EXPERIMENTAL_CXX0X__))
// GCC 4.7 and following have noexcept
#elif defined(__clang__) && __has_feature(cxx_noexcept)
// Clang 3.0 and above have noexcept
#elif defined(_MSC_VER) && _MSC_VER > 1800
// Visual Studio 2015 and above have noexcept
#else
// Visual Studio 2013 does not support noexcept, and "throw()" is deprecated by C++11
#define noexcept
#endif

namespace Mist {

class Exception: public std::runtime_error {
public:
    explicit Exception( char const* errorMessage );
    explicit Exception( const std::string& errorMessage );
    Exception( const std::string& errorMessage, const Mist::Error::ErrorCode& mistErrorCode );
    Exception( const Mist::Error::ErrorCode& mistErrorCode );
    virtual ~Exception();
    /*
    virtual const char* what() const throw () override {
        return errorMessage.c_str();
    }
     //*/

    inline int getErrorCode() const noexcept {
        return errorCode;
    }

    const char* getErrorMessage() {
        return errorMessage.c_str();
    }

private:
    const int errorCode;
    const std::string errorMessage;
};

} /* namespace Mist */

#endif /* SRC_EXCEPTION_H_ */
