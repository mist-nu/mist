/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef INCLUDE_JSONSTREAM_H_
#define INCLUDE_JSONSTREAM_H_

#include <array>
#include <cstddef> // std::nullptr_t
#include <deque>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <stack>
#include <string>
//#include <unordered_set>
#include <vector>

namespace JSON {

class json_exception : public std::runtime_error {
public:
    json_exception( const std::string& e ) : std::runtime_error( "JSON Exception: " + e ) {}
    virtual ~json_exception() = default;
};

enum class Json_type {
    Error,
    Null,
    True,
    False,
    Number,
    String,
    Array,
    Object
};

class basic_json_value {
public:
    basic_json_value( basic_json_value* p = nullptr ) : basic_json_value( Json_type::Error, p ) {}
    virtual ~basic_json_value() = default;

    virtual Json_type get_type() const { return type; }
    //*
    virtual bool is_null() const { return false; }
    virtual bool is_string() const { return false; }
    virtual bool is_array() const { return false; }
    virtual bool is_object() const { return false; }
    virtual bool is_true() const { return false; }
    virtual bool is_false() const { return false; }
    virtual bool is_bool() const { return false; }
    virtual bool is_number() const { return false; }
    virtual bool is_float() const { return false; }
    virtual bool is_integer() const { return false; }
    //*/

    virtual std::unique_ptr<basic_json_value> copy() const {
        return std::unique_ptr<basic_json_value>( new basic_json_value( type,  parent ) );
    }

    virtual std::string to_json() const { return std::string{ "ERROR" }; }
    basic_json_value* parent;

protected:
    basic_json_value( Json_type type, basic_json_value* p = nullptr ) : parent( p ), type( type ) {}
    Json_type type;

private:
    explicit basic_json_value( const basic_json_value& ) = delete;
    explicit basic_json_value( basic_json_value&& ) = delete;
};

class Null : public basic_json_value {
public:
    Null( basic_json_value* p = nullptr ) : basic_json_value( Json_type::Null, p ) {}
    virtual ~Null() = default;

    virtual bool is_null() const override { return true; }
    virtual std::string to_json() const override { return "null"; }

    virtual std::unique_ptr<basic_json_value> copy() const override {
        return std::unique_ptr<basic_json_value>( new Null( this->parent ) );
    }
};

class True : public basic_json_value {
public:
    True( basic_json_value* p = nullptr ) : basic_json_value( Json_type::True, p ) {}
    virtual ~True() = default;

    virtual bool is_true() const override { return true; }
    virtual bool is_bool() const override { return true; }
    virtual std::string to_json() const override { return "true"; }

    virtual std::unique_ptr<basic_json_value> copy() const override {
        return std::unique_ptr<basic_json_value>( new True( this->parent ) );
    }
};

class False : public basic_json_value {
public:
    False( basic_json_value* p = nullptr ) : basic_json_value( Json_type::False, p ) {}
    virtual ~False() = default;

    virtual bool is_false() const override { return true; }
    virtual bool is_bool() const override { return true; }
    virtual std::string to_json() const override { return "false"; }

    virtual std::unique_ptr<basic_json_value> copy() const override {
        return std::unique_ptr<basic_json_value>( new False( this->parent ) );
    }
};

class Number : public basic_json_value {
public:
    Number( const Number& other );
    Number( const Number* other );
    Number( long long integer, basic_json_value* p = nullptr );
    Number( long double value, basic_json_value* p = nullptr );
    Number( const std::string& value, basic_json_value* p = nullptr );
    virtual ~Number() = default;

    virtual bool is_number() const override { return true; }
    virtual bool is_integer() const { return Number_type::Integer == num_type; }
    virtual bool is_float() const { return Number_type::Float == num_type; }

    virtual long long get_integer() const;
    virtual long double get_float() const;

    virtual std::string to_json() const override { return str; }

    virtual std::unique_ptr<basic_json_value> copy() const override {
        return std::unique_ptr<basic_json_value>( new Number( this ) );
    }

protected:
    std::string str;
    enum class Number_type {
        Integer,
        Float
    } num_type;
    union Num {
        long long i;
        long double f;
    } value;
};

class String : public basic_json_value {
public:
    String( basic_json_value* p = nullptr ) : basic_json_value( Json_type::String, p ), string() {}
    String( const String& other );
    String( const String* other );
    String( const std::string& str, basic_json_value* p = nullptr );
    String( const std::string* str, basic_json_value* p = nullptr );
    virtual ~String() = default;

