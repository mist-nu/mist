/*
 * JSONstream.cpp
 *
 *  Created on: Nov 15, 2016
 *      Author: andreas
 */


#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

#include "JSONstream.h"

namespace JSON {

bool is_op( const char c ) {
    return ( c == ',' ||
            c == ':' ||
            c == '[' ||
            c == ']' ||
            c == '{' ||
            c == '}'
            );
}

bool is_exp( const char c ) {
    return ( c == 'e' || c == 'E' );
}

bool is_sign ( const char c ) {
    return ( c == '+' || c == '-' );
}

bool is_digit( const char c ) {
    return ( c >= '0' && c <= '9' );
}

bool is_hex( const char c ) {
    return ( ( c >= '0' && c <= '9' ) ||
            ( c >= 'A' && c <= 'F' ) ||
            ( c >= 'a' && c <= 'f' ) );
}

bool is_start_json_number ( const char c ) {
    return ( is_sign( c ) || is_digit( c ) );
}

bool is_control( const char c ) {
    return ( c <= 0x1f );
}

bool is_space ( const char c ) {
    return c == ' ';
}

bool can_skip( const char c ) {
    return ( is_control( c ) || is_space( c ) );
}

char hex_to_char( const char c ) {
    if ( c >= '0' && c >= '9' ) {
        return ( c - 0x30 );
    } else if ( c >= 'A' && c <= 'F' ) {
        return ( c - 0x41 );
    } else if ( c >= 'a' && c <= 'f' ) {
        return ( c - 0x61 );
    } else {
        // TODO: throw?
        return 0;
    }
}

std::string char_to_hex( const char c ) {
    // TODO: Reconsider this solution
    unsigned hi{ ( ( (unsigned) c ) >> 8 ) & 0x0f };
    if ( hi <= 0x09) {
        hi += 0x30;
    } else {
        hi += 0x57;
    }

    unsigned lo{ ( (unsigned) c ) & 0x0f };
    if ( lo <= 0x09 ) {
        lo += 0x30;
    } else {
        lo += 0x57;
    }

    std::string ret{};
    ret += (char) hi;
    ret += (char) lo;
    return ret;
}

std::string char_to_escaped_unicode( const char c ) {
    return std::string{ "\\u00" } + char_to_hex( c );
}

long long convert_to_int( const char* number, unsigned int length ) {
    return std::stoll( std::string( number, length) );
}

long double convert_to_float( const char* number, unsigned int length ) {
    return std::stold( std::string( number, length ) );
}

/************************************************************/

Number::Number(const Number& other ) :
        basic_json_value( Json_type::Number, other.parent ), str( other.str ), num_type( other.num_type ) {
    if ( Number_type::Integer == other.num_type ) {
        value.i = other.value.i;
    } else {
        value.f = other.value.f;
    }
}

Number::Number( const Number* other ) : Number( *other ) {
}

Number::Number( long long integer, basic_json_value* p ) :
        basic_json_value( Json_type::Number, p ), num_type( Number_type::Integer ) {
    this->str = std::to_string( integer );
    this->value.i = integer;
}

Number::Number( long double value, basic_json_value* p ) :
        basic_json_value( Json_type::Number, p ), num_type( Number_type::Float )  {
    this->str = std::to_string( value ); // TODO: fix precision
    this->value.f = value;
}

Number::Number( const std::string& value, basic_json_value* p ) :
        basic_json_value( Json_type::Number, p )  {
    this->str = value;
    // TODO: run through serializer?
    // TODO: check against standard if it's a float or integer number
    // TODO: set num_type correctly
    num_type = Number_type::Float;
    this->value.f = std::stold( value );
}

long long Number::get_integer() const {
    if ( Number_type::Integer == num_type ) {
        return value.i;
    } else {
        // TODO: throw?
        return (long long) get_float();
    }
}

long double Number::get_float() const {
    if ( Number_type::Float == num_type ) {
        return value.f;
    } else {
        // TODO: throw?
        return (long double) get_integer();
    }
}

String::String( const String& other ) :
        basic_json_value( Json_type::String, other.parent ) {
    this->string =  other.string;
}

String::String( const String* other ) : String( *other ) {
}

String::String( const std::string& str,  basic_json_value* p ) :
        basic_json_value( Json_type::String, p ) {
    this->string = str;
}

String::String( const std::string* str, basic_json_value* p ) : String( *str, p ) {
}

std::string String::to_json() const {
    // TODO, feed through json serializer
    return string;
}

Array::Array( Array&& other ) :
        basic_json_value( Json_type::Array, other.parent ), array( std::move( other.array ) ) {
}

Array::Array( const Array& other ) :
        basic_json_value( Json_type::Array, other.parent ), array() {
    for( std::size_t i{ 0 }; i < other.array.size(); ++i ) {
        array.push_back( other.array[i]->copy() );
    }
}

Array::Array( const Array* other ) : Array( *other ) {
}

std::string Array::to_json() const {
    // TODO
    return "";
}

std::unique_ptr<basic_json_value> Array::copy() const {
    Array* a{ new Array( this->parent ) };
    for( std::size_t i{ 0 }; i < array.size(); ++i ) {
        a->array.push_back( array[i]->copy() );
    }
    return std::unique_ptr<basic_json_value>( a );
}

basic_json_value& Array::at( std::size_t pos ) {
    return *array.at( pos ).get();
}

const basic_json_value& Array::at( std::size_t pos ) const {
    return *array.at( pos ).get();
}

basic_json_value& Array::operator[]( std::size_t pos ) {
    return *array[pos].get();
}

const basic_json_value& Array::operator[]( std::size_t pos ) const {
    return *array[pos].get();
}

bool Array::empty() const {
    return array.empty();
}

std::size_t Array::size() const {
    return array.size();
}

void Array::clear() {
    array.clear();
}

void Array::erase( std::size_t pos ) {
    array.erase( array.begin() + pos );
}

void Array::push_back( std::unique_ptr<basic_json_value> value ) {
    array.push_back( std::move( value ) );
}

/*
void Array::emplace_back( basic_json_value* value ) {
    array.emplace_back( value );
}
//*/

Object::Object( Object&& other ) :
        basic_json_value( Json_type::Object, other.parent ), object( std::move( other.object ) ) {
}

Object::Object( const Object& other ) :
        basic_json_value( Json_type::Object, other.parent ), object() {
    // using IT_TYPE = std::map<String, std::unique_ptr<basic_value, String::compare>>::const_iterator;
    for( auto it{ other.object.cbegin()}; it != other.object.cend(); ++it) {
        object[ it->first ] = it->second->copy();
    }
}

Object::Object( const Object* other ) : Object( *other ) {
}

std::string Object::to_json() const {
    // TODO
    return "";
}

std::unique_ptr<basic_json_value> Object::copy() const {
    Object* o{ new Object( this->parent ) };
    for( auto it{ object.cbegin()}; it != object.cend(); ++it) {
        o->object[ it->first ] = it->second->copy();
    }
    return std::unique_ptr<basic_json_value>( o );
}

basic_json_value& Object::at( const String& key ) {
    return *object.at( key ).get();
}

basic_json_value& Object::at( const String* key ) {
    return at( *key );
}

basic_json_value& Object::at( const std::string& key ) {
    return at( String( key ) );
}

const basic_json_value& Object::at( const String& key ) const {
    return *object.at( key ).get();
}

const basic_json_value& Object::at( const String* key ) const {
    return at( *key );
}

const basic_json_value& Object::at( const std::string& key) const {
    return at( String( key ) );
}

basic_json_value& Object::operator[]( const String& key ) {
    return *object[ key ].get();
}

basic_json_value& Object::operator []( const std::string& key ) {
    return *object[ String( key ) ].get();
}

bool Object::empty() const {
    return object.empty();
}

std::size_t Object::size() const {
    return object.size();
}

void Object::clear() {
    object.clear();
}

void Object::erase( const String& key ) {
    object.erase( key );
}

void Object::erase( const String* key ) {
    erase( *key );
}

void Object::insert( String key, std::unique_ptr<basic_json_value> value ) {
    object.insert( std::pair<String, std::unique_ptr<basic_json_value>>( key, std::move( value ) ) );
}

/*
void Object::emplace( const std::string& key, basic_json_value* value ) {
    object.emplace( key, std::unique_ptr<basic_json_value>( value ) );
}
//*/

/*****************************************************************************/

Value::Value() : type( Json_type::Error ) {
}

Value::Value( const Value& value ) {
    *this = value.copy();
}

Value::Value( Value&& value ) {
    data = std::move(value.data);
    array = std::move(value.array);
    object = std::move(value.object);
    type = value.type;
    is_int = value.is_int;
}

Value::Value( std::nullptr_t ) : type( Json_type::Null ) {

}

Value::Value( bool b ) {
    if ( b ) {
        type = Json_type::True;
    } else {
        type = Json_type::False;
    }
}

Value::Value( int i ) : type( Json_type::Number ) {
    is_int = true;
    data = std::to_string( i );
}

Value::Value( long long l ) : type( Json_type::Number ) {
    is_int = true;
    data = std::to_string( l );
}

Value::Value( long double d ) : type( Json_type::Number ) {
    is_int = false;
    data = std::to_string( d );
}

Value::Value( const std::string& str ) : type( Json_type::String ) {
    data = str;
}

Value::Value( const std::vector<Value>& arr ) : type( Json_type::Array ) {
    for ( const Value& v: arr ) {
        this->array.push_back( v.copy() );
    }
}

Value::Value( const std::map<std::string,Value>& obj ) : type( Json_type::Object ) {
    for ( const auto& p: obj ) {
        this->object.emplace( p.first, p.second.copy() );
    }
}

bool Value::get_bool() const {
    if ( !is_bool() ) {
        throw json_exception( "Not boolean" );
    }
    return is_true();
}

std::string Value::get_number() const {
    if ( !is_number() ) {
        throw json_exception( "Not a number" );
    }
    return data;
}

long double Value::get_float() const {
    if ( !is_number() ) {
        throw json_exception( "Not a number" );
    }
    return std::stold( data );
}

long long Value::get_integer() const {
    if ( !is_number() ) {
        throw json_exception( "Not a number" );
    }
    return std::stoll( data );
}
std::string Value::get_string() const {
    if ( !is_string() ) {
        throw json_exception( "Not a string" );
    }
    return data;
}

Value& Value::at( std::size_t i ) {
    if ( !is_array() ) {
        throw json_exception( "Not an array" );
    }
    return array.at( i );
}

const Value& Value::at( std::size_t i ) const {
    if ( !is_array() ) {
        throw json_exception( "Not an array" );
    }
    return array.at( i );
}

Value& Value::at( const std::string& key ) {
    if ( !is_object() ) {
        throw json_exception( "Not an object" );
    }
    return object.at( key );
}

const Value& Value::at( const std::string& key ) const {
    if ( !is_object() ) {
        throw json_exception( "Not an object" );
    }
    return object.at( key );
}

Value Value::copy() const {
    Value val{};
    val.type = type;
    val.is_int = is_int;

    val.data = data;
    for ( const Value& v: array ) {
        val.array.push_back( v.copy() );
    }
    for ( const auto& p: object ) {
        val.object.emplace( p.first, p.second.copy() );
    }

    return val;
}

Value& Value::operator=( const Value& value ) {
    this->type = value.type;
    this->is_int = value.is_int;
    this->data = value.data;
    array.clear();
    for ( const Value& v: value.array ) {
        this->array.push_back( v.copy() );
    }
    object.clear();
    for ( const auto& p: value.object ) {
        this->object.emplace( p.first, p.second.copy() );
    }
    return *this;
}

/*****************************************************************************/

std::shared_ptr<Event_receiver> Event_receiver::new_shared() {
    return std::make_shared<Event_receiver>( Event_receiver::Key() );
}

Event_receiver::~Event_receiver() {
    remove_transmitter();
}

/*
void Event_receiver::set_transmitter( std::shared_ptr<Event_transmitter> transmitter ) {
    remove_transmitter();
    this->transmitter = transmitter;
    transmitter->add_receiver( this );
}
//*/
void Event_receiver::set_transmitter( Event_transmitter* transmitter ) {
    remove_transmitter();
    this->transmitter = transmitter;
    if ( nullptr != transmitter ) {
        transmitter->add_receiver( this );
    }
}

/*
void Event_receiver::remove_transmitter() {
    if ( auto trans = transmitter.lock() ) {
        trans->remove_receiver( this );
    }
    transmitter.reset();
}
//*/
void Event_receiver::remove_transmitter() {
    if ( nullptr != transmitter ) {
        transmitter->remove_receiver( this );
        transmitter = nullptr;
    }
}

void Event_receiver::set_event_handler( Event_handler* handler ) {
    this->handler = handler;
}

void Event_receiver::event( Event e ) {
    if( nullptr != handler) {
        handler->event( e );
    }
}
/*
void Event_receiver::nullify_transmitter() {
    transmitter = nullptr;
}
//*/
std::shared_ptr<Event_transmitter> Event_transmitter::new_shared() {
    return std::make_shared<Event_transmitter>( Event_transmitter::Key() );
}

Event_transmitter::~Event_transmitter() {
    /*
    for ( recv_set_t::iterator it = receivers.begin(); it != receivers.end(); ++it ) {
        if ( recv_ptr_t recv = (*it).lock() ) {
            recv->remove_transmitter();
        }
    }
    //*/
    for ( Event_receiver* recv : receivers ) {
        //receivers.erase( recv );
        //recv->remove_transmitter();
        recv->set_transmitter( nullptr );
    }
}

/*
void Event_transmitter::add_receiver( recv_ptr_t receiver ) {
    receivers.emplace( receiver );
}
//*/

void Event_transmitter::add_receiver( Event_receiver* receiver ) {
    if ( nullptr != receiver ) {
        auto it = receivers.find( receiver );
        if ( it == receivers.end() ) {
            receivers.insert( receiver );
            receiver->set_transmitter( this );
        }
    }
}

/*
void Event_transmitter::remove_receiver( recv_ptr_t receiver ) {
    for( recv_set_t::iterator it{ receivers.begin() }; it != receivers.end(); ++it ) {
        if ( recv_ptr_t recv_ptr = (*it).lock() ) {
            if ( recv_ptr.get() == receiver.get() ) {
                receivers.erase( it );
            }
        } else {
            // Expired receiver
            receivers.erase( it );
        }
    }
}
//*/

void Event_transmitter::remove_receiver( Event_receiver* receiver ) {
    receivers.erase( receiver );
}

void Event_transmitter::event( Event e ) {
    /*
    for( recv_set_t::iterator it{ receivers.begin() }; it != receivers.end(); ++it ) {
        if ( recv_ptr_t recv_ptr = (*it).lock() ) {
            recv_ptr->event( e );
        } else {
            // Expired receiver
            receivers.erase( it );
        }
    }
    //*/
    for ( Event_receiver* receiver : receivers ) {
        receiver->event( e );
    }
}

/*****************************************************************************/

class json_generator : public Event_handler {
public:
    json_generator( const std::string& json );
    ~json_generator() = default;

