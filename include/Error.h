/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_ERROR_H_
#define SRC_ERROR_H_

#include <string>

namespace Mist {

class Error {
public:
    enum class ErrorCode
        : int {
            AccessDenied = 1,
        NotFound = 2,
        InvalidTransaction = 3,
        InvalidAccessDomain = 4,
        InvalidParent = 5,
        UnexpectedDatabaseError = 6,
        LoopDetected = 7,
        NotEmpty = 8,
        ObjectCollisionInTransaction = 9,
        AlreadyInUse = 10,
        ParentTransactionNotFound = 11
    };

    Error( ErrorCode errorCode ) :
            errorCode( errorCode ), errorMessage( "" ) {
    }
    Error( ErrorCode errorCode, const std::string& errorMessage ) :
            errorCode( errorCode ), errorMessage( errorMessage ) {
    }
    virtual ~Error() {
    }

    Mist::Error::ErrorCode getErrorCode() {
        return errorCode;
    }

    const char* getErrorMessage() {
        return errorMessage.c_str();
    }

private:
    const Mist::Error::ErrorCode errorCode;
    const std::string errorMessage;
};

} /* namespace Mist */

#endif /* SRC_ERROR_H_ */
