#include "Query.h"
#include "Database.h"
#include <iostream>

namespace Mist {

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

bool ArgumentVT::operator <( const ArgumentVT& rhs ) const {
    if ( t == rhs.t ) {
        switch( t ) {
        case Type::Typeless:
        case Type::Null:
            return false;
        case Type::Boolean:
            return boolVal < rhs.boolVal;
        case Type::Number:
            return numberVal < numberVal;
        case Type::String:
        case Type::JSON:
            return 0 > strVal.compare( rhs.strVal );
        }
    }
    return static_cast<unsigned>( t ) < static_cast<unsigned>( rhs.t );
}

std::string Expression::getContent() const {
//    std::cout << content;
    return content;
}

std::vector<Token> QueryTokenizer::tokenize( const std::string& str ) {
    int line = 0;
    int startCol = 0;
    std::vector<Token> res;
    char ch;
    char chh;
    char chhh;

    for ( unsigned i{0}; i < str.length(); ++i ) {
        ch = str[i];
        if ( i+1 < str.length() )
            chh = str[i+1];
        else
            chh = 0;
        if ( i+2 < str.length() )
            chhh = str[i+2];
        else
            chhh = 0;

        if (QueryTokenizer::whiteSpace(ch)) {
            if( ch == '\n'){
                line ++;
                startCol = i;
            }
            unsigned j;
            for ( j=i+1; j < str.length() && QueryTokenizer::whiteSpace(str[j]); ++j ) {
                if(str[j]=='\n'){
                    line++;
                    startCol = j;
                }
            }
            j--;
            i = j;
        } else if ( ( ch == '.' && chh >= '0' && chh<='9' ) || ( ch >='0' && ch <'9' ) ) {
            bool hasPeriod = false;
            bool hasExponent = false;
            unsigned j;
            for ( j = i; j < str.length(); ++j ) {
                ch=str[j];
            if (ch>='0' && ch<='9')
                continue;
            if (ch == '.' && !hasPeriod) {
                hasPeriod = true;
                continue;
            }
            if (ch == 'e' && !hasExponent){
                hasExponent = true;
                continue;
            }
            break;
        }
        res.push_back( Token( TokenType::Number, str.substr(i,j-i),line,i-startCol ) );
        i = j-1;
    } else {
        switch(ch) {
            case ',':
                res.push_back(Token(TokenType::Comma,",",line,i-startCol));
                break;

            case '.':
                res.push_back(Token(TokenType::Dot,".",line,i-startCol));
                break;

            case '&':
                if (chh == '&') {
                    res.push_back(Token(TokenType::And,"&&",line,i-startCol));
                    i++;
                } else {
                    QueryTokenizer::parseError( line, i-startCol );
                }
                break;

            case '|':
                if (chh=='|') {
                    res.push_back(Token(TokenType::Or, "||",line,i-startCol));
                    i++;
                } else {
                    QueryTokenizer::parseError( line, i-startCol );
                }
                break;

            case '!':
                if (chh == '=') {
                    res.push_back(Token( TokenType::NotEqual, "!=", line, i-startCol ) );
                    i++;
                } else {
                    res.push_back(Token( TokenType::Not, "!", line, i-startCol ) );
                }
                break;

            case '=':
                if (chh == '=') {
                    res.push_back(Token( TokenType::Equal, "==", line, i-startCol ) );
                    i++;
                } else {
                    QueryTokenizer::parseError( line, i - startCol );
                }
                break;

            case '<':
                if (chh == '=') {
                    res.push_back(Token( TokenType::LessThanOrEqual, "<=", line, i-startCol ) );
                    i++;
                } else {
                    res.push_back(Token( TokenType::LessThan, "<", line, i-startCol ) );
                }
                break;

            case '>':
                if (chh == '=') {
                    res.push_back(Token( TokenType::GreaterThanOrEqual, ">=", line, i-startCol ) );
                    i++;
                } else {
                    res.push_back(Token( TokenType::GreaterThan, ">", line, i-startCol ) );
                }
                break;

            case '(':
                res.push_back(Token( TokenType::LeftPara, "(", line, i-startCol ) );
                break;

            case ')':
                res.push_back(Token( TokenType::RightPara, ")", line, i-startCol ) );
                break;

            case '"':
            case '\'':
                i = QueryTokenizer::tokenizeString( str, i, res, line, startCol );
                break;

            default:
                i = QueryTokenizer::tokenizeIdentifier( str, i, res, line, startCol );
                break;
            }
        }
    }
    return res;
}


void QueryTokenizer::parseError( int line, int col ) {
    throw std::runtime_error( "Parse error at line " + std::to_string( line ) + " col " + std::to_string( col ) );
}

int QueryTokenizer::tokenizeString( const std::string& str, int i , std::vector<Token> &res, int line, int startCol )
{
    char start = str[i];
    unsigned j;
    std::string content = "";

    for ( j=i+1; j < str.length() && str[j] != start; j++ ) {
        char ch = str[j];

        switch (ch) {
            case '\\':
                if ( j+1 >= str.length() )
                    QueryTokenizer::parseError( line, j+1 - startCol );
                j++;
                switch (str[j]) {
                    case 't':
                        content += '\t';
                        break;

                    case 'n':
                        content += '\n';
                        break;

                    case 'r':
                        content += '\r';
                        break;

                    default:
                        content += str[j];
                        break;
                }
                break;

            case '\n':
            case '\r':
                QueryTokenizer::parseError( line, j - startCol );
                break;

            default:
                content += str[j];
                break;
        }
    }
    if (j < str.length()) {
        res.push_back(Token( TokenType::String, content, line, i-startCol ) );
        return j;
    } else {
        QueryTokenizer::parseError( line, j-1 - startCol );
    }
}

int QueryTokenizer::tokenizeIdentifier( const std::string& str, int i, std::vector<Token> &res, int line, int startCol)
{
    unsigned j;
    const std::map<const char,const int> specialChars= {
            {'*', 1}, {',', 1}, {'.', 1}, {'?', 1}, {'(', 1}, { ')',1}, {'&', 1},
            {'|', 1}, {'!', 1}, {'=', 1}, {'<', 1}, {'>', 1}, {'"', 1}, {'\'', 1}
    };

    for (j=i+1; j < str.length() && !QueryTokenizer::whiteSpace( str.at(j) ); j++) {
        try {
            specialChars.at( str[j] );
            break;
        } catch ( const std::out_of_range& e ) {
            // OK
        }
    }
    j--;
    res.push_back(Token( TokenType::Identifier, str.substr( i, j+1-i ), line, i-startCol ) );
    return j;
}

Expression QueryParser::commaExpression()
{
    std::vector<Expression> res;

    res.push_back( expression() );
    while (getNextToken(std::vector<TokenType>{TokenType::Comma})!=NULL )
    {
        res.push_back( expression() );
    }
    Expression e = Expression( ExpressionType::Comma, res );
    return e;
}

Expression QueryParser::expression()
{
    return orExpression();
}

Expression QueryParser::orExpression()
{
    std::vector<Expression> res;
    res.push_back( andExpression() );
    if (getNextToken(std::vector<TokenType>{ TokenType::Or})!=NULL )
        res.push_back( orExpression() );
    if (res.size() == 1)
        return res[0];
    return Expression( ExpressionType::Or, res );
}

Expression QueryParser::andExpression()
{
    std::vector<Expression> res;

    res.push_back( compareExpression() );
    if (getNextToken(std::vector<TokenType>{ TokenType::And})!=NULL )
        res.push_back( andExpression() );
    if (res.size() == 1)
        return res[0];
    return Expression( ExpressionType::And, res );
}

Expression QueryParser::compareExpression()
{
    Expression left = unaryExpression();

    ExpressionType et;
    const std::vector<TokenType> tt{
        TokenType::Equal, TokenType::NotEqual, TokenType::GreaterThan, TokenType::LessThan,
        TokenType::GreaterThanOrEqual, TokenType::LessThanOrEqual } ;
    Token* t = getNextToken( tt );

    if ( !t )
        return left;
    switch( t->getType() )
    {
        case TokenType::Equal: et = ExpressionType::Equal; break;
        case TokenType::NotEqual: et = ExpressionType::NotEqual; break;
        case TokenType::GreaterThan: et = ExpressionType::GreaterThan; break;
        case TokenType::LessThan: et = ExpressionType::LessThan; break;
        case TokenType::GreaterThanOrEqual: et = ExpressionType::GreaterThanOrEqual; break;
        case TokenType::LessThanOrEqual: et = ExpressionType::LessThanOrEqual; break;
        // TODO: what todo with the rest of the cases? throw?
    }

    Expression e = unaryExpression() ;
    std::vector<Expression>v{left, e};
    return Expression( et, v );
}

Expression QueryParser::unaryExpression()
{
    if (getNextToken(std::vector<TokenType>{ TokenType::Not })){
        std::vector<Expression>v{functionExpression()};
        return Expression( ExpressionType::Not, v );
    }
    return functionExpression();
}

Expression QueryParser::functionExpression()
{
    Expression left = dotExpression();

    if (getNextToken(std::vector<TokenType>{TokenType::LeftPara })) {
        if (getNextToken(std::vector<TokenType>{TokenType::RightPara })){
            std::vector<Expression>v{left};
            return  Expression( ExpressionType::FunctionCall, v);
        }
        Expression args = commaExpression();

        if (!getNextToken(std::vector<TokenType>{TokenType::RightPara} )){
            parseError( lastToken() );
        }
        std::vector<Expression>v{left, args};
        return Expression( ExpressionType::FunctionCall,v );
    }
    return left;
}

Expression QueryParser::dotExpression()
{
    Expression left = identifierOrConstant();
//    std::cout << left.getContent() << std::endl;
    std::vector<Expression> args;

    if (left.getType() == ExpressionType::Identifier) {
        args = {left};
    }
    while (getNextToken(std::vector<TokenType>{ TokenType::Dot} )) {
        args.push_back( identifier() );
    }
    if (args.size() > 1){
        return  Expression( ExpressionType::Dot, args );
    }

    return left;
}

Expression QueryParser::identifierOrConstant()
{
    Token* t = getNextToken();
    std::vector<Expression>v{};

    if ( !t ) {
        parseError( lastToken() );
    } else if (t->getType() == TokenType::LeftPara) {
        Expression e = expression();
        std::vector<TokenType>v{TokenType::RightPara};
        if (!getNextToken(v)){
            parseError( lastToken() );
        }
        return e;
    } else if (t->getType() == TokenType::String) {
//        std::cout << t->getContent() << std::endl;
        return Expression( ExpressionType::String, v, t->getContent() );
    } else if (t->getType() == TokenType::Number) {
//        std::cout << t->getContent() << std::endl;

        return Expression( ExpressionType::Number,v , t->getContent() );
    } else if (t->getType() == TokenType::Identifier) {
//        std::cout << t->getContent() << std::endl;

        if ( t->getContent() == "true")
            return Expression( ExpressionType::True, v, t->getContent() );
        else if (t->getContent() == "false")
            return Expression( ExpressionType::False, v, t->getContent() );
        else if (t->getContent() == "null")
            return Expression( ExpressionType::False, v, t->getContent() );
        else
            return Expression( ExpressionType::Identifier,v, t->getContent() );
    } else {
        parseError( *t );
    }
}

Expression QueryParser::identifier()
{
    Token* t = getNextToken();
    std::vector<Expression>v{};

    if (!t) {
        parseError( lastToken() );
    } else if (t->getType() == TokenType::Identifier) {
        return  Expression( ExpressionType::Identifier, v, t->getContent() );
    } else {
        parseError( *t );
    }
}


Token* QueryParser::getNextToken( const std::vector<TokenType>& types)
{
    if (index >= tokens.size()){
        return nullptr;
    }
    if (types.size() == 0){
        return &tokens[ index++ ];
    }

    TokenType tt = tokens[ index ].getType();

    for (unsigned i=0; i < types.size(); i++) {
        if (types[i] == tt) {
            return &tokens[ index++ ];
        }
    }

    return nullptr; // TODO or something else?
}

Token QueryParser::lastToken()
{
    return tokens[ index-1 ];
}

void QueryParser::parseError( const Token& token )
{
    throw std::runtime_error( std::string( "Parse error at line " ) + std::to_string( token.getLine() ) + " col " + std::to_string( token.getCol() ) );
}


Expression QueryParser::parseExpression( std::vector<Token> tokens )
{
    QueryParser queryParser = QueryParser( tokens );

    return queryParser.expression();
}

Expression QueryParser::parseCommaExpression(std::vector<Token> tokens )
{
    QueryParser queryParser = QueryParser( tokens );

    return queryParser.commaExpression();
}

// TODO
/*
std::vector<Token> Query::tokenize( std::string str )
{
    return QueryTokenizer::tokenize( str );
}
//*/

void Select::parse( std::string str )
{
    if ( trim( str ).empty() ) {
        all = true;
        return;
    }

    Expression commaExpression = QueryParser::parseCommaExpression( QueryTokenizer::tokenize( str ) );
    std::vector<Expression> args = commaExpression.getArgs();

    if (args.size() == 1 && args[0].getType() == ExpressionType::FunctionCall) {
        Expression functionCall = args[0];
        std::vector<Expression> functionArgs;

        args = functionCall.getArgs();
        if (args[0].getType() != ExpressionType::Identifier)
            throw std::runtime_error( "Parse error at line " + std::to_string( args[0].getLine() ) + " col " + std::to_string( args[0].getCol() ) + " invalid function expression." );

        std::string functionName = args[0].getContent();

        if (!(functionName == "avg" || functionName == "count" || functionName == "max" || functionName == "min" || functionName == "sum"))
            throw std::runtime_error( "Parse error at line " + std::to_string( args[0].getLine() ) +
                    " col " + std::to_string( args[0].getCol() ) + " invalid function expression." );

        functionArgs = args[1].getArgs();
        if (functionArgs.size() == 0) {
            if (functionName != "count")
                throw std::runtime_error( "Parse error at line " + std::to_string( args[1].getLine() ) +
                        " col " + std::to_string( args[1].getCol() ) + " function " + functionName + " takes one argument." );
            this->functionName = functionName;
        } else {
            if (functionArgs.size() > 1)
                throw std::runtime_error( "Parse error at line " + std::to_string( args[1].getLine() ) +
                        " col " + std::to_string( args[1].getCol() ) + " function " + functionName +
                        " does not take more than one argument." );
            if (functionArgs[0].getType() != ExpressionType::Dot)
                throw std::runtime_error( "Parse error at line " + std::to_string( functionArgs[0].getLine() ) +
                        " col " + std::to_string( functionArgs[0].getCol() ) + " invalid function argument 1." );
            args = functionArgs[0].getArgs();
            if (args[0].getType() != ExpressionType::Identifier
                || args[0].getContent() != "o"
                || args[1].getType() != ExpressionType::Identifier)
                throw std::runtime_error( "Parse error at line " + std::to_string( functionArgs[0].getLine()) +
                        " col " + std::to_string( functionArgs[0].getCol() ) + " invalid function argument." );
            this->functionName = functionName;
            this->functionAttribute = args[1].getContent();
        }
    } else {
        for (unsigned  i=0; i < args.size(); i++) {
            std::vector<Expression> a;
            if (args[i].getType() != ExpressionType::Dot)
                throw std::runtime_error( "Parse error at line " + std::to_string(  args[i].getLine() ) + " col " + std::to_string( args[i].getCol() ) + " invalid attribute." );
            a = args[i].getArgs(); // TODO: verify this
            if (a[0].getType() != ExpressionType::Identifier
                || a[0].getContent() != "o"
                || a[1].getType() != ExpressionType::Identifier)
                throw std::runtime_error( "Parse error at line " + std::to_string( args[i].getLine() ) + " col " + std::to_string( args[i].getCol() ) + " invalid attribute." );
            this->attributes.push_back( a[1].getContent() );
        }
    }
}

/*
FilterExpression::FilterExpression() :
        type( NoType ), _operator(), left( nullptr ), right( nullptr ), attribute(),
        argument(), constBoolean( false ), constNumber(), constString() {
}
//*/
FilterExpression::FilterExpression() :
        type( NoType ), _operator(), left( nullptr ), right( nullptr ), value() {
}

FilterExpression::~FilterExpression()
{
    if (type == UnaryFunction || type == BinaryFunction)
        delete this->left;
    if (type == BinaryFunction)
        delete this->right;
}

/*
FilterExpressionType FilterExpression::getType() {
    return type;
}

std::string FilterExpression::getOperator() {
    return _operator;
}
FilterExpression *FilterExpression::getLeft() {
    return left;
}

FilterExpression *FilterExpression::getRight() {
    return right;
}

std::string FilterExpression::getAttribute() {
    return attribute;
}

std::string FilterExpression::getArgument() {
    return argument;
}

bool FilterExpression::getConstBoolean() {
    return constBoolean;
}

double FilterExpression::getConstNumber() {
    return constNumber;
}

std::string FilterExpression::getConstString() {
    return constString;
}
//*/

bool FilterExpression::attributeArgumentOrConstant() const {
    return ( type == Attribute || type == Argument || type == ConstBoolean || type == ConstNumber || type == ConstString || type == ConstNull );
}

std::set<std::string> FilterExpression::getAttributes()
{
    std::set<std::string> res;

    if (type == Attribute)
        res.insert( attribute );
    if (type == UnaryFunction || type == BinaryFunction) {
        std::set<std::string> r = left->getAttributes();

        for (std::set<std::string>::iterator it=r.begin(); it!=r.end(); ++it)
            res.insert( *it );
    }
    if (type == BinaryFunction) {
        std::set<std::string> r = right->getAttributes();

        for (std::set<std::string>::iterator it=r.begin(); it!=r.end(); ++it)
            res.insert( *it );
    }
    return res;
}

std::set<std::string> FilterExpression::getArguments()
{
    std::set<std::string> res;

    if (type == Argument)
        res.insert( argument );
    if (type == UnaryFunction || type == BinaryFunction) {
        std::set<std::string> r = left->getArguments();

        for (std::set<std::string>::iterator it=r.begin(); it!=r.end(); ++it)
            res.insert( *it );
    }
    if (type == BinaryFunction) {
        std::set<std::string> r = right->getArguments();

        for (std::set<std::string>::iterator it=r.begin(); it!=r.end(); ++it)
            res.insert( *it );
    }
    return res;
}

std::set<ArgumentVT> FilterExpression::getConstants() {
    std::set<ArgumentVT> res{};

    if( type == ConstBoolean )
        res.insert( value );
    else if ( type == ConstNumber )
        res.insert( value );
    else if ( type == ConstString )
        res.insert( value );
    else if ( type == ConstNull )
        res.insert( value );

    if ( left )
        for( const ArgumentVT& v : left->getConstants() )
            res.insert( v );
    if ( right )
        for( const ArgumentVT& v : right->getConstants() )
            res.insert( v );
    return res;
}

bool FilterExpression::hasComparison()
{
    if (type == BinaryFunction) {
        if ( _operator == "=="
            || _operator == "!="
            || _operator == "<"
            || _operator == ">"
            || _operator == "<="
            || _operator == ">=")
            return true;
        else
            return left->hasComparison() && right->hasComparison();
    } else if (type == UnaryFunction) {
        return left->hasComparison();
    }
    return false;
}

std::string escapeString( const std::string& str ) {
    std::string res{ str };
    for( std::string::const_iterator it{ res.begin() }; it != res.end(); ++it ) {
        if ( '"' == *it || '\\' == *it ) {
            res.insert( it, '\\' );
        }
    }
    return res;
}

void FilterExpression::parse( Expression expression )
{
    std::vector<Expression> args = expression.getArgs();
    switch (expression.getType()) {
        case ExpressionType::String:
            type = ConstString;
            value = expression.getContent();
            break;

        case ExpressionType::Number:
            type = ConstNumber;
            value = std::stod( expression.getContent() );
            break;

        case ExpressionType::True:
            type = ConstBoolean;
            value = true;
            break;

        case ExpressionType::False:
            type = ConstBoolean;
            value = false;
            break;

        case ExpressionType::Null:
            type = ConstNull;
            value = std::nullptr_t();
            break;

        case ExpressionType::Dot:
            if (args[0].getType() == ExpressionType::Identifier
                && args[1].getType() == ExpressionType::Identifier) {
                if (args[0].getContent() == "a") {
                    this->type = Argument;
                    this->argument = args[1].getContent();
                } else if ( args[0].getContent() == "o") {
                    this->type = Attribute;
                    this->attribute = args[1].getContent();
                } else {
                    throw std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) );
                }
            } else {
                throw std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) );
            }
            break;

        case ExpressionType::And:
            type = BinaryFunction;
            _operator = "&&";
            left = new FilterExpression();
            right = new FilterExpression();
            left->parse( args[0] );
            right->parse( args[1] );
            if (!left->hasComparison() || !right->hasComparison())
                std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) + " and expression must have two arguemnts that contain a comparison expression." );
            break;

        case ExpressionType::Or:
            type = BinaryFunction;
            _operator = "||";
            left = new FilterExpression();
            right = new FilterExpression();
            left->parse( args[0] );
            right->parse( args[1] );
            if (!left->hasComparison() || !right->hasComparison())
                std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) + " or expression must have two arguemnts that contain a comparison expression." );
            break;

        case ExpressionType::Not:
            type = UnaryFunction;
            _operator = "!";
            left = new FilterExpression();
            left->parse( args[0] );
            if (!left->hasComparison())
                std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) + " not expression must have one arguemnt that contain a comparison expression." );
            break;

        case ExpressionType::Equal:
        case ExpressionType::NotEqual:
        case ExpressionType::LessThan:
        case ExpressionType::GreaterThan:
        case ExpressionType::LessThanOrEqual:
        case ExpressionType::GreaterThanOrEqual:
            type = BinaryFunction;
            switch ( expression.getType() ) {
                case ExpressionType::Equal: _operator = "=="; break;
                case ExpressionType::NotEqual: _operator = "!="; break;
                case ExpressionType::LessThan: _operator = "<"; break;
                case ExpressionType::GreaterThan: _operator = ">"; break;
                case ExpressionType::LessThanOrEqual: _operator = "<="; break;
                case ExpressionType::GreaterThanOrEqual: _operator = ">="; break;
            }
            left = new FilterExpression();
            right = new FilterExpression();
            left->parse( args[0] );
            right->parse( args[1] );
            if (left->getType() != Attribute) {
                FilterExpression *t = left;

                left = right;
                right = t;
                if (_operator == "<")
                    _operator = ">";
                if (_operator == ">")
                    _operator = "<";
                if (_operator == "<=")
                    _operator = ">=";
                if (_operator == ">=")
                    _operator = "<=";
            }
            if (left->getType() != Attribute)
                std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) + " must contain an attribute expression on either left or right hand." );
            if (right->getType() == Attribute && left->getAttribute() == right->getAttribute())
                std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) + " cannot use same attribute on both left and right side in an expression." );
            if (!right->attributeArgumentOrConstant())
                std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) + " comparison operation must compare an attribute against another attribute, an argument or a constant." );
            break;

        defualt:
            throw std::runtime_error( "Parse error at line " + std::to_string( expression.getLine() ) + " col " + std::to_string( expression.getCol() ) );
            break;
    }
}

