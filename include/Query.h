/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef INCLUDE_QUERY_H_
#define INCLUDE_QUERY_H_

#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace Mist {

enum class TokenType {
    Identifier,

    Comma,
    Dot,

    LeftPara,
    RightPara,

    And,
    Or,
    Not,
    Equal,
    NotEqual,
    LessThan,
    GreaterThan,
    LessThanOrEqual,
    GreaterThanOrEqual,

    String,
    Number,
};

class Token {
private:
    const TokenType type;
    const std::string content;
    const int col;
    const int line;

public:
    Token( TokenType type, const std::string& content, int col, int line )  :
        type(type), content(content), col(col), line(line) {}
    TokenType getType() const { return type; }
    std::string getContent() const { return content; }
    int getCol() const { return col; }
    int getLine() const { return line; }
};

enum class ExpressionType {
    Identifier,
    String,
    Number,
    True,
    False,
    Null,

    Comma,
    Dot,

    And,
    Or,
    Not,
    Equal,
    NotEqual,
    LessThan,
    GreaterThan,
    LessThanOrEqual,
    GreaterThanOrEqual,

    FunctionCall,
};


class Expression {
    ExpressionType type;
    std::vector<Expression> args;
    std::string content;
    int col;
    int line;

public:
    Expression( ExpressionType type , const std::vector<Expression>& args, std::string content = {}, const Token *token = nullptr ) :
        type(type), args(args), content(content), col( token ? token->getCol() : 0 ), line( token ? token->getLine() : 0 ) {}
    const std::vector<Expression>& getArgs() const { return args; }
    ExpressionType getType() const { return type; }
    std::string getContent() const;
    int getCol() const { return col; }
    int getLine() const { return line; }
};

class QueryTokenizer {
public:
    static std::vector<Token> tokenize( const std::string& str );
    static void parseError( int line, int col );
    static int tokenizeString( const std::string& str, int i , std::vector<Token> &res, int line, int startCol );
    static int tokenizeIdentifier( const std::string& str, int i, std::vector<Token> &res, int line, int startCol );
    static bool whiteSpace( char c ) { return isspace(c); }
    static std::string trimNumber( std::string number );
};


class QueryParser {
private:
    std::vector<Token> tokens;
    unsigned index{0};

public:
    QueryParser( std::vector<Token>& tokens ) : tokens( tokens ) {}

    Expression commaExpression();
    Expression expression();
    Expression orExpression();
    Expression andExpression();
    Expression compareExpression();
    Expression unaryExpression();
    Expression functionExpression();
    Expression dotExpression();
    Expression identifierOrConstant();
    Expression identifier();
    Token* getNextToken( const std::vector<TokenType>& types = std::vector<TokenType>{} );

    Token lastToken();
    void parseError( const Token& token );

    static Expression parseExpression( std::vector<Token> tokens );
    static Expression parseCommaExpression( std::vector<Token> tokens );
};


class Select {
private:
    bool all{};
    std::string functionName{};
    std::string functionAttribute{};
    std::vector<std::string> attributes{};

public:
    Select() = default;
    void parse( std::string str );
    bool getAll() const { return all; }
    bool isFunctionCall() const { return !functionName.empty(); }
    const std::string& getFunctionName() const { return functionName; }
    const std::string& getFunctionAttribute() const { return functionAttribute; }
    const std::vector<std::string>& getAttributes() const { return attributes; }
};

enum Type {
    Typeless = 0,
    Null,
    Boolean,
    Number,
    String,
    JSON
};

template<class T>
class ValueType {
protected:
    std::string strVal;
    double numberVal;
    bool boolVal;
    T t;
public:
    ValueType( T type ) : strVal(), numberVal(), boolVal(), t( type ) {}
    ValueType( const std::string& value, T type ) : strVal( value ), numberVal(), boolVal(), t( type ) {}
    ValueType( double value, T type ) : strVal(), numberVal( value ), boolVal(), t( type ) {}
    ValueType( bool value, T type ) : strVal(), numberVal(), boolVal( value ), t( type ) {}
    ValueType( T type, const std::string& strVal, double numberVal, bool boolVal ) : strVal( strVal ), numberVal( numberVal ), boolVal( boolVal ),  t( type ) {}
    std::string stringValue() const { return strVal; }
    double numberValue() const { return numberVal; }
    bool boolValue() const { return boolVal; }
    T type() const { return t; }
};