    virtual bool is_string() const override { return true; }
    virtual std::string to_json() const override;

    virtual std::unique_ptr<basic_json_value> copy() const override {
        return std::unique_ptr<basic_json_value>( new String( this ) );
    }

    struct compare {
        bool operator()( const String& lhs, const String& rhs ) const {
            return lhs.string.compare( rhs.string ) < 0;
        }

        bool operator()( const String* lhs, const String* rhs) const {
            return lhs->string.compare( rhs->string ) < 0;
        }

        bool operator()( const std::unique_ptr<String>& rhs, const std::unique_ptr<String>& lhs ) const {
            return lhs->string.compare( rhs->string ) < 0;
        }
    };

    using iterator = std::string::iterator;
    using const_iterator = std::string::const_iterator;

    iterator begin() { return string.begin(); }
    const_iterator cbegin() const { return string.cbegin(); }
    iterator end() { return string.end(); }
    const_iterator cend() const { return string.cend(); }

    std::string string;
};

class Array : public basic_json_value {
public:
    Array( basic_json_value* p = nullptr ) : basic_json_value( Json_type::Array, p ) {}
    Array( Array&& other );
    Array( const Array& other );
    Array( const Array* other );
    virtual ~Array() = default;

    virtual bool is_array() const override { return true; }
    virtual std::string to_json() const override;

    virtual std::unique_ptr<basic_json_value> copy() const override;

    basic_json_value& at( std::size_t pos );
    const basic_json_value& at( std::size_t pos ) const;
    basic_json_value& operator[]( std::size_t pos );
    const basic_json_value& operator[]( std::size_t pos ) const;

    using iterator = std::vector<std::unique_ptr<basic_json_value>>::iterator;
    using const_iterator = std::vector<std::unique_ptr<basic_json_value>>::const_iterator;

    iterator begin() { return array.begin(); }
    const_iterator cbegin() const { return array.cbegin(); }
    iterator end() { return array.end(); }
    const_iterator cend() const { return array.cend(); }

    bool empty() const;
    std::size_t size() const;

    void clear();
    void erase( std::size_t pos );
    void push_back( std::unique_ptr<basic_json_value> value ); // given ownership
    //void emplace_back( basic_json_value* value ); // takes ownership

protected:
    std::vector<std::unique_ptr<basic_json_value>> array;
};

class Object : public basic_json_value {
public:
    Object( basic_json_value* p = nullptr ) : basic_json_value( Json_type::Object, p ) {}
    Object( Object&& other );
    Object( const Object& other );
    Object( const Object* other );
    virtual ~Object() = default;

    virtual bool is_object() const override { return true; }
    virtual std::string to_json() const override;

    virtual std::unique_ptr<basic_json_value> copy() const override;

    basic_json_value& at( const String& key );
    basic_json_value& at( const String* key );
    basic_json_value& at( const std::string& key );
    const basic_json_value& at( const String& key ) const;
    const basic_json_value& at( const String* key ) const;
    const basic_json_value& at( const std::string& key ) const;
    basic_json_value& operator[]( const String& key );
    basic_json_value& operator[]( const std::string& key );

    using iterator = std::map<String, std::unique_ptr<basic_json_value>>::iterator;
    using const_iterator = std::map<String, std::unique_ptr<basic_json_value>>::const_iterator;

    iterator begin() { return object.begin(); }
    const_iterator cbegin() const { return object.cbegin(); }
    iterator end() { return object.end(); }
    const_iterator cend() const { return object.cend(); }

    bool empty() const;
    std::size_t size() const;

    void clear();
    void erase( const String& key );
    void erase( const String* key );
    void insert( String key, std::unique_ptr<basic_json_value> value );
    //void emplace( const std::string& key, basic_json_value* value ); // take ownership

protected:
    std::map<String, std::unique_ptr<basic_json_value>, String::compare> object;
};

/*****************************************************************************/

class Value {
public:
    Value();
    Value( const Value& value);
    Value( Value&& value);
    Value( std::nullptr_t );
    Value( bool b );
    Value( int i );
    Value( long long l );
    Value( long double d );
    Value( const std::string& str );
    Value( const std::vector<Value>& arr );
    Value( const std::map<std::string,Value>& obj );