/*
std::string FilterExpression::makeSQL1( std::map<std::string,std::string> &args,
        std::map<std::string,std::string> &argsIndex, std::map<std::string,std::string> &constIndex )
//*/
std::string FilterExpression::makeSQL(
            const std::map<std::string,ArgumentVT>& args,
            const std::map<std::string,std::string>& argsIndex,
            const std::map<ArgumentVT,std::string>& constIndex )
{
    if ( type == UnaryFunction && _operator == "!" ) {
        return "NOT (" + left->makeSQL( args, argsIndex, constIndex ) + ") ";
    } else if (type == BinaryFunction && _operator == "&&") {
        return "(" + left->makeSQL( args, argsIndex, constIndex ) + ") AND ("
                + right->makeSQL( args, argsIndex, constIndex ) + ") ";
    } else if (type == BinaryFunction && _operator == "||") {
        return "(" + left->makeSQL( args, argsIndex, constIndex ) + ") OR ("
                + right->makeSQL( args, argsIndex, constIndex ) + ") ";
    } else if ( type == BinaryFunction && (_operator == "==" || _operator == "!=") ) {
        std::string res;

        if (right->getType() == Attribute) {
            res = "a" + left->getAttribute() + ".value IS NULL AND a" + right->getAttribute() + " IS NULL "
                + "OR a" + left->getAttribute() + ".value IS NOT NULL "
                    + "AND a" + right->getAttribute() + ".value IS NOT NULL "
                    + "AND a" + left->getAttribute() + ".type == a" + right->getAttribute() + ".type"
                    + "AND a" + left->getAttribute() + ".value == a" + right->getAttribute() + ".value ";
        } else if (right->getType() == Argument) {
            std::string argument = right->getArgument();

            if ( !args.count( argument ) )
                std::runtime_error( "Unknown argument " + right->getArgument() );

            const ArgumentVT& arg = args.at( argument );
            std::string argIndex{};

            try {
                argIndex = argsIndex.at( argument );
            } catch (const std::out_of_range& e ) {
                throw std::runtime_error( "Unknown argument " + right->getArgument() );
            }

            if ( arg.type() == Type::Typeless )
                res = "a" + left->getAttribute() + ".value IS NULL ";
            else if ( arg.type() == Type::Null )
                res = "a" + left->getAttribute() + ".value IS NOT NULL "
                    + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::Null ) ) + " ";
            else if ( arg.type() == Type::Boolean )
                res = "a" + left->getAttribute() + ".value IS NOT NULL "
                        + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::Boolean ) ) + " "
                        + "AND a" + left->getAttribute() + ".value = " + argIndex + " ";
            else if ( arg.type() == Type::Number )
                res = "a" + left->getAttribute() + ".value IS NOT NULL "
                    + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::Number ) ) + " "
                    + "AND a" + left->getAttribute() + ".value = " + argIndex + " ";
            else if ( arg.type() == Type::String ) // is string?
                res = "a" + left->getAttribute() + ".value IS NOT NULL "
                    + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::String ) ) + " "
                    + "AND a" + left->getAttribute() + ".value = " + argIndex + " ";
            else
                throw std::runtime_error( "Wrong type of argument " + argument );

        } else if (right->getType() == ConstBoolean)
            res = "a" + left->getAttribute() + ".value IS NOT NULL "
                + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::Boolean ) ) + " "
                + "AND a" + left->getAttribute() + ".value = " + constIndex.at( this->right->value ) + " ";
        else if (right->getType() == ConstNull)
            res = "a" + left->getAttribute() + ".value IS NOT NULL "
                + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::Null ) ) + " ";
        else if (right->getType() == ConstNumber)
            res = "a" + left->getAttribute() + ".value IS NOT NULL "
                + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::Number ) ) + " "
                + "AND a" + left->getAttribute() + ".value = " + constIndex.at( this->right->value ) + " ";
        else if (right->getType() == ConstString)
            res = "a" + left->getAttribute() + ".value IS NOT NULL "
                + "AND a" + left->getAttribute() + ".type = " + std::to_string( static_cast<int>( Type::String ) ) + " "
                + "AND a" + left->getAttribute() + ".value = " + constIndex.at( this->right->value ) + " ";
        if (_operator == "==")
            return res;
        else
            return "NOT (" + res + ")";
    } else if (type == BinaryFunction && (_operator == "<" || _operator == ">" || _operator == "<=" || _operator == ">=")) {
        if (right->getType() == Attribute) {
            return "a" + left->getAttribute() + ".value IS NOT NULL "
                + "AND a" + right->getAttribute() + ".value IS NOT NULL "
                + "AND a" + left->getAttribute() + ".type == " + std::to_string( static_cast<int>( Type::Number ) ) + " "
                + "AND a" + right->getAttribute() + ".type == " + std::to_string( static_cast<int>( Type::Number ) ) + " "
                + "AND a" + left->getAttribute() + ".value " + _operator + "a" + right->getAttribute() + ".value ";
        } else if (right->getType() == Argument) {
            std::string argument = right->getArgument();

            if (!args.count( argument ))
                std::runtime_error( "Unknown argument " + right->getArgument() );

            ArgumentVT arg = args.at( argument );
            std::string argIndex = argsIndex.at( argument );

            if ( arg.type() != Type::Number ) {
                return "0 == 1";
            } else {
                return "a" + left->getAttribute() + ".value IS NOT NULL "
                    + "AND a" + left->getAttribute() + ".type == " + std::to_string( static_cast<int>( Type::Number ) ) + " "
                    + "AND a" + left->getAttribute() + ".value " + _operator + " " + argIndex + " ";
            }
        } else if ( right->getType() == ConstNumber ) {
            return "a" + left->getAttribute() + ".value IS NOT NULL "
                + "AND a" + left->getAttribute() + ".type == " + std::to_string( static_cast<int>( Type::Number ) ) + " "
                + "AND a" + left->getAttribute() + ".value " + _operator + " " + constIndex.at( right->value ) + " "; // TODO: reconsider?
        } else {
            return "0 == 1";
        }
    }
    std::runtime_error( "Parse error" );
}

