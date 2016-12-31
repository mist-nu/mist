/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_HELPER_H_
#define SRC_HELPER_H_

#include <array>
#include <cassert>
#include <ctime>
#include <functional>
#include <streambuf>
#include <string>

#include <boost/filesystem.hpp>
#include <g3log/g3log.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <sqlite3.h>

namespace Mist {
namespace Helper {

namespace Database {
using Column = SQLite::Column;
const int INTEGER = SQLITE_INTEGER;
const int FLOAT = SQLITE_FLOAT;
const int TEXT = SQLITE_TEXT;
const int BLOB = SQLITE_BLOB;
const int Null = SQLITE_NULL;

using Database = SQLite::Database;
const int OPEN_READONLY = SQLITE_OPEN_READONLY;
const int OPEN_READWRITE = SQLITE_OPEN_READWRITE;
const int OPEN_CREATE = SQLITE_OPEN_CREATE;
const int OPEN_URI = SQLITE_OPEN_URI;
const int OK = SQLITE_OK;

using Exception = SQLite::Exception;
using Statement = SQLite::Statement;
using Transaction = SQLite::Transaction;

class SavePoint {
protected:
    std::string name;
    bool released;
    Database* connection;
public:
    SavePoint( Database* connection, const std::string& name );
    virtual ~SavePoint();
    void save();
    void rollback();
};

} /* namespace Database */

namespace filesystem = ::boost::filesystem;
class TemporaryFile {};


class Date {
    // TODO: fix the whole class
public:
    static std::string now( int diffSeconds = 0 );
    Date() : date( { } ) {}
    Date( const char* date ) : date( date ) {}
    Date( const std::string& date ) : date( date ) {}
    ~Date() {}
    const std::string& toString() const { return this->date; }
private:
    std::string date;
};

} /* namespace Helper */

template<std::size_t SIZE = 512, class CharT = char>
class StreamToString : public std::basic_streambuf<CharT> {
public:
    using Base = std::basic_streambuf<CharT>;
    using char_type = typename Base::char_type;
    using int_type = typename Base::int_type;

    explicit StreamToString( std::function<void(std::basic_string<CharT>)> cb ) :
            buffer{}, cb( cb ) {
                Base::setp( &( *buffer.begin() ), &( *buffer.end() ) - 1 );
            }
    virtual ~StreamToString() = default;

protected:
    bool flush() {
        std::ptrdiff_t n{ this->pptr() - this->pbase() };
        if ( n > 0 ) {
            cb( std::basic_string<CharT>( static_cast<CharT*>( this->pbase() ), n ) );
            LOG( DBUG ) << "Streamer: " << std::basic_string<CharT>( static_cast<CharT*>( this->pbase() ), n );
            this->pbump( -n );
        }
        return true; // Assume true since cb returns void
    }

    virtual int_type sync() override {
        return flush() ? 0 : std::char_traits<CharT>::eof();
    }

    virtual int_type overflow( int_type ch ) override {
        if ( cb && ch != std::char_traits<CharT>::eof() ) {
            assert( std::less_equal<CharT*>()(this->pptr(),this->epptr() ) );
            *this->pptr() = ch;
            this->pbump( 1 );
            if( flush() ) {
                return ch;
            }
        }

        return std::char_traits<CharT>::eof();
    }

private:
    StreamToString( const StreamToString& ) = delete;
    StreamToString( StreamToString&& ) = delete;
    StreamToString& operator=( const StreamToString& ) = delete;
    StreamToString& operator=( StreamToString&& ) = delete;

    std::array<char_type, SIZE> buffer;
    std::function<void(std::basic_string<CharT>)> cb;
};

} /* namespace Mist */

#endif /* SRC_HELPER_H_ */