    virtual ~Value() = default;

    virtual Json_type get_type() const { return type; }
    virtual bool is_null() const { return Json_type::Null == type; }
    virtual bool is_string() const { return Json_type::String == type; }
    virtual bool is_array() const { return Json_type::Array == type; }
    virtual bool is_object() const { return Json_type::Object == type; }
    virtual bool is_true() const { return Json_type::True == type; }
    virtual bool is_false() const { return Json_type::False == type; }
    virtual bool is_bool() const { return is_true() || is_false(); }
    virtual bool is_number() const { return Json_type::Number == type; }
    virtual bool is_float() const { return is_number() && !is_int; }
    virtual bool is_integer() const { return is_number() && is_int; }

    virtual bool get_bool() const;
    virtual std::string get_number() const;
    virtual long double get_float() const;
    virtual long long get_integer() const;
    virtual std::string get_string() const;

    virtual Value& at( std::size_t i );
    virtual const Value& at( std::size_t i ) const;
    virtual Value& at( const std::string& key );
    virtual const Value& at( const std::string& key ) const;

    Value copy() const;
    Value& operator=( const Value& value );

    // TODO: Hide this ugly quick hack
    std::string data{};
    std::vector<Value> array{};
    std::map<std::string,Value> object{};

protected:
    Json_type type;
    bool is_int{ false };
};


/*****************************************************************************/

enum class Event : int {
    Error = 0,
    Null,
    True,
    False,
    Integer,
    Float,
    String_start,
    String_end,
    Array_start,
    Array_end,
    Object_start,
    //Object_key,
    Object_end,
};
class Event_transmitter;

class Event_handler {
public:
    virtual ~Event_handler() {}
    virtual void event( Event ) = 0;
};

class Event_receiver {
public:
    static std::shared_ptr<Event_receiver> new_shared();
    class Key {
    private:
        friend std::shared_ptr<Event_receiver> Event_receiver::new_shared();
        Key() {}
    };

    Event_receiver( const Key& ) {};
    virtual ~Event_receiver();

    //virtual void set_transmitter( std::shared_ptr<Event_transmitter> transmitter );
    virtual void set_transmitter( Event_transmitter* transmitter );
    virtual void remove_transmitter();

    virtual void set_event_handler( Event_handler* handler );

    virtual void event( Event e );

protected:
    //std::weak_ptr<Event_transmitter> transmitter{};
    Event_transmitter* transmitter{ nullptr };
    Event_handler* handler{ nullptr };

private:
    //friend Event_transmitter;
    //void nullify_transmitter();
};

class Event_transmitter {
public:
    //using recv_set_t = std::set<std::weak_ptr<Event_receiver>,std::owner_less<std::weak_ptr<Event_receiver>>>;
    //using recv_ptr_t = std::shared_ptr<Event_receiver>;

    static std::shared_ptr<Event_transmitter> new_shared();
    class Key {
    private:
        friend std::shared_ptr<Event_transmitter> Event_transmitter::new_shared();
        Key() {}
    };

    Event_transmitter( const Key& ) {};
    virtual ~Event_transmitter();

    //virtual void add_receiver( recv_ptr_t receiver );
    //virtual void remove_receiver( recv_ptr_t receiver );
    virtual void add_receiver( Event_receiver* receiver );
    virtual void remove_receiver( Event_receiver* receiver );

    virtual void event( Event e );

protected:
    //std::unordered_set<Event_receiver*> receivers{};
    //std::unordered_set<std::weak_ptr<Event_receiver>*> receivers{};
    //recv_set_t receivers{};
    std::set<Event_receiver*> receivers{};
};

/***************************************************************/

class Deserialize {
    /*
     * TODO: Redesign needed, currently there are too many things to keep
     * track of when a change occur.
     * The states could be made into a separate class that is pushed to
     * a stack, and from the state the current type and what not could
     * be inferred. As it stands multiple things has to be changed in
     * multiple places when pushing and popping states. It is too easy
     * for a bug to occur at the moment.
     */

    /*
     * TODO: Redesign tip, implement events for handling different states in the json.
     */
public:
    virtual ~Deserialize() = default;
    // JSON in data
    bool put( const char c );
    std::streamsize writesome( const char* buffer, std::streamsize length );