Filter::Filter()
    : none(false)
{}

void Filter::parse( std::string str )
{
    if (str.length() == 0) {
        none = true;
        return;
    }

    expression.parse( QueryParser::parseExpression( QueryTokenizer::tokenize( str ) ) );
}

void Filter::makeSQL( Query &res, const std::map<std::string,ArgumentVT> &args, int maxVersion, std::string status, bool versionsQuery )
{
    if (none) {
        if (maxVersion) {
            res.args.push_back( std::to_string( maxVersion ) );
            res.sqlQuery += std::string( "AND o.rowId IN (SELECT o.rowId, max(version) FROM Object AS o " )
                + "WHERE o.accessDomain=" + Query::printArg( 1 )
                + (versionsQuery ? " AND o.id=" : " AND o.parent=" ) + Query::printArg( 2 ) + " "
                + "AND " + status + " AND version >= " + Query::printArg( res.args.size() ) + " "
                + (versionsQuery ? " GROUP BY o.version " : " GROUP BY o.id " )
                + ")";
        } else {
        }
    } else {
        std::map<std::string,std::string> argsIndex;
        std::map<ArgumentVT,std::string> constIndex;

        std::set<std::string> _arguments = this->expression.getArguments();
        std::set<std::string> attributes = this->expression.getAttributes();
        std::set<ArgumentVT> constants = this->expression.getConstants();

        for ( const std::string& k : _arguments ) {
            if ( !args.count( k ) )
                throw std::runtime_error( "Missing argument " + k );
            res.args.push_back( args.at( k ) );
            argsIndex[ k ] = Query::printArg( res.args.size() );
        }
        for( const ArgumentVT& v : constants ) {
            res.args.push_back( v );
            constIndex[ v ] = Query::printArg( res.args.size() );
        }
        if (maxVersion) {
            res.sqlQuery += "AND o.rowId IN (SELECT o.rowId, max(version) FROM Object AS o ";
        } else {
            res.sqlQuery += "AND o.rowId IN (SELECT o.rowId FROM Object AS o ";
        }
        for( const std::string& k : attributes ) {
            res.args.push_back( k );
            res.sqlQuery += "LEFT OUTER JOIN Attribute a" + k + " ON o.accessDomain=a" + k + ".accessDomain AND o.id=a" + k + ".id "
                + "AND o.version=a" + k + ".version AND a" + k + ".name=" + Query::printArg( res.args.size() ) + " ";
        }
        if (maxVersion) {
            res.args.push_back( std::to_string( maxVersion ) );
            res.sqlQuery += "WHERE o.accessDomain=" + Query::printArg( 1 )
                + (versionsQuery ? " AND o.id=" : " AND o.parent=" ) + Query::printArg( 2 ) + " "
                + "AND " + status + " AND version >= " + Query::printArg( res.args.size() ) + " ";
        } else {
            res.sqlQuery += "WHERE o.accessDomain=" + Query::printArg( 1 )
                + (versionsQuery ? " AND o.id=" : " AND o.parent=" ) + Query::printArg( 2 ) + " "
                + "AND " + status + " ";
        }
        res.sqlQuery += "AND ";
        res.sqlQuery += expression.makeSQL( args, argsIndex, constIndex );
        res.sqlQuery += (versionsQuery ? " GROUP BY o.version " : " GROUP BY o.id " );
        res.sqlQuery += ")";
    }
}

