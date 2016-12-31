/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <algorithm> // count
#include <limits> // int/float -> min/max

#include <gtest/gtest.h> // Google test framework

#include "Helper.h"
#include "JSONstream.h"

namespace J = JSON;

namespace {


class openD : public J::Deserialize {
public:
    virtual ~openD() = default;

    std::stack<Lexer_state>* s_stack() { return &( this->state ); }
    Lexer_state s() { return ( this->state.empty() ? Lexer_state::Error : this->state.top() ); }
    Lexer_number_state n() { return this->number_state; }
    char k() { return this->keyword_state; }
    Internal_type it() { return this->current_type; }
    char u() { return this->unicode_buffer; }

    char* get_input_buffer() { return this->input_buffer.data(); }
    unsigned int get_inbuf_len() { return this->input_buffer_length; }
};

using D = openD;
using T = J::Json_type;
using LS = D::Lexer_state;
using LN = D::Lexer_number_state;
using IT = D::Internal_type;

class DeserializeTest:  public ::testing::Test {
public:
    DeserializeTest() : d{}, ss( d.s_stack() ) {}
    virtual ~DeserializeTest() = default;

    D d;
    std::stack<LS>* ss;
};

TEST_F( DeserializeTest, Empty ) {
    EXPECT_EQ( T::Error, d.get_type() );
    EXPECT_TRUE( ss->empty() );
    EXPECT_EQ( LS::Error, d.s() );

    EXPECT_FALSE( d.is_complete() );
    EXPECT_FALSE( d.is_null() );
    EXPECT_FALSE( d.is_string() );
    EXPECT_FALSE( d.is_array() );
    EXPECT_FALSE( d.is_object() );
    EXPECT_FALSE( d.is_bool() );
    EXPECT_FALSE( d.is_true() );
    EXPECT_FALSE( d.is_false() );
    EXPECT_FALSE( d.is_number() );
    EXPECT_FALSE( d.is_float() );
    EXPECT_FALSE( d.is_integer() );
}

TEST_F( DeserializeTest, Start_testing_without_streams ) {
    SUCCEED() << "Test without streams first";
}

TEST_F( DeserializeTest, Control_chars_ignored ) {
    for( char c = 0; c <= 0x1f; ++c ) {
        EXPECT_TRUE( d.put( c ) );
    }

    ASSERT_FALSE( ss->empty() );
    EXPECT_EQ( LS::Value, d.s() );
    EXPECT_EQ( T::Error, d.get_type() );
}

TEST_F( DeserializeTest, Null ) {
    const char Null[] = "null";
    EXPECT_EQ( 4, d.writesome( Null, 4 ) );
    EXPECT_EQ( LS::Complete, d.s() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_EQ( T::Null, d.get_type() );
}

TEST_F( DeserializeTest, True ) {
    const char True[] = "true";
    EXPECT_EQ( 4, d.writesome( True, 4 ) );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_EQ( T::True, d.get_type() );
}

TEST_F( DeserializeTest, False ) {
    const char False[] = "false";
    EXPECT_EQ( 5, d.writesome( False, 5 ) );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_EQ( T::False, d.get_type() );
}

TEST_F( DeserializeTest, Integer_leading_zero ) {
    EXPECT_TRUE( d.put( '0' ) );
    EXPECT_FALSE( d.put( '1' ) );
    EXPECT_EQ( LN::Error, d.n() );
    EXPECT_EQ( LS::Error, d.s() );
    EXPECT_FALSE( d.is_complete() );
}

TEST_F( DeserializeTest, Number_buffer ) {
    constexpr char start = '1';
    constexpr unsigned int length = 500;

    auto mod_c = [] ( char c ) { return (char) ( ( c - 0x30 ) % 10 ) + 0x30; };

    char c = start;
    for ( unsigned int i = 0; i < length; ++i) {
        c = mod_c( c++ );
        EXPECT_TRUE( d.put( c ) );
        EXPECT_EQ( IT::Integer, d.it() );
        EXPECT_EQ( LN::Digit, d.n() );
        EXPECT_FALSE( d.is_complete() );
    }
    EXPECT_FALSE( d.put( 0 ) );
    EXPECT_EQ( IT::Integer, d.it() );
    EXPECT_EQ( LN::Digit, d.n() );
    EXPECT_TRUE( d.is_complete() );

    ASSERT_EQ( length, d.get_inbuf_len() );
    c = start;
    for ( unsigned int i = 0; i < d.get_inbuf_len(); ++i ) {
        c = mod_c( c++ );
        EXPECT_EQ( c, d.get_input_buffer()[i] );
    }

    d.pop();
    const char str[] = "1234\0";
    EXPECT_EQ( 4, d.writesome( str, 5 ) );
    EXPECT_EQ( "1234", std::string( d.get_input_buffer(), 4 ) );
}

TEST_F( DeserializeTest, convert_to_int ) {
    const char one = '1';
    EXPECT_EQ( 1, std::stoll( std::string( &one, 1 ) ) );

    const char cstr_1234[] = "1234";
    EXPECT_EQ( 1234, std::stoll( std::string( cstr_1234, 4 ) ) );
}

TEST_F( DeserializeTest, Integer_zero ) {
    EXPECT_TRUE( d.put( '0' ) );
    EXPECT_EQ( LS::Number, d.s() );
    EXPECT_EQ( LN::Zero, d.n() );
    EXPECT_FALSE( d.is_complete() );

    EXPECT_FALSE( d.put( '\n' ) );
    EXPECT_EQ( LS::Complete, d.s() );
    EXPECT_EQ( LN::Zero, d.n() );
    EXPECT_EQ( IT::Integer, d.it() );
    EXPECT_TRUE( d.is_complete() );

    ASSERT_EQ( 1U, d.get_inbuf_len() );
    EXPECT_EQ( '0', d.get_input_buffer()[0] );

    EXPECT_EQ( 0U, d.get_integer() );
}

TEST_F( DeserializeTest, Integer_1234 ) {
    const char cstr_1234[] = "1234\0";
    // Check that it takes the 4 digits
    EXPECT_EQ( 4, d.writesome( cstr_1234, 5 ) );
    // Expect the state stack to contain number->complete
    EXPECT_EQ( 2U, ss->size() );
    EXPECT_EQ( LS::Complete, ss->top() );
    // Expect the ss to get popped when getting the number
    EXPECT_EQ( IT::Integer, d.it() );
    EXPECT_EQ( 1234, d.get_integer() );
    EXPECT_EQ( 0U, ss->size() );
}

TEST_F( DeserializeTest, Integer_negative ) {
    const char neg_one[] = "-1\0";
    EXPECT_EQ( 2, d.writesome( neg_one, 3 ) );
    EXPECT_EQ( IT::Integer, d.it() );
    EXPECT_EQ( -1, d.get_integer() );
}

TEST_F( DeserializeTest, Integer_limits ) {
    const long long max_i64 = std::numeric_limits<long long>::max();
    const std::string max{ std::to_string( max_i64 ) };
    EXPECT_LT( 1U, max.length() );
    EXPECT_EQ( (unsigned) max.length(), d.writesome( max.c_str(), max.length() + 1 ) );
    EXPECT_EQ( max_i64, d.get_integer() );

    const long long min_i64 = std::numeric_limits<long long>::min();
    const std::string min{ std::to_string( min_i64 ) };
    EXPECT_LT( 1U, min.length() );
    EXPECT_EQ( (unsigned) min.length(), d.writesome( min.c_str(), min.length() + 1 ) );
    EXPECT_EQ( min_i64, d.get_integer() );
}

TEST_F( DeserializeTest, Float_zero ) {
    const char zero[] = "0.0\0";
    EXPECT_EQ( 3, d.writesome( zero, 4 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( 0.0, d.get_double() );
}

TEST_F( DeserializeTest, Float_one ) {
    const char one[] = "1.0\0";
    EXPECT_EQ( 3, d.writesome( one, 4 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( 1.0, d.get_double() );
}

TEST_F( DeserializeTest, Float_negative_zero ) {
    const char neg_zero[] = "-0.0\0";
    EXPECT_EQ( 4, d.writesome( neg_zero, 5 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( 0.0, d.get_double() );
}

TEST_F( DeserializeTest, Float_negative_one ) {
    const char neg_one[] = "-1.0\0";
    EXPECT_EQ( 4, d.writesome( neg_one, 5 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( -1.0, d.get_double() );
}

TEST_F( DeserializeTest, Float_exp ) {
    const std::string exp{ "1.0e2" };
    EXPECT_EQ( (unsigned) exp.length(), d.writesome( exp.c_str(), exp.length() + 1 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( 100.0, d.get_double() );
}

TEST_F( DeserializeTest, Float_max ) {
    using D = std::numeric_limits<double>;
    const double f_max{ D::max() };
    std::string s_max{ std::to_string( D::max() ) };
    EXPECT_LT( 3U, s_max.length() );

    EXPECT_EQ( (unsigned) s_max.length(), d.writesome( s_max.c_str(), s_max.length() ) );
    EXPECT_EQ( 0U, d.writesome( "\0", 1 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( f_max, d.get_double() );
}

TEST_F( DeserializeTest, Float_min ) {
    using D = std::numeric_limits<double>;
    const double f_min{ D::min() };
    std::stringstream ss_min;
    ss_min << std::setprecision( D::max_digits10 ) << f_min;
    std::string s_min;
    ss_min >> s_min;
    EXPECT_LT( 3U, s_min.length() );

    EXPECT_EQ( (unsigned) s_min.length(), d.writesome( s_min.c_str(), s_min.length() + 1 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( f_min, d.get_double() );
}

TEST_F( DeserializeTest, Float_lowest ) {
    using D = std::numeric_limits<double>;
    const double f_lowest{ D::lowest() };
    std::stringstream ss_lowest;
    ss_lowest << std::setprecision( D::max_digits10 ) << f_lowest;
    std::string s_lowest;
    ss_lowest >> s_lowest;
    EXPECT_LT( 3U, s_lowest.length() );

    EXPECT_EQ( (unsigned) s_lowest.length(), d.writesome( s_lowest.c_str(), s_lowest.length() + 1 ) );
    EXPECT_EQ( IT::Float, d.it() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_FLOAT_EQ( f_lowest, d.get_double() );
}

TEST_F( DeserializeTest, String_empty ) {
    std::string empty{ "\"\"" };
    EXPECT_EQ( (unsigned) empty.length(), d.writesome( empty.c_str(), empty.length() ) );
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    char buf[3];
    EXPECT_EQ( 0, d.readsome( buf, 3 ) );
}

TEST_F( DeserializeTest, String_space ) {
    std::string space{ R"(" ")" };
    EXPECT_EQ( (unsigned) space.length(), d.writesome( space.c_str(), space.length() ) );
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    char space_buf[2];
    EXPECT_EQ( 1, d.readsome( space_buf, 1) );
    EXPECT_EQ( " ", std::string( space_buf, 1) );
}

TEST_F( DeserializeTest, String_abc ) {
    std::string abc{ R"("abc")" };
    EXPECT_EQ( (unsigned) abc.length(), d.writesome( abc.c_str(), abc.length() ) );
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    char buf[5];
    EXPECT_EQ( 3, d.readsome( buf, 5 ) );
    EXPECT_EQ( "abc", std::string( buf, 3 ) );
}

TEST_F( DeserializeTest, String_text_with_space) {
    std::string text{ R"raw("Testing text with spaces.")raw" };
    EXPECT_EQ( (unsigned) text.length(), d.writesome( text.c_str(), text.length() ) );
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    const int buf_size{100};
    char buf[buf_size];
    EXPECT_EQ( (unsigned) text.length() - 2, d.readsome( buf, buf_size ) );
    EXPECT_EQ( text.substr( 1, text.length() - 2 ), std::string( buf, text.length() - 2 ) );
}

TEST_F( DeserializeTest, String_get ) {
    std::string json{ R"("Testing text with spaces.")" };
    EXPECT_EQ( (unsigned) json.length(), d.writesome( json.c_str(), json.length() ) );
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    std::string result{ d.get_string() };
    EXPECT_EQ( result, json.substr(1, json.length() - 2 ) );
}

TEST_F( DeserializeTest, String_escape ) {
    const std::string escape{ R"("Test \" \\ \/ \b \f \n \r \t \u0009 \u0000")" };
    const std::string result{ "Test \" \\ / \b \f \n \r \t \t \0",  24 };
    EXPECT_EQ( (unsigned) escape.length(), d.writesome( escape.c_str(), escape.length() ) );
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    char buf[50];
    EXPECT_EQ( (unsigned) result.length(), d.readsome( buf, 50 ) );
    EXPECT_EQ( result, std::string( buf, result.length() ) );
}

TEST_F( DeserializeTest, String_illegal ) {
    const std::string control_char{ "\"\n\"" };
    EXPECT_EQ( 1U, d.writesome( control_char.c_str(), control_char.length() ) );
    EXPECT_EQ( T::Error, d.get_type() );
    EXPECT_FALSE( d.is_complete() );
}

TEST_F( DeserializeTest, Object_empty) {
    // Start object
    EXPECT_TRUE( d.put( '{' ) );
    // Verify type
    EXPECT_EQ( T::Object, d.get_type() );
    // Should not be a complete object
    EXPECT_FALSE( d.is_complete() );
    // Should not be able to continue before object start is consumed
    EXPECT_FALSE( d.put( '}' ) );
    // Consume object start
    EXPECT_TRUE( d.pop() );
    // Object end
    EXPECT_TRUE( d.put( '}' ) );
    // Verify type
    EXPECT_EQ( T::Object, d.get_type() );
    // Should be a complete object
    EXPECT_TRUE( d.is_complete() );
    // Consume object
    EXPECT_TRUE( d.pop() );
}

TEST_F( DeserializeTest, Object_one_pair ) {
    EXPECT_EQ( 0U, ss->size() ); // Empty
    EXPECT_TRUE( d.put( '{' ) );
    EXPECT_EQ( 3U, ss->size() ); // Object -> O_start_needs_consuming -> Needs_consuming
    EXPECT_EQ( T::Object, d.get_type() );
    EXPECT_FALSE( d.is_complete() );
    EXPECT_TRUE( d.pop() );
    EXPECT_EQ( 1U, ss->size() ); // Object
    EXPECT_TRUE( d.put( '"' ) );
    EXPECT_TRUE( d.put( 'a' ) );
    EXPECT_EQ( 3U, ss->size() ); // Object -> O_expect_colon -> String
    EXPECT_TRUE( d.put( '"' ) );
    EXPECT_EQ( 4U, ss->size() ); // Object -> O_expect_colon -> String -> Complete
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_TRUE( d.pop() );
    EXPECT_EQ( 2U, ss->size() ); // Object -> O_expect_colon
    EXPECT_TRUE( d.put( ':' ) );
    EXPECT_EQ( 3U, ss->size() ); // Object -> O_value
    EXPECT_TRUE( d.put( '1' ) );
    EXPECT_EQ( 3U, ss->size() ); // Object -> O_value -> Number
    EXPECT_FALSE( d.put( '}' ) );
    EXPECT_EQ( 4U, ss->size() ); // Object -> O_value -> Number -> Complete
    EXPECT_EQ( T::Number, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_TRUE( d.pop() );
    EXPECT_EQ( 2U, ss->size() ); // Object -> O_value
    EXPECT_TRUE( d.put( '}' ) );
    EXPECT_EQ( T::Object, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_TRUE( d.pop() );
    EXPECT_EQ( 0U, ss->size() ); // Empty
}

TEST_F( DeserializeTest, Object_miltiple_pairs ) {
    const std::string json{ R"raw({"aLongKey":1,"b":"text","c":null,"d":true,"e":false,"f":1.0})raw" };
    unsigned index{ 0 };
    bool failed{ false };
    std::map<std::string, std::string> obj{};

    std::string key;
    std::string value;
    for( int i = 0; i < 13; ++i ) {
        // Read next
        while ( index < json.length() ) {
            if ( d.is_complete() ) {
                break;
            } else if ( d.put( json[index] ) ) {
                failed = false;
                ++index;
            } else if ( !d.is_complete() && T::Object == d.get_type() ) {
                // Should get here after a opening bracket
                if ( i != 0 ) {
                    FAIL() << "Should only get here at the start.";
                }
                if ( !d.pop() ) {
                    FAIL() << "Incomplete object that can't be popped.";
                }
            } else if ( !failed ) {
                failed = true;
            } else {
                FAIL();
            }
        }

        // Fill object map
        if ( i < 12 ) {
            const int max{ 20 };
            int len{ 0 };
            if ( 0 == ( i % 2 ) ) {
                char k[max];
                len = d.readsome( k, max );
                EXPECT_LT( 0, len );
                EXPECT_GE( max, len);
                d.pop();
                key = std::string( k, len );
            } else {
                switch ( d.get_type() ) {
                case T::Null:
                    value = "null";
                    d.pop();
                    break;
                case T::False:
                    value = "false";
                    d.pop();
                    break;
                case T::True:
                    value = "true";
                    d.pop();
                    break;
                case T::Number:
                    if ( d.is_integer() ) {
                        value = std::to_string( d.get_integer() );
                    } else {
                        value = std::to_string( d.get_double() );
                    }
                    break;
                case T::String:
                    char val[max];
                    len = d.readsome( val, max );
                    d.pop();
                    EXPECT_LT( 0, len );
                    EXPECT_GE( max, len );
                    EXPECT_EQ( 3, i );
                    value = std::string( val, len );
                    break;
                case T::Object:
                    break;
                default:
                    FAIL() << "Should not get to default!";
                    break;
                }
                obj.emplace( key, value );
            }
        }
    }

    // Verify object, floats are troublesome
    // {"aLongKey":1,"b":"text","c":null,"d":true,"e":false,"f":1.0}
    EXPECT_EQ( "1", obj["aLongKey"] );
    EXPECT_EQ( "text", obj["b"] );
    EXPECT_EQ( "null", obj["c"] );
    EXPECT_EQ( "true", obj["d"] );
    EXPECT_EQ( "false", obj["e"] );
    EXPECT_FLOAT_EQ( 1.0, std::stod( obj["f"] ) );
}

TEST_F( DeserializeTest, Array_empty ) {
    EXPECT_TRUE( d.put( '[' ) );
    EXPECT_EQ( T::Array, d.get_type() );
    EXPECT_FALSE( d.is_complete() );
    EXPECT_FALSE( d.put( ']' ) );
    EXPECT_TRUE( d.pop() );
    EXPECT_TRUE( d.put( ']' ) );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_EQ( T::Array, d.get_type() );
    EXPECT_TRUE( d.pop() );
}

TEST_F( DeserializeTest, Array_one ) {
    EXPECT_TRUE( d.put( '[' ) );
    EXPECT_TRUE( d.pop() );

    std::string text{ R"("test")" };
    EXPECT_EQ( (std::streamsize) text.length(), d.writesome( text.c_str(), text.length() ) );
    EXPECT_EQ( T::String, d.get_type() );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_TRUE( d.pop() );

    EXPECT_FALSE( d.is_complete() );
    // EXPECT_EQ( T::Array, d.get_type() ); // TODO: Fails, the type is not changed after popping
    EXPECT_TRUE( d.put( ']' ) );
    EXPECT_TRUE( d.is_complete() );
    EXPECT_EQ( T::Array, d.get_type() );
    EXPECT_TRUE( d.pop() );
}

TEST_F( DeserializeTest, Aarray_multiple ) {
    const std::string json{ R"([1,-25,"c","text",1.0,-1e-2,true,false,null])" };
    const unsigned long  num_of_elements{ (unsigned long) std::count( json.cbegin(), json.cend(), ',' ) + 1 };
    std::vector<std::string> arr{};

    unsigned index{ 0 };
    for ( unsigned long i = 0; i < num_of_elements; ++i ) {
        // Read input
        bool failed{ false };
        while ( index < json.length() ) {
            if ( d.is_complete() ) {
                break;
            } else if ( d.put( json[index] ) ) {
                failed = false;
                ++index;
            } else if ( !d.is_complete() && T::Array == d.get_type() ) {
                // Should get here after a opening bracket
                if ( i != 0 ) {
                    FAIL() << "Should only get here at the start.";
                }
                if ( !d.pop() ) {
                    FAIL() << "Incomplete array that can't be popped.";
                }
            } else if ( !failed ) {
                failed = true;
            } else {
                FAIL();
            }
        }

        // Build vector array
        std::string value{};
        unsigned len{};
        const unsigned max{ 50 };
        switch ( d.get_type() ) {
        case T::Null:
            value = "null";
            d.pop();
            break;
        case T::False:
            value = "false";
            d.pop();
            break;
        case T::True:
            value = "true";
            d.pop();
            break;
        case T::Number:
            if ( d.is_integer() ) {
                value = std::to_string( d.get_integer() );
            } else {
                value = std::to_string( d.get_double() );
            }
            break;
        case T::String:
            char val[max];
            len = d.readsome( val, max );
            d.pop();
            EXPECT_LT( 0U, len );
            EXPECT_GE( max, len );
            value = std::string( val, len );
            break;
        case T::Object:
            break;
        default:
            FAIL() << "Should not get to default!";
            break;
        }
        arr.push_back( value );
    }

    // Verify array
    // [1,-25,"c","text",1.0,-1e-2,true,false,null]
    EXPECT_EQ( "1", arr[0] );
    EXPECT_EQ( "-25", arr[1] );
    EXPECT_EQ( "c", arr[2] );
    EXPECT_EQ( "text", arr[3] );
    EXPECT_FLOAT_EQ( 1.0 , std::stod( arr[4] ) );
    EXPECT_FLOAT_EQ( -0.01 , std::stod( arr[5] ) );
    EXPECT_EQ( "true", arr[6] );
    EXPECT_EQ( "false", arr[7] );
    EXPECT_EQ( "null", arr[8] );
}

TEST_F( DeserializeTest, In_stream ) {
    const std::string json{ R"([1,-25,"c","text",1.0,-1e-2,true,false,null])" };
    std::stringstream sstream{ json };
    d.set_istream( sstream );

    // Add 1 for the array start and 1 for the array end
    const unsigned long  num_of_elements{ (unsigned long) std::count( json.cbegin(), json.cend(), ',' ) + 2 };

    std::vector<std::string> arr{};

    for ( unsigned long i = 0; i < num_of_elements; ++i ) {
        // Read input
        bool failed_once{ false };
        while ( true ) {
            if ( d.is_complete() ) {
                break;
            } else if ( d.read_istream() ) {
                failed_once = false;
            } else if ( !d.is_complete() && T::Array == d.get_type() ) {
                // Should get here after a opening bracket
                if ( i != 0 ) {
                    FAIL() << "Should only get here at the start.";
                }
                if ( !d.pop() ) {
                    FAIL() << "Incomplete array that can't be popped.";
                }
            } else if ( !failed_once ) {
                failed_once = true;
            } else {
                FAIL();
            }
        }

        // Build vector array
        std::string value{};
        unsigned len{};
        const unsigned max{ 50 };
        switch ( d.get_type() ) {
        case T::Null:
            value = "null";
            d.pop();
            break;
        case T::False:
            value = "false";
            d.pop();
            break;
        case T::True:
            value = "true";
            d.pop();
            break;
        case T::Number:
            if ( d.is_integer() ) {
                value = std::to_string( d.get_integer() );
            } else {
                value = std::to_string( d.get_double() );
            }
            break;
        case T::String:
            char val[max];
            len = d.readsome( val, max );
            EXPECT_TRUE( d.pop() );
            EXPECT_LT( 0U, len );
            EXPECT_GE( max, len );
            value = std::string( val, len );
            break;
        case T::Array:
            value = "";
            EXPECT_TRUE( d.pop() );
            break;
        default:
            FAIL() << "Should not get to default!";
            break;
        }
        arr.push_back( value );
    }

    // Verify array
    // [1,-25,"c","text",1.0,-1e-2,true,false,null]
    EXPECT_EQ( "1", arr[0] );
    EXPECT_EQ( "-25", arr[1] );
    EXPECT_EQ( "c", arr[2] );
    EXPECT_EQ( "text", arr[3] );
    EXPECT_FLOAT_EQ( 1.0 , std::stod( arr[4] ) );
    EXPECT_FLOAT_EQ( -0.01 , std::stod( arr[5] ) );
    EXPECT_EQ( "true", arr[6] );
    EXPECT_EQ( "false", arr[7] );
    EXPECT_EQ( "null", arr[8] );
    EXPECT_EQ( "", arr[9] ); // Array end added by above loop
    EXPECT_EQ( 10, (int) arr.size() );
}

class JsonValueGenerationTest : public DeserializeTest {
public:
    JsonValueGenerationTest() = default;
    virtual ~JsonValueGenerationTest() = default;
};

TEST_F( JsonValueGenerationTest, Null ) {
    const std::string json{ "null" };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_null() );
}

TEST_F( JsonValueGenerationTest, True ) {
    const std::string json{ "true" };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_true() );
}

TEST_F( JsonValueGenerationTest, False ) {
    const std::string json{ "false" };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_false() );
}

TEST_F( JsonValueGenerationTest, Number ) {
    const std::string json{ "1 " };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_number() );
}

TEST_F( JsonValueGenerationTest, String ) {
    const std::string json{ "\"test\"" };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_string() );
}

TEST_F( JsonValueGenerationTest, Array ) {
    const std::string json{ "[]" };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_array() );
}

TEST_F( JsonValueGenerationTest, Object ) {
    const std::string json{ "{}" };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_object() );
}

TEST_F( JsonValueGenerationTest, Complex ) {
    const std::string json{ R"({"a":[true,"string",{"inner":true}],"b":null,"c":"test"})" };
    J::Value value{ d.generate_json_value( json ) };
    EXPECT_TRUE( value.is_object() );
    EXPECT_EQ( 3u, value.object.size() );
    ASSERT_TRUE( value.at( "a" ).is_array() );
    EXPECT_TRUE( value.at( "a" ).at( 0 ).is_true() );
    ASSERT_TRUE( value.at( "c" ).is_string() );
    EXPECT_EQ( "test", value.at( "c" ).get_string() );
}

/***************************************************************/

using S = J::Serialize;

std::vector<char> vec{};
void output( const char c ) { vec.push_back( c ); }

TEST( State, Ctor_Dtor ) {
    std::vector<S::State> v;
    v.push_back( S::State{ T::Array } );
    v.push_back( S::State{ T::Error } );
    v.push_back( S::State{ T::False } );
    v.push_back( S::State{ T::Null } );
    v.push_back( S::State{ T::Number } );
    v.push_back( S::State{ T::Object } );
    v.push_back( S::State{ T::String } );
    v.push_back( S::State{ T::True } );
}

TEST( Error_state, Good ) {
    S::Error_state e{};
    EXPECT_FALSE( e.accept_close() );
    EXPECT_FALSE( e.accept_value() );
    EXPECT_FALSE( e.accept_string() );

    EXPECT_FALSE( e.create_value() );
    EXPECT_FALSE( e.create_string() );
    EXPECT_FALSE( e.close() );
}

TEST( String_state, Good ) {
    vec.clear();
    S::String_state s{ &output };
    EXPECT_EQ( '"', vec.back() );

    EXPECT_FALSE( s.accept_string() );
    EXPECT_FALSE( s.create_string() );

    EXPECT_FALSE( s.accept_value() );
    EXPECT_FALSE( s.create_value() );

    EXPECT_TRUE( s.accept_close() );
    EXPECT_TRUE( s.close() );
    EXPECT_EQ( 2, (int) vec.size() );
    EXPECT_EQ( '"', vec.back() );
}

TEST( Array_state, Good) {
    vec.clear();
    unsigned i{ 0 };
    S::Array_state a{ &output };
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '[', vec.back() );

    EXPECT_TRUE( a.accept_string() );
    EXPECT_TRUE( a.create_string() );
    // After this state input json string (value) here

    EXPECT_TRUE( a.accept_value() );
    EXPECT_TRUE( a.create_value() );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ',', vec.back() );
    // After this state input json value here

    EXPECT_TRUE( a.accept_close() );
    EXPECT_TRUE( a.close() );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ']', vec.back() );
}

TEST( Object_state, Good) {
    vec.clear();
    unsigned i{ 0 };
    S::Object_state o{ &output };
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '{', vec.back() );

    EXPECT_TRUE( o.accept_string() );
    EXPECT_TRUE( o.create_string() );
    // After this state input json string here

    EXPECT_TRUE( o.accept_value() );
    EXPECT_TRUE( o.create_value() );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ':', vec.back() );
    // After this state input json value here

    EXPECT_TRUE( o.accept_string() );
    EXPECT_TRUE( o.create_string() );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ',', vec.back() );
    // After this state input json string here

    EXPECT_TRUE( o.accept_value() );
    EXPECT_TRUE( o.create_value() );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ':', vec.back() );
    // After this state input json value here

    EXPECT_TRUE( o.accept_close() );
    EXPECT_TRUE( o.close() );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '}', vec.back() );
}

TEST( State_machiene, Simple ) {
    vec.clear();
    S::State_machiene s{ &output };

    EXPECT_TRUE( s.create( T::False ) );
    EXPECT_TRUE( s.create( T::Null ) );
    EXPECT_TRUE( s.create( T::Number ) );
    EXPECT_TRUE( s.create( T::True ) );

    EXPECT_FALSE( s.create( T::Array ) );
    EXPECT_FALSE( s.create( T::Error ) );
    EXPECT_FALSE( s.create( T::Object ) );
    EXPECT_FALSE( s.create( T::String ) );

    EXPECT_TRUE( vec.empty() );
}

TEST( State_machiene, Good_string ) {
    vec.clear();
    unsigned i{ 0 };
    S::State_machiene s{ &output };

    EXPECT_TRUE( s.open( T::String ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
    EXPECT_EQ( T::String, s.type() );

    EXPECT_TRUE( s.close( T::String ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
}

TEST( State_machiene, Good_array ) {
    vec.clear();
    unsigned i{ 0 };
    S::State_machiene s{ &output };

    EXPECT_TRUE( s.open( T::Array ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '[', vec.back() );
    EXPECT_EQ( T::Array, s.type() );

    EXPECT_TRUE( s.open( T::String ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
    EXPECT_EQ( T::String, s.type() );

    EXPECT_TRUE( s.close( T::String ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
    EXPECT_EQ( T::Array, s.type() );

    EXPECT_TRUE( s.open( T::Array ) );
    EXPECT_EQ( ',', vec.at( i++ ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '[', vec.back() );
    EXPECT_EQ( T::Array, s.type() );

    EXPECT_TRUE( s.close( T::Array ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ']', vec.back() );
    EXPECT_EQ( T::Array, s.type() );

    EXPECT_TRUE( s.close( T::Array ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ']', vec.back() );
    EXPECT_EQ( T::Error, s.type() );
}

TEST( State_machiene, Good_object ) {
    vec.clear();
    unsigned i{ 0 };
    S::State_machiene s{ &output };

    EXPECT_TRUE( s.open( T::Object ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '{', vec.back() );
    EXPECT_EQ( T::Object, s.type() );

    // First pair
    EXPECT_TRUE( s.open( T::String ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
    EXPECT_EQ( T::String, s.type() );
    // Input key string here

    EXPECT_TRUE( s.close( T::String ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
    EXPECT_EQ( T::Object, s.type() );

    EXPECT_TRUE( s.open( T::Array ) );
    EXPECT_EQ( ':', vec.at( i++ ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '[', vec.back() );
    EXPECT_EQ( T::Array, s.type() );
    // Input json value here

    EXPECT_TRUE( s.close( T::Array ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ']', vec.back() );
    EXPECT_EQ( T::Object, s.type() );

    // Second pair
    EXPECT_TRUE( s.open( T::String ) );
    EXPECT_EQ( ',', vec.at( i++ ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
    EXPECT_EQ( T::String, s.type() );
    // Input key string here

    EXPECT_TRUE( s.close( T::String ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '"', vec.back() );
    EXPECT_EQ( T::Object, s.type() );

    EXPECT_TRUE( s.create( T::True ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( ':', vec.back() );
    // Input true here

    EXPECT_TRUE( s.close( T::Object ) );
    EXPECT_EQ( ++i, vec.size() );
    EXPECT_EQ( '}', vec.back() );
    EXPECT_EQ( T::Error, s.type() );
}

class SerializeTest:  public ::testing::Test {
public:
    SerializeTest() : s{} {}
    virtual ~SerializeTest() = default;

    S s;
};

TEST_F( SerializeTest, Empty ) {
    EXPECT_TRUE( s.empty() );
}

TEST_F( SerializeTest, Error ) {
    EXPECT_TRUE( s.empty() );
}

TEST_F( SerializeTest, Null ) {
    ASSERT_TRUE( s.put( nullptr ) );
    const std::string str{ "null" };
    ASSERT_EQ( (int) str.size(), s.size() );
    for( unsigned i{ 0 }; i < str.size(); ++i ) {
        EXPECT_EQ( str.at( i ), s.get() );
    }
}

TEST_F( SerializeTest, True ) {
    ASSERT_TRUE( s.put( true ) );
    const std::string str{ "true" };
    ASSERT_EQ( (int) str.size(), s.size() );
    for( unsigned i{ 0 }; i < str.size(); ++i ) {
        EXPECT_EQ( str.at( i ), s.get() );
    }
}

TEST_F( SerializeTest, False ) {
    ASSERT_TRUE( s.put( false ) );
    const std::string str{ "false" };
    ASSERT_EQ( (int) str.size(), s.size() );
    for( unsigned i{ 0 }; i < str.size(); ++i ) {
        EXPECT_EQ( str.at( i ), s.get() );
    }
}

TEST_F( SerializeTest, Integer ) {
    ASSERT_TRUE( s.put( (long long) 10 ) );
    ASSERT_EQ( 2, s.size() );
    EXPECT_EQ( '1', s.get() );
    EXPECT_EQ( '0', s.get() );
}

TEST_F( SerializeTest, Float ) {
    ASSERT_TRUE( s.put( (long double) 10.1 ) );
    ASSERT_LT( 5, s.size() );
    EXPECT_EQ( '1', s.get() );
    EXPECT_EQ( '0', s.get() );
    EXPECT_EQ( '.', s.get() );
    char d{ s.get() };
    EXPECT_TRUE( ( '1' == d || '0' == d ) );
    if ( '0' == d ) {
        EXPECT_EQ( '9', s.get() );
    }
}

TEST_F( SerializeTest, String ) {
    const std::string str{ "text" };
    ASSERT_TRUE( s.put( str ) );
    ASSERT_EQ( (int) str.size() + 2, s.size() );
    EXPECT_EQ( '"', s.get() );
    for( unsigned i{ 0 }; i < str.size(); ++i ) {
        EXPECT_EQ( str.at( i ), s.get() );
    }
    EXPECT_EQ( '"', s.get() );
    EXPECT_TRUE( s.empty() );
}

TEST_F( SerializeTest, Chars ) {
    const std::string str{ "text" };
    ASSERT_TRUE( s.start_string() );
    ASSERT_EQ( 1U, s.size() );
    ASSERT_EQ( '"', s.get() );
    for( unsigned i{ 0 }; i < str.size(); ++i ) {
        EXPECT_TRUE( s.put( str.at( i ) ) );
        ASSERT_EQ( 1U, s.size() );
        ASSERT_EQ( str.at( i ), s.get() );
    }
    ASSERT_TRUE( s.close_string() );
    ASSERT_EQ( 1U, s.size() );
    ASSERT_EQ( '"', s.get() );
    ASSERT_TRUE( s.empty() );
}

TEST_F( SerializeTest, Escape ) {
    const std::string str{ "\\\"\b\f\n\r\t\v\0", 9 };
    const std::string res{ R"("\\\"\b\f\n\r\t\u000b\u0000")" };
    ASSERT_TRUE( s.put( str ) );
    ASSERT_EQ( (int) res.size(), s.size() );
    for( unsigned i{ 0 }; i < res.size(); ++i ){
        EXPECT_EQ( res.at(i), s.get() );
    }
    EXPECT_TRUE( s.empty() );
}

TEST_F( SerializeTest, Array ) {
    ASSERT_TRUE( s.start_array() );
    ASSERT_TRUE( s.put( nullptr ) );
    ASSERT_TRUE( s.put( true ) );
    ASSERT_TRUE( s.put( false ) );
    ASSERT_TRUE( s.put( "test" ) );
    ASSERT_TRUE( s.put( (long long) -10 ) );
    ASSERT_TRUE( s.start_array() );
    ASSERT_TRUE( s.start_array() );
    ASSERT_TRUE( s.close_array() );
    ASSERT_TRUE( s.close_array() );
    ASSERT_TRUE( s.start_array() );
    ASSERT_TRUE( s.put( "text" ) );
    ASSERT_TRUE( s.put( true ) );
    ASSERT_TRUE( s.close_array() );
    ASSERT_TRUE( s.start_string() );
    ASSERT_TRUE( s.put( 'c' ) );
    ASSERT_TRUE( s.close_string() );
    ASSERT_TRUE( s.close_array() );

    // Verify output
    std::string json_test_array{
        R"([null,true,false,"test",-10,[[]],["text",true],"c"])"
    };
    for( std::string::const_iterator it{ json_test_array.cbegin() }; it != json_test_array.cend(); ++it ) {
        EXPECT_EQ( *it, s.get() );
    }
    EXPECT_TRUE( s.empty() );
}

TEST_F( SerializeTest, Object ) {
    ASSERT_TRUE( s.start_object() );
    ASSERT_TRUE( s.put( "arr" ) );
    ASSERT_TRUE( s.start_array() );
    ASSERT_TRUE( s.put( nullptr ) );
    ASSERT_TRUE( s.put( true ) );
    ASSERT_TRUE( s.put( false ) );
    ASSERT_TRUE( s.put( "test" ) );
    ASSERT_TRUE( s.put( (long long) -10 ) );
    ASSERT_TRUE( s.start_array() );
    ASSERT_TRUE( s.start_object() );
    ASSERT_TRUE( s.close_object() );
    ASSERT_TRUE( s.close_array() );
    ASSERT_TRUE( s.start_object() );
    ASSERT_TRUE( s.put( "text" ) );
    ASSERT_TRUE( s.put( true ) );
    ASSERT_TRUE( s.close_object() );
    ASSERT_TRUE( s.start_string() );
    ASSERT_TRUE( s.put( 'c' ) );
    ASSERT_TRUE( s.close_string() );
    ASSERT_TRUE( s.close_array() );
    ASSERT_TRUE( s.put( "a" ) );
    ASSERT_TRUE( s.put( "b" ) );
    ASSERT_TRUE( s.close_object() );

    // Verify output
    std::string json_test_array{
        R"({"arr":[null,true,false,"test",-10,[{}],{"text":true},"c"],"a":"b"})"
    };
    for( std::string::const_iterator it{ json_test_array.cbegin() }; it != json_test_array.cend(); ++it ) {
        EXPECT_EQ( *it, s.get() );
    }
    EXPECT_TRUE( s.empty() );
}

TEST_F( SerializeTest, Out_stream ) {
    const std::string text( "test string" );
    std::stringstream sstream{};
    s.set_ostream( sstream );

    EXPECT_TRUE( s.start_array() );

    EXPECT_TRUE( s.start_string() );
    for( std::string::const_iterator it{ text.cbegin() }; it != text.cend(); ++it ) {
        s.put( *it );
    }
    EXPECT_TRUE( s.close_string() );

    EXPECT_TRUE( s.start_object() );
    EXPECT_TRUE( s.put( "key" ) );
    EXPECT_TRUE( s.create_null() );
    EXPECT_TRUE( s.close_object() );

    EXPECT_TRUE( s.close_array() );

    const std::string result{ R"(["test string",{"key":null}])" };
    EXPECT_EQ( result, sstream.str() );
}

TEST( Serialize_Deserialize_Test, Test ) {
    std::stringstream ss;
    S s{};
    s.set_ostream( ss );
    D d{};
    d.set_istream( ss );

    s.start_object();
    s.put( "true" );
    s.put( true );
    s.put( "object" );
    s.start_object();
    s.put( "null" );
    s.put( nullptr );
    s.put( "array" );
    s.start_array();
    s.close_array();
    s.close_object();
    s.close_object();

    EXPECT_TRUE( s.is_complete() );

    while( d.read_istream() ) {
        // TODO: remove this loop and verify the results instead
        EXPECT_TRUE( d.pop() );
    }

    EXPECT_EQ( -1, ss.peek() );
}

/***************************************************************/

TEST( JSON_struct, basic_json_value ) {
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::basic_json_value() ) );
    EXPECT_EQ( T::Error, val->get_type() );
    EXPECT_EQ( "ERROR", val->to_json() );
}

TEST( JSON_struct, Null ) {
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::Null() ) );
    EXPECT_TRUE( val->is_null() );
    EXPECT_EQ( T::Null, val->get_type() );
    EXPECT_EQ( "null", val->to_json() );
}

TEST( JSON_struct, True ) {
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::True() ) );
    EXPECT_TRUE( val->is_true() );
    EXPECT_EQ( T::True, val->get_type() );
    EXPECT_EQ( "true", val->to_json() );
}

TEST( JSON_struct, False ) {
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::False() ) );
    EXPECT_TRUE( val->is_false() );
    EXPECT_EQ( T::False, val->get_type() );
    EXPECT_EQ( "false", val->to_json() );
}

TEST( JSON_struct, Number_integer ) {
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::Number( 0LL ) ) );
    EXPECT_TRUE( val->is_number() );
    EXPECT_EQ( T::Number, val->get_type() );
    EXPECT_EQ( "0", val->to_json() );
    ASSERT_TRUE( static_cast<J::Number*>( val.get() )->is_integer() );
    EXPECT_EQ( 0LL, static_cast<J::Number*>( val.get() )->get_integer() );

    ASSERT_NO_THROW( val.reset( new J::Number( 1LL ) ) );
    EXPECT_EQ( T::Number, val->get_type() );
    EXPECT_EQ( "1", val->to_json() );
    EXPECT_TRUE( static_cast<J::Number*>( val.get() )->is_integer() );
    EXPECT_EQ( 1LL, static_cast<J::Number*>( val.get() )->get_integer() );

    ASSERT_NO_THROW( val.reset( new J::Number( -1LL ) ) );
    EXPECT_EQ( T::Number, val->get_type() );
    EXPECT_EQ( "-1", val->to_json() );
    EXPECT_TRUE( static_cast<J::Number*>( val.get() )->is_integer() );
    EXPECT_EQ( -1LL, static_cast<J::Number*>( val.get() )->get_integer() );
}

TEST( JSON_struct, Number_float ) {
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::Number( 0.0L ) ) );
    EXPECT_TRUE( val->is_number() );
    EXPECT_EQ( T::Number, val->get_type() );
    EXPECT_EQ( "0.000000", val->to_json() );
    ASSERT_TRUE( static_cast<J::Number*>( val.get() )->is_float() );
    EXPECT_FLOAT_EQ( 0.0L, static_cast<J::Number*>( val.get() )->get_float() );

    ASSERT_NO_THROW( val.reset( new J::Number( 1.0L ) ) );
    EXPECT_EQ( T::Number, val->get_type() );
    EXPECT_EQ( "1.000000", val->to_json() );
    EXPECT_TRUE( static_cast<J::Number*>( val.get() )->is_float() );
    EXPECT_FLOAT_EQ( 1.0L, static_cast<J::Number*>( val.get() )->get_float() );

    ASSERT_NO_THROW( val.reset( new J::Number( -1.0L ) ) );
    EXPECT_EQ( T::Number, val->get_type() );
    EXPECT_EQ( "-1.000000", val->to_json() );
    EXPECT_TRUE( static_cast<J::Number*>( val.get() )->is_float() );
    EXPECT_FLOAT_EQ( -1.0L, static_cast<J::Number*>( val.get() )->get_float() );
}

TEST( JSON_struct, Number_string ) {
    // Strings default to floats at the moment.
    // It does not modify the string, and have no real validation.
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::Number( "-1" ) ) );
    ASSERT_TRUE( val->is_number() );
    EXPECT_EQ( T::Number, val->get_type() );
    EXPECT_EQ( "-1", val->to_json() );
    EXPECT_TRUE( static_cast<J::Number*>( val.get() )->is_float() );
}

TEST( JSON_struct, String ) {
    // TODO: rewrite test once string is fed thoguh json serializer
    std::unique_ptr<J::basic_json_value> val;
    ASSERT_NO_THROW( val.reset( new J::String() ) );
    ASSERT_TRUE( val->is_string() );
    EXPECT_EQ( T::String, val->get_type() );
    EXPECT_EQ( "", val->to_json() );

    ASSERT_NO_THROW( val.reset( new J::String( "test" ) ) );
    ASSERT_TRUE( val->is_string() );
    EXPECT_EQ( T::String, val->get_type() );
    EXPECT_EQ( "test", val->to_json() );
}

TEST( JSON_struct, Array ) {
    using U = std::unique_ptr<J::basic_json_value>;

    U val;
    ASSERT_NO_THROW( val.reset( new J::Array() ) );
    ASSERT_TRUE( val->is_array() );
    EXPECT_EQ( T::Array, val->get_type() );
    J::Array* a{ static_cast<J::Array*>( val.get() ) };
    EXPECT_TRUE( a->empty() );
    EXPECT_EQ( 0u, a->size() );

    //a->emplace_back( new J::Null() );
    a->push_back( U( new J::Null() ) );
    EXPECT_FALSE( a->empty() );
    ASSERT_EQ( 1u, a->size() );
    EXPECT_EQ( T::Null, ( *a )[0].get_type() );

    //a->emplace_back( new J::True() );
    a->push_back( U( new J::True() ) );
    ASSERT_EQ( 2u, a->size() );
    EXPECT_EQ( T::True, a->at( 1 ).get_type() );

    a->push_back( a->copy() );
    ASSERT_EQ( 3u, a->size() );
    ASSERT_EQ( T::Array, a->at( 2 ).get_type() );
    J::Array* a2{ static_cast<J::Array*>( &( a->at(2) ) ) };
    ASSERT_EQ( 2u, a2->size() );
    EXPECT_EQ( T::Null, a2->at( 0 ).get_type() );

    a->clear();
    EXPECT_TRUE( a->empty() );
}

TEST( JSON_struct, Object ) {
    using U = std::unique_ptr<J::basic_json_value>;

    std::unique_ptr<J::Object> obj( new J::Object() );
    ASSERT_EQ( T::Object, obj->get_type() );
    ASSERT_TRUE( obj->is_object() );
    EXPECT_TRUE( obj->empty() );
    EXPECT_EQ( 0u, obj->size() );

    //obj->emplace( "b", new J::Null() );
    obj->insert( J::String( "b" ), U( new J::Null() ) );
    EXPECT_FALSE( obj->empty() );
    ASSERT_EQ( 1u, obj->size() );
    EXPECT_EQ( T::Null, obj->at( "b" ).get_type() );

    obj->insert( J::String( "a" ), obj->copy() );
    ASSERT_EQ( 2u, obj->size() );
    ASSERT_EQ( T::Object, obj->at( "a" ).get_type() );
    EXPECT_EQ( T::Null, static_cast<J::Object*>( &( obj->at( "a" ) ) )->at( "b" ).get_type() );

    // Test that the object is sorted
    EXPECT_EQ( "a", obj->cbegin()->first.string );
    EXPECT_EQ( "b", ( ++obj->cbegin() )->first.string );
}

class ex_err : public std::runtime_error {
public:
    ex_err(): std::runtime_error( "" ) {}
};
class ex_null : public std::runtime_error {
public:
    ex_null(): std::runtime_error( "" ) {}
};
class ex_true : public std::runtime_error {
public:
    ex_true(): std::runtime_error( "" ) {}
};
class ex_false : public std::runtime_error {
public:
    ex_false(): std::runtime_error( "" ) {}
};
class ex_int : public std::runtime_error {
public:
    ex_int(): std::runtime_error( "" ) {}
};
class ex_float : public std::runtime_error {
public:
    ex_float(): std::runtime_error( "" ) {}
};
class ex_str_s : public std::runtime_error {
public:
    ex_str_s(): std::runtime_error( "" ) {}
};
class ex_str_e : public std::runtime_error {
public:
    ex_str_e(): std::runtime_error( "" ) {}
};
class ex_arr_s : public std::runtime_error {
public:
    ex_arr_s(): std::runtime_error( "" ) {}
};
class ex_arr_e : public std::runtime_error {
public:
    ex_arr_e(): std::runtime_error( "" ) {}
};
class ex_obj_s : public std::runtime_error {
public:
    ex_obj_s(): std::runtime_error( "" ) {}
};
class ex_obj_e : public std::runtime_error {
public:
    ex_obj_e(): std::runtime_error( "" ) {}
};

using E = J::Event;

class Mediator : public J::Event_handler {
public:
    D d;
    std::shared_ptr<J::Event_receiver> receiver{ J::Event_receiver::new_shared() };

    Mediator(): J::Event_handler(), d{} {
        receiver->set_event_handler( this );
        d.add_event_receiver( receiver.get() );
    }
    virtual ~Mediator() {

    }

    virtual void event( E e ) {
        switch( e ) {
        case E::Error:
            throw ex_err();
        case E::Null:
            throw ex_null();
        case E::True:
            throw ex_true();
        case E::False:
            throw ex_false();
        case E::Integer:
            throw ex_int();
        case E::Float:
            throw ex_float();
        case E::String_start:
            throw ex_str_s();
        case E::String_end:
            throw ex_str_e();
        case E::Array_start:
            throw ex_arr_s();
        case E::Array_end:
            throw ex_arr_e();
        case E::Object_start:
            throw ex_obj_s();
        case E::Object_end:
            throw ex_obj_e();
        default:
            throw std::runtime_error( "" );
        }
    }
};

class JSON_Deserialize_Event_Test : public ::testing::Test {
public:
    JSON_Deserialize_Event_Test() : ::testing::Test() {}
    virtual ~JSON_Deserialize_Event_Test() {}

    Mediator m{};
};

TEST_F( JSON_Deserialize_Event_Test, Empty ) {

}

TEST_F( JSON_Deserialize_Event_Test, Null ) {
    std::string json{ "null" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_null );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, True ) {
    std::string json{ "true" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_true );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, False ) {
    std::string json{ "false" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_false );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, Integer ) {
    std::string json{ "1\0" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_int );
    EXPECT_EQ( 1LL, m.d.get_integer() );
}

TEST_F( JSON_Deserialize_Event_Test, Float ) {
    std::string json{ "1.0\0" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_float );
    EXPECT_FLOAT_EQ( 1.0L, m.d.get_double() );
}

TEST_F( JSON_Deserialize_Event_Test, String_Empty ) {
    std::string json{ "\"\"" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_THROW( m.d.put( *it++ ), ex_str_s );
    EXPECT_THROW( m.d.put( *it++ ), ex_str_e );
    EXPECT_EQ( "", m.d.get_string() );
}

TEST_F( JSON_Deserialize_Event_Test, String_Text ) {
    std::string json{ R"("Test")" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_THROW( m.d.put( *it++ ), ex_str_s );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_str_e );
    EXPECT_EQ( "Test", m.d.get_string() );
}

TEST_F( JSON_Deserialize_Event_Test, Array_Empty ) {
    std::string json{ "[]" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, Array_One ) {
    std::string json{ "[true]" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_true );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, Array_Many ) {
    std::string json{ "[true,true]" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_true );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );

    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_true );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, Array_Deep ) {
    std::string json{ "[[],[true,[true,[]]]]" };
    std::string::const_iterator it{ json.cbegin() };

    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );

    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_true );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );

    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_true );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );

    EXPECT_THROW( m.d.put( *it++ ), ex_arr_s );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_arr_e );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, Object_empty ) {
    std::string json{ "{}" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_s );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_e );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, Object_One ) {
    std::string json{ R"({"t":true})" };
    std::string::const_iterator it{ json.cbegin() };
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_s );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_THROW( m.d.put( *it++ ), ex_str_s );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_str_e );
    EXPECT_EQ( "t", m.d.get_string() );

    EXPECT_NO_THROW( m.d.put( *it++ ) );

    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_true );
    EXPECT_TRUE( m.d.pop() );

    EXPECT_THROW( m.d.put( *it++ ), ex_obj_e );
    EXPECT_TRUE( m.d.pop() );
}

TEST_F( JSON_Deserialize_Event_Test, Object_Deep ) {
    std::string json{ R"({"e":{},"d":{"n":1,"m":{}}})" };
    std::string::const_iterator it{ json.cbegin() };

    // {
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_s );
    EXPECT_TRUE( m.d.pop() );

    // "e"
    EXPECT_THROW( m.d.put( *it++ ), ex_str_s );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_str_e );
    EXPECT_EQ( "e", m.d.get_string() );

    // :
    EXPECT_NO_THROW( m.d.put( *it++ ) );

    // {}
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_s );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_e );
    EXPECT_TRUE( m.d.pop() );