    bool read_istream();
    bool read_istream( std::istream& is );
    bool read_istream( std::istream* is );
    void set_istream( std::istream& is ) { this->is = &is; }
    void set_istream( std::istream* is ) { this->is = is; }
    void unset_istream() { is = nullptr; }
    std::istream& get_istream() { return *is; }

    // JSON events e.g. Event::String_start or Event::Integer
    //void add_event_receiver( Event_transmitter::recv_ptr_t receiver );
    //void remove_event_receiver( Event_transmitter::recv_ptr_t receiver );
    void add_event_receiver( Event_receiver* receiver );
    void remove_event_receiver( Event_receiver* receiver );

    /*
    std::unique_ptr<basic_json_value> generate_json_value();
    static std::unique_ptr<basic_json_value> generate_json_value( std::istream& is );
    static std::unique_ptr<basic_json_value> generate_json_value( const std::string& json );
    //*/
    static Value generate_json_value( const std::string& json );

    // JSON tests
    bool is_complete();

    Json_type get_type();
    bool is_null();
    bool is_string();
    bool is_array();
    bool is_object();

    bool is_bool();
    bool is_true();
    bool is_false();

    bool is_number();
    bool is_float();
    bool is_integer();

    // Value out data
    bool pop();

    char peek();
    char get(); // will get next char from out buffer
    std::streamsize readsome( char* buffer, std::streamsize max_length ); // returns amount read.
    std::string get_string();

    bool get_bool();
    std::string get_number();
    long long get_integer();
    long double get_double();

    void set_ostream( std::ostream& os ) { this->os = &os; }
    void set_ostream( std::ostream* os ) { this->os = os; }
    void unset_ostream() { os = nullptr; }

// protected:
    enum class Lexer_state {
        Error,
        Complete,
        Needs_consuming,
        Null,
        True,
        False,
        Number,
        Value,
        // String
        String,
        S_escape,
        S_unicode0,
        S_unicode1,
        S_unicode2,
        S_unicode3,
        // Array
        Array,
        A_start_needs_consuming,
        A_elements,
        // Object
        Object,
        O_start_needs_consuming,
        O_expect_key_string,
        O_expect_colon,
        O_value,
    };

    enum class Lexer_number_state {
        Error,
        Start,
        Sign,
        Zero,
        Digit,
        Dot,
        Frac,
        E_start,
        E,
    };

    enum class Internal_type {
        Error,
        Null,
        True,
        False,
        Integer,
        Float,
        String,
        Array,
        Object,
    };

protected:
    //static std::unique_ptr<basic_json_value> generate_json_value( Deserialize* d );

    void move_output_buffer_to_os();

    bool lexer_output( const char c );
    bool lexer_feed( const char c ); // return true if it wants more data to finish reading current type

    bool lexer_array( const char c );
    bool lexer_escape( const char c );
    bool lexer_false( const char c );
    bool lexer_null( const char c );
    bool lexer_number( const char c );
    bool lexer_object( const char c );
    bool lexer_string( const char c );
    bool lexer_true( const char c );
    bool lexer_unicode( const char c );
    bool lexer_value( const char c );

    std::stack<Lexer_state> state{};
    Lexer_number_state number_state{};
    char keyword_state{};
    Internal_type current_type{ Internal_type::Error };

    //const static unsigned int input_buffer_size{ 1000 }; // TODO: consider setting this in ctor
    std::array<char,1000> input_buffer{}; // Used to ensure that we can parse complete numbers.
    unsigned int input_buffer_length{ 0 };

    //const static unsigned int output_buffer_size{ 1000 }; // TODO: consider setting this in ctor
    std::array<char,100000> output_buffer; // Used to ensure that we can parse complete numbers.
    unsigned int output_buffer_index{ 0 }; // TODO: refactor this to length

    char unicode_buffer{};

    std::istream* is{ nullptr };
    std::ostream* os{ nullptr };

    std::shared_ptr<Event_transmitter> event_transmitter{ Event_transmitter::new_shared() };
};

/***************************************************************/

class Serialize {
public:
    void reset();

    bool put_json_and_sort( const std::string& raw_json );
    bool put( const basic_json_value* json_value );