void Sort::parse( const std::string& str ) {
    {
        std::string tmp{ str };
        if ( trim( tmp ).empty() ) {
            this->none = true;
            return;
        }
    }
    this->none = false;

    Expression e{ QueryParser::parseExpression( QueryTokenizer::tokenize( str ) ) };

    if ( ExpressionType::FunctionCall == e.getType()
            && 2 == e.getArgs().size()
            && ExpressionType::Identifier == e.getArgs().at(0).getType()
            && ( "desc" == e.getArgs().at(0).getContent() || "asc" == e.getArgs().at(0).getContent() )
            && 1 == e.getArgs().at(1).getArgs().size()
            && ExpressionType::Dot == e.getArgs().at(1).getArgs().at(0).getType()
            && 2 == e.getArgs().at(1).getArgs().at(0).getArgs().size()
            && ExpressionType::Identifier == e.getArgs().at(1).getArgs().at(0).getArgs().at(0).getType()
            && "o" == e.getArgs().at(1).getArgs().at(0).getArgs().at(0).getContent()
            && ExpressionType::Identifier == e.getArgs().at(1).getArgs().at(0).getArgs().at(1).getType() ) {
        this->desc = "desc" == e.getArgs().at(0).getContent();
        this->attribute = e.getArgs().at(1).getArgs().at(0).getArgs().at(1).getContent();
        return;
    } else if ( ExpressionType::Dot == e.getType()
            && 2 == e.getArgs().size()
            && ExpressionType::Identifier == e.getArgs().at(0).getType()
            && "o" == e.getArgs().at(0).getContent()
            && ExpressionType::Identifier == e.getArgs().at(1).getType()) {
        this->desc = false;
        this->attribute = e.getArgs().at(1).getContent();
        return;
    } else {
        throw std::runtime_error( "Parse error, should be an empty string, o.[attribute], asc( o.[attribute] ) or desc( o.[attribute] )" );
    }
}