class ArgumentVT : public ValueType<Type> {
public:
    ArgumentVT() : ValueType( Type::Typeless ) {}
    ArgumentVT( const std::string& value, bool json = false ) : ValueType<Type>( value, json?Type::JSON:Type::String ) {}
    ArgumentVT( double value ) : ValueType<Type>( value, Type::Number ) {}
    ArgumentVT( bool value ) : ValueType<Type>( value, Type::Boolean ) {}
    ArgumentVT( std::nullptr_t p ) : ValueType<Type>( Type::Null ) {}
    ArgumentVT( const ArgumentVT& other ) : ValueType<Type>( other.t, other.strVal, other.numberVal, other.boolVal ) {}
    bool operator<( const ArgumentVT& rhs ) const;
};


enum FilterExpressionType {
    NoType,
    UnaryFunction,
    BinaryFunction,
    Attribute,
    Argument,
    ConstBoolean,
    ConstNumber,
    ConstString,
    ConstNull
};

class FilterExpression {
    FilterExpressionType type;
    std::string _operator;
    FilterExpression *left;
    FilterExpression *right;

    std::string attribute;
    std::string argument;
    ArgumentVT value;

public:
    FilterExpression();
    ~FilterExpression();

    FilterExpressionType getType() const { return type; }
    std::string getOperator() const { return _operator; }
    FilterExpression *getLeft() const { return left; }
    FilterExpression *getRight() const { return right; }
    std::string getAttribute() const { return attribute; }
    std::string getArgument() const { return argument; }
    ArgumentVT getValue() const { return value; }

    bool attributeArgumentOrConstant() const;
    std::set<std::string> getAttributes();
    std::set<std::string> getArguments();
    std::set<ArgumentVT> getConstants();
    bool hasComparison();
    void parse( Expression expression );
    std::string makeSQL(
            const std::map<std::string,ArgumentVT> &args,
            const std::map<std::string,std::string> &argsIndex,
            const std::map<ArgumentVT,std::string> &constIndex );
};

class Query;

class Filter {
private:
    bool none;
    FilterExpression expression;

public:
    Filter();
    void parse( std::string str );
    void makeSQL( Query &res,
            const std::map<std::string,ArgumentVT> &args,
            int maxVersion,
            std::string status,
            bool versionsQuery );
};

class Sort {
    bool none{};
    std::string attribute{};
    bool desc{};

public:
    Sort() = default;
    //void parse( const Expression &expression );
    void parse( const std::string& str );
    bool getNone() const { return none; }
    bool getDesc() const { return desc; }
    std::string getAttribute() const { return attribute; }
};

class Query {
private:
    std::string sqlQuery{};
    std::vector<ArgumentVT> args{};
    Select select{};
    Filter filter{};
    Sort sort{};
    friend Select;
    friend Filter;
    friend Sort;

public:
    static std::string printArg( int i );

    void parseQuery( int accessDomain,
            long long parent,
            std::string selectStr,
            std::string filterStr,
            std::string sortStr,
            std::map<std::string,ArgumentVT> args,
            int maxVersion,
            bool includeDeleted );
    void parseVersionQuery( int accessDomain,
            long long id,
            std::string selectStr,
            std::string filterStr,
            std::map<std::string,ArgumentVT> args,
            bool includeDeleted );
    std::string getSqlQuery() const { return sqlQuery; }
    std::vector<ArgumentVT> getArgs() const { return args; }

    bool isFunctionCall() const { return select.isFunctionCall(); }
    std::string getFunctionName() const { return select.getFunctionName(); }
    std::string getFunctionAttribute() const { return select.getFunctionAttribute(); }
    std::vector<std::string> getAttributes() const { return select.getAttributes(); }
};

} /* namespace Mist */

#endif /* INCLUDE_QUERY_H_ */