    // part of string
    std::streamsize writesome( const char* buffer, std::streamsize length );
    bool put( const char c );

    // Data to serialize
    // complete strings
    bool put( const char* string, std::streamsize length );
    bool put( const std::string& string );
    bool put( const char* cstr ); // c-string ( null-terminated )

    // Other types
    bool put( std::nullptr_t );
    bool put( const bool b );
    bool put( const long long i );
    bool put( const long double d ); // TODO: Make sure this conforms to the float standard selected

    bool create_null();
    bool create_true();
    bool create_false();

    // TODO: Validate number
    bool create_number( const std::string& number );
    bool create_number( const char* number, std::streamsize length ); // Takes a pre-formated JSON number

    bool start_string();
    bool close_string();
    bool start_array();
    bool close_array();
    bool start_object();
    bool close_object();

    void read_istream();
    void read_istream( std::istream& is );
    void read_istream( std::istream* is );
    void set_istream( std::istream& is ) { this->is = &is; }
    void set_istream( std::istream* is ) { this->is = is; }
    void unset_istream() { is = nullptr; }

    // JSON out
    char peek();
    char get();
    bool empty();
    std::streamsize size();

    std::streamsize readsome( char* buffer, std::streamsize max_length ); // returns amount read.

    bool is_valid();
    bool is_complete();

    void set_ostream( std::ostream& os );
    void set_ostream( std::ostream* os );
    void unset_ostream() { os = nullptr; }

//protected:
    class State {
    public:
        State( Json_type type, bool expect_value = false ) :
            type( type ), expect_value( expect_value ) {}
        virtual ~State() = default;

        virtual Json_type get_type() { return type; }

        virtual bool accept_close() { return false; }
        virtual bool accept_value() { return this->expect_value; }
        virtual bool accept_string() { return this->accept_value(); }

        virtual bool create_value() { return this->accept_value(); }
        virtual bool create_string() { return this->create_value(); }
        virtual bool close() { return this->accept_close(); }

    protected:
        Json_type type;
        bool expect_value;
    };

    class Error_state : public State {
    public:
        Error_state() :
            State( Json_type::Error, false ) {}
        virtual ~Error_state() = default;
    };

    class String_state : public State {
    public:
        String_state( std::function<void(const char)> output );
        virtual ~String_state() { close(); }

        virtual bool accept_close() override { return !closed; }
        virtual bool close() override;
    protected:
        std::function<void(char)> output;
        bool closed{ false };
    };

    class Array_state : public State {
    public:
        Array_state( std::function<void(const char)> output );
        virtual ~Array_state() { close(); }

        virtual bool accept_close() override { return !closed; }
        virtual bool create_value() override;
        virtual bool close() override;
    protected:
        std::function<void(char)> output;
        bool first{ true };
        bool closed{ false };
    };

    class Object_state : public State {
    public:
        Object_state( std::function<void(const char)> output );
        virtual ~Object_state() { close(); }

        virtual bool accept_close() override { return ( !closed && !this->expect_value ); }
        virtual bool accept_value() override { return ( !closed && this->expect_value ); }
        virtual bool accept_string() override { return !closed; }

        virtual bool create_value() override;
        virtual bool create_string() override;
        virtual bool close() override;
    protected:
        std::function<void(char)> output;
        bool first{ true };
        bool closed{ false };
    };

    class State_machiene {
    public:
        State_machiene( std::function<void(const char)> output ) : output( output ) {}
        virtual ~State_machiene() = default;

        virtual bool accept_close( const Json_type type );
        virtual bool accept_value( const Json_type type );

        virtual bool create( const Json_type type );
        virtual bool open( const Json_type type );
        virtual bool close( const Json_type type );
        virtual Json_type type();

        virtual bool is_valid();
        virtual bool is_complete();

        void clear();

    protected:
        std::stack<std::unique_ptr<State>> state{};
        std::function<void(char)> output;
    };

protected:
    void output( const char c );
    void output( const char* cstr );
    void output( const char* data, const std::streamsize length );
    void output( const std::string& data );

    std::deque<char> output_buffer{};
    State_machiene state{ [this]( const char c ) -> void { this->output( c ); } };
    std::istream* is { nullptr };
    std::ostream* os { nullptr };
};

} /* namespace JSON */


#endif /* INCLUDE_JSONSTREAM_H_ */