std::string Query::printArg( int i ) {
    if (i >= 100)
        return "?" + std::to_string( i );
    if (i >= 10)
        return "?0" + std::to_string( i );
    return "?00" + std::to_string( i );
}

void Query::parseQuery( int accessDomain, long long parent, std::string selectStr, std::string filterStr, std::string sortStr, std::map<std::string,ArgumentVT> args, int maxVersion, bool includeDeleted ) {
    std::string status;

    select.parse( selectStr );
    filter.parse( filterStr );
    sort.parse( sortStr );

    if (maxVersion) {
        if (includeDeleted) {
            status = " o.status IN (" + std::to_string( (int )Mist::Database::ObjectStatus::Current )
                + ", " + std::to_string( (int )Mist::Database::ObjectStatus::Deleted )
                + ", " + std::to_string( (int )Mist::Database::ObjectStatus::DeletedParent )
                + ", " + std::to_string( (int )Mist::Database::ObjectStatus::Old )
                + ", " + std::to_string( (int )Mist::Database::ObjectStatus::OldDeleted )
                + ", " + std::to_string( (int )Mist::Database::ObjectStatus::OldDeletedParent ) + ")";
        } else {
            status = " o.status IN (" + std::to_string( (int )Mist::Database::ObjectStatus::Current )
                + ", " + std::to_string((int ) Mist::Database::ObjectStatus::Old ) + ") ";
        }
    } else {
        if (includeDeleted) {
            status = " o.status <= " + std::to_string( (int )Mist::Database::ObjectStatus::DeletedParent );
        } else {
            status = " o.status == " + std::to_string( (int )Mist::Database::ObjectStatus::Current );
        }
    }
    this->args.push_back( std::to_string( accessDomain ) );
    this->args.push_back( std::to_string( parent ) );
    if (maxVersion)
        this->args.push_back( std::to_string(  maxVersion ) );
    if (select.getFunctionName() != "") {
        if ( !sort.getNone() )
            throw std::runtime_error( "Cannot sort output from a function" );
        if ( select.getFunctionAttribute().size() > 0 ) {
            this->args.push_back( select.getFunctionAttribute() );
            int argIndex = this->args.size();

            this->sqlQuery = "SELECT " + select.getFunctionName() + "( a.value ) AS value "
                + "FROM Object AS o, Attribute AS a "
                + "WHERE o.accessDomain=" + printArg( 1 ) + " AND o.parent=" + printArg( 2 ) + " AND " + status + " ";
                + "AND a.accessDomain=o.accessDomain AND a.id=o.id AND a.version=o.version AND a.name=" + printArg( argIndex ) + " "
                + "AND a.type=" + std::to_string( static_cast<int>( Type::Number ) ) + " ";
            filter.makeSQL( *this, args, maxVersion, status, false );
        } else {
            this->sqlQuery = "SELECT " + select.getFunctionName() + "( * ) AS value "
                + "FROM Object AS o "
                + "WHERE o.accessDomain=" + printArg( 1 ) + " AND o.parent=" + printArg( 2 ) + " AND " + status + " ";
            filter.makeSQL( *this, args, maxVersion, status, false );
        }
    } else {
        std::string attributeNames = "";

        if (!select.getAll()) {
            attributeNames += "AND a.name IN ( ";
            for( const std::string& k: select.getAttributes() ) {
                this->args.push_back( k );
                attributeNames += printArg( this->args.size() ) + ",";
            }
            attributeNames.pop_back();
            attributeNames += ") ";
        }
        if (!sort.getNone())
            this->args.push_back( sort.getAttribute() );
        this->sqlQuery = std::string( "SELECT o.id AS _id, o.version AS _version, a.name AS name, a.type AS type, a.value AS value " )
            + "FROM Object AS o, Attribute AS a "
            + (sort.getNone() ? "" : std::string( "LEFT OUTER JOIN Attribute AS aSort WHERE o.accessDomain=a.accessDomain AND o.id=a.id AND o.version=a.version " )
                + "AND a.name=" + printArg( this->args.size() ) + " ")
            + "WHERE o.accessDomain=a.accessDomain AND o.id=a.id AND o.version=a.version " + attributeNames + " ";
        filter.makeSQL( *this, args, maxVersion, status, false );
        if (sort.getNone()) {
            this->sqlQuery += "ORDER BY o.version, o.id ";
        } else {
            this->sqlQuery += std::string( "ORDER BY aSort " ) + (sort.getDesc() ? "DESC" : "") + ", o.version, o.id";
        }
    }
}