    Value get_value();
    void event( Event ) override;

protected:
    std::deque<Value*> stack{};
    Value value{};
    bool Continue{ true };
    std::string key{};
    Deserialize d;
    std::stringstream ss;
    std::shared_ptr<JSON::Event_receiver> event_receiver;
};

json_generator::json_generator( const std::string& json ) : d{}, ss{ json }, event_receiver{ JSON::Event_receiver::new_shared() } {
    event_receiver->set_event_handler( this );
    d.set_istream( ss );
    d.add_event_receiver( event_receiver.get() );
}

Value json_generator::get_value() {
    while( Continue && d.read_istream() ){

    }
    return value;
}

void json_generator::event( Event e ) {
    if ( Event::Error == e ) {
        Continue = false;
        throw json_exception( "Invalid json input" );
    }

    if ( !stack.empty() ) {
        if ( stack.back()->is_array() ) {
            Value* arr = stack.back();
            switch ( e ) {
            case Event::Null:
                arr->array.push_back( Value( nullptr ) );
                d.pop();
                break;
            case Event::True:
                arr->array.push_back( Value( true ) );
                d.pop();
                break;
            case Event::False:
                arr->array.push_back( Value( false ) );
                d.pop();
                break;
            case Event::Integer:
                arr->array.push_back( Value( d.get_integer() ) );
                break;
            case Event::Float:
                arr->array.push_back( Value( d.get_double() ) );
                break;
            case Event::String_start:
                break;
            case Event::String_end:
                arr->array.push_back( Value( d.get_string() ) );
                break;
            case Event::Array_start:
                arr->array.push_back( Value( std::vector<Value>{} ) );
                d.pop();
                stack.push_back( &arr->array.back() );
                break;
            case Event::Array_end:
                d.pop();
                stack.pop_back();
                if ( stack.empty() ) {
                    Continue = false;
                }
                break;
            case Event::Object_start:
                arr->array.push_back( Value( std::map<std::string,Value>{} ) );
                d.pop();
                stack.push_back( &arr->array.back() );
                break;
            case Event::Object_end:
                Continue = false;
                throw json_exception( "Invalid json input" );
            default:
                Continue = false;
                throw json_exception( "Unhandled input case." );
            }
        } else if ( stack.back()->is_object() ) {
            if ( key.empty() ) {
                switch ( e ) {
                case Event::String_start:
                    break;
                case Event::String_end:
                    key = d.get_string();
                    break;
                case Event::Null:
                case Event::True:
                case Event::False:
                case Event::Integer:
                case Event::Float:
                case Event::Array_start:
                case Event::Array_end:
                case Event::Object_start:
                    Continue = false;
                    throw json_exception( "Invalid json input" );
                case Event::Object_end:
                    d.pop();
                    stack.pop_back();
                    if ( stack.empty() ) {
                        Continue = false;
                    }
                    break;
                default:
                    Continue = false;
                    throw json_exception( "Unhandled input case." );
                }
            } else {
                Value* obj{ stack.back() };
                switch ( e ) {
                case Event::Null:
                    obj->object.emplace( key, Value( nullptr ) );
                    key.clear();
                    d.pop();
                    break;
                case Event::True:
                    obj->object.emplace( key, Value( true ) );
                    key.clear();
                    d.pop();
                    break;
                case Event::False:
                    obj->object.emplace( key, Value( false ) );
                    key.clear();
                    d.pop();
                    break;
                case Event::Integer:
                    obj->object.emplace( key, Value( d.get_integer() ) );
                    key.clear();
                    break;
                case Event::Float:
                    obj->object.emplace( key, Value( d.get_integer() ) );
                    key.clear();
                    break;
                case Event::String_start:
                    break;
                case Event::String_end:
                    obj->object.emplace( key, Value( d.get_string() ) );
                    key.clear();
                    break;
                case Event::Array_start:
                    obj->object.emplace( key, Value( std::vector<Value>{} ) );
                    stack.push_back( &obj->object.at( key ) );
                    key.clear();
                    d.pop();
                    break;
                case Event::Array_end:
                    key.clear();
                    Continue = false;
                    throw json_exception( "Invalid json input" );
                case Event::Object_start:
                    obj->object.emplace( key, Value( std::map<std::string,Value>{} ) );
                    stack.push_back( &obj->object.at( key ) );
                    key.clear();
                    d.pop();
                    break;
                case Event::Object_end:
                    key.clear();
                    Continue = false;
                    throw json_exception( "Invalid json input" );
                default:
                    key.clear();
                    Continue = false;
                    throw json_exception( "Unhandled input case." );
                }
            }
        } else {
            Continue = false;
            throw json_exception( "Invalid json input" );
        }
    } else {
        switch ( e ) {
        case Event::Null:
            Continue = false;
            value = Value( nullptr );
            stack.push_back( &value );
            d.pop();
            break;
        case Event::True:
            Continue = false;
            value = Value( true );
            stack.push_back( &value );
            d.pop();
            break;
        case Event::False:
            Continue = false;
            value = Value( false );
            stack.push_back( &value );
            d.pop();
            break;
        case Event::Integer:
            Continue = false;
            value = Value( d.get_integer() );
            stack.push_back( &value );
            break;
        case Event::Float:
            Continue = false;
            value = Value( d.get_double() );
            stack.push_back( &value );
            break;
        case Event::String_start:
            break;
        case Event::String_end:
            Continue = false;
            value = Value( d.get_string() );
            stack.push_back( &value );
            break;
        case Event::Array_start:
            value = Value( std::vector<Value>{} );
            stack.push_back( &value );
            d.pop();
            break;
        case Event::Array_end:
            Continue = false;
            throw json_exception( "Invalid json input" );
        case Event::Object_start:
            value = Value( std::map<std::string,Value>{} );
            stack.push_back( &value );
            d.pop();
            break;
        case Event::Object_end:
            Continue = false;
            throw json_exception( "Invalid json input" );
        default:
            Continue = false;
            throw json_exception( "Unhandled input case." );
        }
    }
}

/*****************************************************************************/

bool Deserialize::put( const char c ) {
    return lexer_feed( c );
}

std::streamsize Deserialize::writesome( const char* buffer, std::streamsize length ) {
    std::streamsize index { 0 };
    while ( index < length && lexer_feed( buffer[ index ] ) )
        index++;
    return index;
}

bool Deserialize::read_istream() {
    return read_istream( this->is );
}

bool Deserialize::read_istream( std::istream& is ) {
    return read_istream( &is );
}

bool Deserialize::read_istream( std::istream* is ) {
    if( is == nullptr || !*is ) {
        // TODO: throw?
        return false;
    }
    char c;
    bool valid{ false };
    while( true ) {
        if ( is->get( c ) ) {
            if ( !lexer_feed( c ) ) {
                is->putback( c );
                break;
            } else {
                valid = true;
            }
        } else {
            break;
        }
    }
    /*
    while ( is->get( c ) && lexer_feed( c ) ) {
        valid = true;
    }
    //*/
    return valid;
}

/*
void Deserialize::add_event_receiver( Event_transmitter::recv_ptr_t receiver ) {
    event_transmitter->add_receiver( receiver );
}

void Deserialize::remove_event_receiver( Event_transmitter::recv_ptr_t receiver ) {
    event_transmitter->remove_receiver( receiver );
}
//*/
void Deserialize::add_event_receiver( Event_receiver* receiver ) {
    if ( event_transmitter ) {
        event_transmitter->add_receiver( receiver );
    }
}

void Deserialize::remove_event_receiver( Event_receiver* receiver ) {
    if ( event_transmitter ) {
        event_transmitter->remove_receiver( receiver );
    }
}

bool accept_value( basic_json_value* p ) {
    return nullptr == p ||
            Json_type::Array == p->get_type() ||
            Json_type::Object == p->get_type();
}

std::unique_ptr<basic_json_value> generate_new_json_value( Deserialize* ptr, basic_json_value* parent ) {
    using U = std::unique_ptr<basic_json_value>;
    using T = Json_type;

    std::streamsize len;
    T type = ptr->get_type();

    switch( type ) {
    case T::Null:
        return U( new Null( parent ) );
    case T::True:
        return U( new True( parent ) );
    case T::False:
        return U( new False( parent ) );
    case T::Number:
        return U( new Number( ptr->get_number(), parent ) );
    case T::String:
        // TODO: remove string size limit
        char buffer[100000];
        len = ptr->readsome( buffer, 100000 );
        return U( new String( std::string( buffer, len ), parent ) );
    case T::Array:
        return U( new Array( parent ) );
    case T::Object:
        return U( new Object( parent ) );
    default:
        return U( nullptr );
    }
}
/*
void add_value( basic_json_value* val ) {
    basic_json_value* parent{ val->parent };
    if ( nullptr != parent ) {
        switch( parent->get_type() ) {
        case Json_type::Array:
            static_cast<Array*>( parent )->emplace_back( val );
            return;
        case Json_type::Object:
            static_cast<Object*>
        }
    }
}
//*/

inline void parent_up( basic_json_value* parent ) {
    if ( nullptr != parent ) {
        parent = parent->parent;
    }
}

std::unique_ptr<String> cast_to_uni_string( std::unique_ptr<basic_json_value> val ) {
    if( val->is_string() ) {
        return std::unique_ptr<String>( static_cast<String*>( val.release() ) );
    } else {
        // TOOD: throw error?
        return nullptr;
    }
}

std::unique_ptr<Array> cast_to_uni_array( std::unique_ptr<basic_json_value> val ) {
    if( val->is_array() ) {
        return std::unique_ptr<Array>( static_cast<Array*>( val.release() ) );
    } else {
        // TOOD: throw error?
        return nullptr;
    }
}

std::unique_ptr<Object> cast_to_uni_object( std::unique_ptr<basic_json_value> val ) {
    if( val->is_object() ) {
        return std::unique_ptr<Object>( static_cast<Object*>( val.release() ) );
    } else {
        // TOOD: throw error?
        return nullptr;
    }
}

Value Deserialize::generate_json_value(const std::string& json ) {
    json_generator gen{ json };
    return gen.get_value();
}

bool Deserialize::is_complete() {
    if ( !state.empty() && state.top() == Lexer_state::Complete ) {
        return true;
    } else {
        return false;
    }
}

Json_type Deserialize::get_type() {
    // TODO: Consider a redesign that is less complex/error prone.
    // Currently both the state and type has to be check to make sure
    // that the lexer is in a valid state.
    if ( !state.empty() &&  state.top() == Lexer_state::Error ) {
        return Json_type::Error;
    }

    switch(current_type) {
    case Internal_type::Error:
        return Json_type::Error;
    case Internal_type::Null:
        return Json_type::Null;
    case Internal_type::True:
        return Json_type::True;
    case Internal_type::False:
        return Json_type::False;
    case Internal_type::Integer:
    case Internal_type::Float:
        return Json_type::Number;
    case Internal_type::String:
        return Json_type::String;
    case Internal_type::Array:
        return Json_type::Array;
    case Internal_type::Object:
        return Json_type::Object;
    default:
        return Json_type::Error;
    }
}

bool Deserialize::is_null() {
    return current_type == Internal_type::Null;
}

bool Deserialize::is_bool() {
    return current_type == Internal_type::True || current_type == Internal_type::False;
}

bool Deserialize::is_true() {
    return current_type == Internal_type::True;
}

bool Deserialize::is_false() {
    return current_type == Internal_type::False;
}

bool Deserialize::is_number() {
    return current_type == Internal_type::Integer || current_type == Internal_type::Float;
}

bool Deserialize::is_float() {
    return current_type == Internal_type::Float;
}

bool Deserialize::is_integer() {
    return current_type == Internal_type::Integer;
}

bool Deserialize::is_string() {
    return current_type == Internal_type::String;
}

bool Deserialize::is_array() {
    return current_type == Internal_type::Array;
}

bool Deserialize::is_object() {
    return current_type == Internal_type::Object;
}

bool Deserialize::pop() {
    if ( state.empty() ) {
        return false;
    }

    if ( state.top() == Lexer_state::Complete ||
            state.top() == Lexer_state::Needs_consuming ) {
        state.pop(); // pop Complete/Needs_consuming
        state.pop(); // pop what was completed.
        return true;
    } else {
        return false;
    }
    // TODO: else throw?
}

char Deserialize::peek() {
    if ( output_buffer_index > 0 && current_type == Internal_type::String ) {
        return output_buffer[ output_buffer_index ];
    }
    return 0;
}

char Deserialize::get() {
    // TODO: need to change buffer type before this can work correctly.
    // E.g. by using std::deque or some kind of fixed size ring buffer.
    if ( output_buffer_index == 1 ) {
        output_buffer_index = 0;
        return output_buffer[0];
    }
    return 0;
}

std::streamsize Deserialize::readsome( char* buffer, std::streamsize max_length ) {
    // TODO: redesign the output buffer, then change this
    if ( max_length < output_buffer_index || output_buffer_index == 0 ) {
        return 0;
    } else {
        const std::streamsize n = output_buffer_index;
        std::copy_n( output_buffer.cbegin(), n, buffer );
        output_buffer_index = 0;
        return n;
    }
}

std::string Deserialize::get_string() {
    if ( !state.empty() && state.top() == Lexer_state::Complete ) {
        if ( current_type == Internal_type::String ) {
            std::string ret{ output_buffer.cbegin(), output_buffer.cbegin() + output_buffer_index };
            output_buffer_index = 0;
            pop();
            return ret;
        }
    }
    // TODO: throw?
    return "";
}

bool Deserialize::get_bool() {
    if ( !state.empty() && state.top() == Lexer_state::Complete ) {
        if ( current_type == Internal_type::True ) {
            pop();
            return true;
        } else if (current_type == Internal_type::False ) {
            pop();
            return false;
        }
    }
    // TODO: throw?
    return false;
}

std::string Deserialize::get_number() {
    std::string number{ input_buffer.data(), input_buffer_length };
    pop();
    return number;
}

long long Deserialize::get_integer() {
    if ( !state.empty() && state.top() == Lexer_state::Complete ) {
        if ( current_type == Internal_type::Integer ) {
            pop();
            return convert_to_int( input_buffer.data(), input_buffer_length );
        }
    }
    // TODO: throw?
    return 0;
}

long double Deserialize::get_double() {
    if ( !state.empty() && state.top() == Lexer_state::Complete ) {
        if ( current_type == Internal_type::Float ) {
            long double f{ convert_to_float( input_buffer.data(), input_buffer_length ) };
            pop();
            return f;
        }
    }
    return 0.0;
}

void Deserialize::move_output_buffer_to_os() {
    if ( output_buffer_index != 0 && os != nullptr ) {
        os->write( output_buffer.data(), output_buffer_index );
        output_buffer_index = 0;
    }
}

bool Deserialize::lexer_output( const char c ) {
    if ( os != nullptr ) {
        move_output_buffer_to_os();
        os->put( c );
        return true;
    }

    if ( output_buffer_index > output_buffer.size() ) {
        // Drop buffer
        output_buffer_index = 0;
    }
    output_buffer.at( output_buffer_index++ ) = c;
    if ( output_buffer_index >= output_buffer.size() ) {
        return false;
    }
    return true;
}

bool Deserialize::lexer_feed( const char c ) {
    using S = Lexer_state;
    if( state.empty() ) {
        // If there are no nested states we are in the root,
        // expect a json value next.
        state.push( S::Value );
    }

    switch( state.top() ) {
    case S::Error:
        event_transmitter->event( Event::Error );
        return false;
        // TODO
    case S::Complete:
    case S::Needs_consuming:
        return false;
    case S::Null:
        return lexer_null( c );
    case S::True:
        return lexer_true( c );
    case S::False:
        return lexer_false( c );
    case S::Number:
        return lexer_number( c );
    case S::Value:
        return lexer_value( c );
    case S::String:
        return lexer_string( c );
    case S::S_escape:
        return lexer_escape( c );
    case S::S_unicode0:
    case S::S_unicode1:
    case S::S_unicode2:
    case S::S_unicode3:
        return lexer_unicode( c );
    case S::Array:
    case S::A_start_needs_consuming: // TODO: should never get here. throw?
    case S::A_elements:
        return lexer_array( c );
    case S::Object:
    case S::O_start_needs_consuming: // TODO: should never get here. throw?
    case S::O_expect_key_string:
    case S::O_expect_colon:
    case S::O_value:
        return lexer_object( c );
    default:
        // TODO: throw?
        state.push( S::Error );
        event_transmitter->event( Event::Error );
        return false;
    }

    // Should never get here
    // TODO: throw?
    state.push( S::Error );
    event_transmitter->event( Event::Error );
    return false;
}

bool Deserialize::lexer_array( const char c ) {
    using S = Lexer_state;
    const S s = state.top();

    if ( s == S::Array ) {
        if ( can_skip( c ) ) {
            return true;
        } else if ( c == ']' ) {
            // Empty array
            //state.pop();
            //return false;
            current_type = Internal_type::Array;
            state.push( S::Complete );
            event_transmitter->event( Event::Array_end );
            return true;
        } else {
            state.top() = S::A_elements;
            state.push( S::Value);
            return lexer_value( c );
        }
    } else if ( s == S::A_elements ) {
        if ( can_skip( c ) ) {
            return true;
        } else if ( c == ']' ) {
            //state.pop();
            //return false;
            current_type = Internal_type::Array;
            state.push( S::Complete );
            event_transmitter->event( Event::Array_end );
            return true;
        } else if ( c == ',' ) {
            state.push( S::Value );
            return true;
        }
    }

    // TODO: throw?
    state.push( S::Error );
    event_transmitter->event( Event::Error );
    return false;
}

bool Deserialize::lexer_escape( const char c ) {
    char translated_char{};
    switch (c) {
    case 'u':
        state.top() = Lexer_state::S_unicode0;
        return true;
    case 'b':
        translated_char = 0x08;
        break;
    case 'f':
        translated_char = 0x0c;
        break;
    case 'n':
        translated_char = 0x0a;
        break;
    case 'r':
        translated_char = 0x0d;
        break;
    case 't':
        translated_char = 0x09;
        break;

    case '"':
    case '\\':
    case '/':
        // All of these just should just be passed along
        translated_char =  c;
        break;
    default:
        state.push( Lexer_state::Error );
        event_transmitter->event( Event::Error );
        return false;
    }

    state.pop();
    return lexer_output( translated_char );
}

bool Deserialize::lexer_false( const char c ) {
    switch( keyword_state ) {
    case 'f':
        if ( c == 'a') {
            keyword_state = c;
            return true;
        }
        break;
    case 'a':
        if ( c == 'l' ) {
            keyword_state = c;
            return true;
        }
        break;
    case 'l':
        if ( c == 's' ) {
            keyword_state = c;
            return true;
        }
        break;
    case 's':
        if ( c == 'e' ) {
            keyword_state = 0;
            //state.pop();
            //return false;
            state.push( Lexer_state::Complete );
            event_transmitter->event( Event::False );
            return true;
        }
        break;
    }

    keyword_state = 0;
    state.push( Lexer_state::Error );
    event_transmitter->event( Event::Error );
    return false;
}

bool Deserialize::lexer_null( const char c ) {
    switch( keyword_state ) {
    case 'n':
        if ( c == 'u') {
            keyword_state = c;
            return true;
        }
        break;
    case 'u':
        if ( c == 'l' ) {
            keyword_state = c;
            return true;
        }
        break;
    case 'l':
        if ( c == 'l' ) {
            keyword_state = 0;
            //state.pop();
            //return false;
            state.push( Lexer_state::Complete );
            event_transmitter->event( Event::Null );
            return true;
        }
        break;
    }

    keyword_state = 0;
    state.push( Lexer_state::Error );
    event_transmitter->event( Event::Error );
    return false;
}

bool Deserialize::lexer_number( const char c ) {
    using S = Lexer_number_state;

    auto save_true = [c, this]()->bool {
        this->input_buffer.at( this->input_buffer_length++ ) = c;
        return true;
    };

    // Number does not have a terminating char, therefore this extra check is needed;
    if ( is_op( c ) || can_skip( c ) ) {
        if ( number_state == S::Zero ||
                number_state == S::Digit ||
                number_state == S::Frac ||
                number_state == S::E
                ) {
            state.push( Lexer_state::Complete );
            if ( is_integer() ) {
                event_transmitter->event( Event::Integer );
            } else if ( is_float() ) {
                event_transmitter->event( Event::Float );
            } else {
                event_transmitter->event( Event::Error );
            }
        } else {
            number_state = S::Error;
            state.push( Lexer_state::Error );
            event_transmitter->event( Event::Error );
        }
        return false;
    }

    switch ( number_state ) {
    case S::Start:
        input_buffer_length = 0;
        if ( is_sign( c ) ) {
            number_state = S::Sign;
            return save_true();
        } else if ( c == '0') {
            number_state = S::Zero;
            return save_true();
        } else if ( is_digit( c ) ) {
            number_state = S::Digit;
            return save_true();
        }
        break;
    case S::Sign:
        if ( c == '0' ) {
            number_state = S::Zero;
            return save_true();
        } else if ( is_digit( c ) ) {
            number_state = S::Digit;
            return save_true();
        }
        break;
    case S::Zero:
        if (c == '0' ) {
            number_state = S::Error;
        } else if (  c == '.' ) {
            number_state = S::Dot;
            return save_true();
        }
        // TODO: termination
        break;
    case S::Digit:
        if ( is_digit( c ) ) {
            return save_true();
        } else if ( c == '.' ) {
            number_state = S::Dot;
            return save_true();
        } else if ( is_exp( c ) ) {
            number_state = S::E_start;
            return save_true();
        }
        // TODO: termination
        break;
    case S::Dot:
        if ( is_digit( c ) ) {
            current_type = Internal_type::Float;
            number_state = S::Frac;
            return save_true();
        }
        break;
    case S::Frac:
        current_type = Internal_type::Float;
        if ( is_digit( c ) ) {
            return save_true();
        } else if ( is_exp( c ) ) {
            number_state = S::E_start;
            return save_true();
        }
        // TODO: termination
        break;
    case S::E_start:
        current_type = Internal_type::Float;
        if ( is_sign( c ) || is_digit( c ) ) {
            number_state = S::E;
            return save_true();
        }
        break;
    case S::E:
        if ( is_digit( c ) ) {
            return save_true();
        }
        // TODO: termination
        break;
    default:
        break;
    }

    // TODO: throw?
    number_state = S::Error;
    state.push( Lexer_state::Error );
    event_transmitter->event( Event::Error );
    return false;
}

bool Deserialize::lexer_object( const char c ) {
    using S = Lexer_state;
    const S s = state.top();

    if ( s == S::Object ) {
        if ( can_skip( c ) ) {
            return true;
        } else if ( c == '}' ) { // Empty object
            //state.pop();
            //return false;
            current_type = Internal_type::Object;
            state.push( S::Complete );
            event_transmitter->event( Event::Object_end );
            return true;
        } else if ( c == '"' ) { // Key
            state.push( S::O_expect_colon );
            state.push( S::String );
            current_type = Internal_type::String;
            event_transmitter->event( Event::String_start );
            return true;
        }
    } else if ( s == S::O_expect_key_string ) {
        if ( can_skip( c ) ) {
            return true;
        } else if ( c == '"' ) {
            state.top() = S::O_expect_colon;
            state.push( S::String );
            current_type = Internal_type::String;
            event_transmitter->event( Event::String_start );
            return true;
        }
    } else if ( s == S::O_expect_colon ) {
        if ( can_skip( c ) ) {
            return true;
        } else if ( c == ':' ) {
            state.top() = S::O_value;
            state.push( S::Value );
            return true;
        }
    } else if ( s == S::O_value ) {
        if ( can_skip( c ) ) {
            return true;
        } else if ( c == '}' ) {
            //state.pop();
            //return false;
            current_type = Internal_type::Object;
            state.top() = S::Complete;
            event_transmitter->event( Event::Object_end );
            return true;
        } else if ( c == ',' ) {
            state.top() = S::O_expect_key_string;
            return true;
        }
    }

    // TODO: throw?
    state.push( S::Error );
    event_transmitter->event( Event::Error );
    return false;
}

bool Deserialize::lexer_string( const char c ) {
    if ( c <= 0x1f ) { // Control char is not legal in JSON string
        state.push( Lexer_state::Error );
        event_transmitter->event( Event::Error );
        // TODO: throw.
        return false;
    } else if ( c == '\\' ) {
        state.push( Lexer_state::S_escape );
        return true;
    } else if ( c == '"' ) { // End of string
        //state.pop();
        //return false;
        state.push( Lexer_state::Complete );
        event_transmitter->event( Event::String_end );
        return true;
    }

    lexer_output( c );
    return true;
}

bool Deserialize::lexer_true( const char c ) {
    switch( keyword_state ) {
    case 't':
        if ( c == 'r') {
            keyword_state = c;
            return true;
        }
        break;
    case 'r':
        if ( c == 'u' ) {
            keyword_state = c;
            return true;
        }
        break;
    case 'u':
        if ( c == 'e' ) {
            keyword_state = 0;
            //state.pop();
            //return false;
            state.push( Lexer_state::Complete );
            event_transmitter->event( Event::True );
            return true;
        }
        break;
    }

    keyword_state = 0;
    state.push( Lexer_state::Error );
    event_transmitter->event( Event::Error );
    return false;
}

bool Deserialize::lexer_unicode( const char c ) {
    // TODO: only handles 1-byte unicode at the moment.
    switch ( state.top() ) {
    case Lexer_state::S_unicode0:
        if ( c != '0' ) {
            state.push( Lexer_state::Error );
            event_transmitter->event( Event::Error );
            return false;
        } else {
            state.top() = Lexer_state::S_unicode1;
            return true;
        }
    case Lexer_state::S_unicode1:
        if ( c != '0' ) {
            state.push( Lexer_state::Error );
            event_transmitter->event( Event::Error );
            return false;
        } else {
            state.top() = Lexer_state::S_unicode2;
            return true;
        }
    case Lexer_state::S_unicode2:
        if ( is_hex(c) ) {
            unicode_buffer = hex_to_char( c ) << 4;
        } else {
            // TODO: throw
            state.push( Lexer_state::Error );
            event_transmitter->event( Event::Error );
            return false;
        }
        state.top() = Lexer_state::S_unicode3;
        return true;
    case Lexer_state::S_unicode3:
        if ( is_hex( c ) ) {
            unicode_buffer |= hex_to_char( c );
        } else {
            // TODO: throw
            state.push( Lexer_state::Error );
            event_transmitter->event( Event::Error );
            return false;
        }
        lexer_output( unicode_buffer );
        state.pop();
        return true;
    default:
        state.push( Lexer_state::Error );
        event_transmitter->event( Event::Error );
        return false;
    }
}

bool Deserialize::lexer_value( const char c ) {
    using S = Lexer_state;

    if ( can_skip( c ) ) {
        return true;
    } else if ( is_start_json_number( c ) ) {
        current_type = Internal_type::Integer;
        state.top() = S::Number;
        number_state = Lexer_number_state::Start;
        return lexer_number( c );
    } else if ( c == '"' ) {
        current_type = Internal_type::String;
        state.top() = S::String;
        event_transmitter->event( Event::String_start );
        return true;
    } else if ( c == '{' ) {
        current_type = Internal_type::Object;
        state.top() = S::Object;
        state.push( S::O_start_needs_consuming );
        state.push( S::Needs_consuming );
        //state.push( S::Complete );
        // Do not start fetching elements at once.
        // Letting the user know what it is if they call get_type()
        event_transmitter->event( Event::Object_start );
        return true;
    } else if ( c == '[' ) {
        current_type = Internal_type::Array;
        state.top() = S::Array;
        state.push( S::A_start_needs_consuming );
        state.push( S::Needs_consuming );
        //state.push( S::Complete );
        // Do not start fetching elements at once.
        // Letting the user know what it is if they call get_type()
        event_transmitter->event( Event::Array_start );
        return true;
    } else if ( c == 't' ) {
        current_type = Internal_type::True;
        state.top() = S::True;
        keyword_state = c;
        return true;
    } else if ( c == 'f' ) {
        current_type = Internal_type::False;
        state.top() = S::False;
        keyword_state = c;
        return true;
    } else if ( c == 'n') {
        current_type = Internal_type::Null;
        state.top() = S::Null;
        keyword_state = c;
        return true;
    }

    current_type = Internal_type::Error;
    state.push( S::Error );
    event_transmitter->event( Event::Error );
    // TODO: throw?
    return false;
}

/*********************************************************/

Serialize::String_state::String_state( std::function<void(char)> output ) :
        State( Json_type::String, false), output( output ) {
    output( '"' );
}

bool Serialize::String_state::close() {
    if( closed ) {
        return false;
    }

    if( this->output ) {
        this->output( '"' );
    }

    closed = true;
    return true;
}

Serialize::Array_state::Array_state( std::function<void(char)> output ) :
        State( Json_type::Array, true ), output( output ) {
    output( '[' );
}

bool Serialize::Array_state::create_value() {
    if( first ) {
        first = false;
    } else {
        if( this->output ) {
            this->output( ',' );
        }
    }
    return true;
}

bool Serialize::Array_state::close() {
    if ( closed ) {
        return false;
    }

    if ( this->output ) {
        this->output( ']' );
    }

    closed = true;
    return true;
}

Serialize::Object_state::Object_state( std::function<void(char)> output ) :
        State( Json_type::Object, false ), output( output ) {
    output( '{' );
}

bool Serialize::Object_state::create_value() {
    if ( !this->expect_value ) {
        // TODO: error
        return false;
    }

    if( output ) {
        output( ':' );
    }
    this->expect_value = false;
    return true;
}

bool Serialize::Object_state::create_string() {
    if( !this->accept_value() ) {
        if( first ) {
            first = false;
        } else {
            if( output ) {
                this->output( ',' );
            }
        }
        this->expect_value = true;
        return true;
    } else {
        return this->create_value();
    }
}

bool Serialize::Object_state::close() {
    if ( this->accept_close() ) {
        if ( this->output ) {
            this->output( '}' );
        }

        closed = true;
        return true;
    }

    return false;
}

bool Serialize::State_machiene::accept_close( const Json_type type ) {
    if( state.empty() || type != state.top()->get_type() ) {
        return false;
    } else {
        return state.top()->accept_close();
    }
}

bool Serialize::State_machiene::accept_value( const Json_type type ) {
    if( state.empty() ) {
        return true;
    } else {
        switch( type ) {
        case Json_type::Error:
            return false;
        case Json_type::String:
            return state.top()->accept_string();
        default:
            return state.top()->accept_value();
        }
    }
}

bool Serialize::State_machiene::create( const Json_type type ) {
    using T = Json_type;
    if( accept_value( type ) ) {
        switch ( type ) {
        case T::Array:
        case T::Object:
        case T::String:
        case T::Error:
            return false;
        default:
            if( !state.empty() ) {
                return state.top()->create_value();
            }
            return true;
        }
    }
    return false;
}

bool Serialize::State_machiene::open( const Json_type type ) {
    using T = Json_type;
    switch( type ) {
    case T::String:
        if( state.empty() || state.top()->accept_string() ) {
            if( !state.empty() ) {
                state.top()->create_string();
            }
            state.emplace( new String_state{ output } );
            return true;
        } else {
            // TODO: Error
        }
        break;
    case T::Array:
        if( state.empty() || state.top()->accept_value() ) {
            if( !state.empty() ) {
                state.top()->create_value();
            }
            state.emplace( new Array_state{ output } );
            return true;
        } else {
            // TODO: Error
        }
        break;
    case T::Object:
        if( state.empty() || state.top()->accept_value() ) {
            if( !state.empty() ) {
                state.top()->create_value();
            }
            state.emplace( new Object_state{ output } );
            return true;
        } else {
            // TODO: Error
        }
        break;
    case T::False:
    case T::Null:
    case T::Number:
    case T::True:
        return false;
        /*
        if ( state.empty() || state.top().accept_value() ) {
            // OK, nothing todo
            return true;
        } else {
            // TODO: Error
        }
        //*/
        break;
    case T::Error:
    default:
        // TODO: Error
        break;
    }
    return false;
}

bool Serialize::State_machiene::close( const Json_type type ) {
    if ( state.empty() || this->type() != type ) {
        // TODO: Error
        return false;
    }
    if( state.top()->accept_close() ) {
        // TODO: state.close()
        if( state.top()->close() ) {
            state.pop();
            return true;
        } else {
            state.emplace( new Error_state{} );
            return false;
        }
    } else {
        // TODO: Error
        state.emplace( new Error_state{} );
    }
    return false;
}

Json_type Serialize::State_machiene::type() {
    if( state.empty() ) {
        // TODO: Error
        return Json_type::Error;
    } else {
        return state.top()->get_type();
    }
}

bool Serialize::State_machiene::is_valid() {
    return state.empty() || ( state.top()->get_type() != Json_type::Error );
}

bool Serialize::State_machiene::is_complete() {
    return state.empty();
}

void Serialize::State_machiene::clear() {
    while( !state.empty() ) {
        state.pop();
    }
}

/***************************************************************/

void Serialize::reset() {
    output_buffer.clear();
    state.clear();
}

bool Serialize::put( const char c ) {
    if ( Json_type::String != state.type() ) {
        // TDOO: Error
        return false;
    }

    if ( '\\' == c) {
        this->output( "\\\\" );
    } else if ( '"' == c ) {
        this->output( "\\\"" );
    } else if ( '\b' == c ) {
        this->output( "\\b" );
    } else if ( '\f' == c ) {
        this->output( "\\f" );
    } else if ( '\n' == c ) {
        this->output( "\\n" );
    } else if ( '\r' == c ) {
        this->output( "\\r" );
    } else if ( '\t' == c ) {
        this->output( "\\t" );
    } else if ( is_control( c ) ) {
        this->output( char_to_escaped_unicode( c ) );
    } else {
        this->output( c );
        return true;
    }

    return true;
}

bool Serialize::put( const char* const buffer, std::streamsize length ) {
    if ( start_string() ) {
        const char* end{ buffer + length };
        const char* char_ptr{ buffer };
        while ( char_ptr != end ) {
            put( *char_ptr++ );
        }
        if ( close_string() ) {
            return true;
        }
    }
    // TODO: Error handling
    return false;
}

bool Serialize::put( const std::string& string ) {
    return put( string.c_str(), string.length() );
}

bool Serialize::put( const char* cstr ) {
    return put( std::string( cstr ) );
}

bool Serialize::put( std::nullptr_t ) {
    return create_null();
}

bool Serialize::put( const bool b ) {
    if ( b ) {
        return create_true();
    } else {
        return create_false();
    }
}

bool Serialize::put( const long long i ) {
    return create_number( std::to_string( i ) );
}

bool Serialize::put( const long double d ) {
    // TODO: Evaluate this solution
    std::stringstream ss;
    ss << std::setprecision( std::numeric_limits<long double>::max_digits10 ) << d;
    return create_number( ss.str() );
}

bool Serialize::create_null() {
    if( state.create( Json_type::Null ) ) {
        output( "null" );
        return true;
    } else {
        // TODO: Error handling
        return false;
    }
}

bool Serialize::create_true() {
    if( state.create( Json_type::True ) ) {
        output( "true" );
        return true;
    } else {
        // TODO: Error
        return false;
    }
}

bool Serialize::create_false() {
    if( state.create( Json_type::False ) ) {
        output( "false" );
        return true;
    } else {
        // TODO: Error
        return false;
    }
}

bool Serialize::create_number( const std::string& number ) {
    if( state.create( Json_type::Number ) ) {
        // TODO: verify number before output
        output( number );
        return true;
    } else {
        // TODO: Error
        return false;
    }
}

bool Serialize::create_number( const char* number, std::streamsize length  ) {
    return create_number( std::string( number, length ) );
}

bool Serialize::start_string() {
    if( state.open( Json_type::String ) ) {
        return true;
    }
    // TODO: Error handling
    return false;
}

bool Serialize::close_string() {
    if( state.close( Json_type::String ) ) {
        return true;
    }
    // TODO: Error
    return false;
}

bool Serialize::start_array() {
    if( state.open( Json_type::Array ) ) {
        return true;
    }
    // TODO: Error
    return false;
}

bool Serialize::close_array() {
    if( state.close( Json_type::Array ) ) {
        return true;
    }
    // TODO: Error
    return false;
}

bool Serialize::start_object() {
    if( state.open( Json_type::Object ) ) {
        return true;
    }
    // TODO: Error
    return false;
}

bool Serialize::close_object() {
    if( state.close( Json_type::Object ) ) {
        return true;
    }
    // TODO: Error
    return false;
}

char Serialize::peek() {
    return output_buffer.front();
}

char Serialize::get() {
    char c{ output_buffer.front() };
    output_buffer.pop_front();
    return c;
}

bool Serialize::empty() {
    return output_buffer.empty();
}

std::streamsize Serialize::size() {
    return output_buffer.size();
}

std::streamsize Serialize::readsome( char* buffer, std::streamsize max_length ) {
    std::streamsize n{ std::min( max_length, (std::streamsize) output_buffer.size() ) };
    for( std::streamsize i{ 0 }; i < n; ++i ) {
        buffer[i] = output_buffer.front();
        output_buffer.pop_front();
    }
    return n;
}

bool Serialize::is_valid() {
    return state.is_valid();
}

bool Serialize::is_complete() {
    return state.is_complete();
}

void Serialize::set_ostream( std::ostream& os ) {
    set_ostream( &os );
}

void Serialize::set_ostream( std::ostream* os ) {
    this->os = os;
    if ( os != nullptr && !*os && !output_buffer.empty() ) {
        // TODO: verify that this removes the elements from the buffer.
        std::for_each( output_buffer.cbegin(), output_buffer.cend() ,
                [this]( const char c ) { this->os->put( c ); } );
    }
}

void Serialize::output( const char c ) {
    if( this->os != nullptr ) {
        /*
        // TOOD: move this to set_ostream
        if( !output_buffer.empty() ) {
            // TODO: verify that this removes the elements from the buffer.
            std::for_each( output_buffer.cbegin(), output_buffer.cend() ,
                    [this]( const char c ) { this->os->put( c ); } );
        }
        //*/
        os->put( c );
    } else {
        // TODO: limit buffer
        output_buffer.push_back( c );
    }
}

void Serialize::output( const char* cstr ) {
    output( std::string( cstr ) );
}

void Serialize::output( const char* data, const std::streamsize length ) {
    output( std::string( data, length ) );
}

void Serialize::output( const std::string& data ) {
    if( os != nullptr ) {
        std::copy_n( data.cbegin(), data.length(), std::ostreambuf_iterator<char>( *os ) );
    } else {
        for ( const char c : data ) {
            output( c );
        }
    }
}

} /* namespace JSON */