    // ,
    EXPECT_NO_THROW( m.d.put( *it++ ) );

    // "d"
    EXPECT_THROW( m.d.put( *it++ ), ex_str_s );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_str_e );
    EXPECT_EQ( "d", m.d.get_string() );

    // :
    EXPECT_NO_THROW( m.d.put( *it++ ) );

    // {
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_s );
    EXPECT_TRUE( m.d.pop() );

    // "n"
    EXPECT_THROW( m.d.put( *it++ ), ex_str_s );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_str_e );
    EXPECT_EQ( "n", m.d.get_string() );

    // :
    EXPECT_NO_THROW( m.d.put( *it++ ) );

    // 1
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it ), ex_int );
    EXPECT_EQ( 1LL, m.d.get_integer() );

    // ,
    EXPECT_NO_THROW( m.d.put( *it++ ) );

    // "m"
    EXPECT_THROW( m.d.put( *it++ ), ex_str_s );
    EXPECT_NO_THROW( m.d.put( *it++ ) );
    EXPECT_THROW( m.d.put( *it++ ), ex_str_e );
    EXPECT_EQ( "m", m.d.get_string() );

    // :
    EXPECT_NO_THROW( m.d.put( *it++ ) );

    // {}}}
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_s );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_e );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_e );
    EXPECT_TRUE( m.d.pop() );
    EXPECT_THROW( m.d.put( *it++ ), ex_obj_e );
    EXPECT_TRUE( m.d.pop() );
}

}