void Query::parseVersionQuery( int accessDomain, long long parent, std::string selectStr, std::string filterStr, std::map<std::string,ArgumentVT> args, bool includeDeleted ) {
    std::string status;

    select.parse( selectStr );
    filter.parse( filterStr );

    if (includeDeleted) {
        status = " o.status IN (" + std::to_string( (int )Mist::Database::ObjectStatus::Current )
            + ", " + std::to_string( (int )Mist::Database::ObjectStatus::Deleted )
            + ", " + std::to_string( (int )Mist::Database::ObjectStatus::DeletedParent )
            + ", " + std::to_string( (int )Mist::Database::ObjectStatus::Old )
            + ", " + std::to_string( (int )Mist::Database::ObjectStatus::OldDeleted )
            + ", " + std::to_string( (int )Mist::Database::ObjectStatus::OldDeletedParent ) + ")";
    } else {
        status = " o.status IN (" + std::to_string( (int )Mist::Database::ObjectStatus::Current )
            + ", " + std::to_string( (int )Mist::Database::ObjectStatus::Old ) + ") ";
    }

    this->args.push_back( std::to_string( accessDomain ) );
    this->args.push_back( std::to_string( parent ) );
    if (select.getFunctionName().length() > 0) {
        if (select.getFunctionAttribute().length() > 0) {
            this->args.push_back( select.getFunctionAttribute() );
            int argIndex = this->args.size();

            this->sqlQuery = "SELECT " + select.getFunctionName() + "( a.value ) AS value "
                + "FROM Object AS o, Attribute AS a "
                + "WHERE o.accessDomain=" + printArg( 1 ) + " AND o.id=" + printArg( 2 ) + " AND " + status + " ";
                + "AND a.accessDomain=o.accessDomain AND a.id=o.id AND a.version=o.version AND a.name=" + printArg( argIndex ) + " "
                + "AND a.type=" + std::to_string( static_cast<int>( Type::Number ) ) + " ";
            filter.makeSQL( *this, args, 0, status, true );
        } else {
            this->sqlQuery = "SELECT " + select.getFunctionName() + "( * ) AS value "
                + "FROM Object AS o "
                + "WHERE o.accessDomain=" + printArg( 1 ) + " AND o.parent=" + printArg( 2 ) + " AND " + status + " ";
            filter.makeSQL( *this, args, 0, status, true );
        }
    } else {
        std::string attributeNames;

        if (!select.getAll()) {
            attributeNames += "AND a.name IN ( ";
            for ( const std::string& k : select.getAttributes() ) {
                this->args.push_back( k );
                attributeNames += printArg( this->args.size() ) + ",";
            }
            attributeNames.pop_back();
            attributeNames += ") ";
        }
        this->sqlQuery = std::string( "SELECT o.id AS _id, o.version AS _version, a.name AS name, a.type AS type, a.value AS value " )
            + "FROM Object AS o, Attribute AS a "
            + "WHERE o.accessDomain=a.accessDomain AND o.id=a.id AND o.version=a.version " + attributeNames + " ";
        filter.makeSQL( *this, args, 0, status, true );
        this->sqlQuery += "ORDER BY o.version, o.id, a.name ";
    }
}

} /* namespace Mist */
