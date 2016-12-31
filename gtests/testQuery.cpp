/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <gtest/gtest.h>

#include "Query.h"

namespace M = Mist;

namespace {

using T = M::TokenType;
using E = M::ExpressionType;
using QP = M::QueryParser;

class QueryTest: public ::testing::Test {
public:
    M::Query q;
    M::QueryTokenizer qt;
};

TEST_F( QueryTest, TestConstructors ) {

}

TEST_F( QueryTest, TestOperators ) {
    std::vector<M::Token> t;
    t = qt.tokenize( "hejhopp" );
    ASSERT_EQ( 1u, t.size() );
    ASSERT_EQ( T::Identifier, t.at(0).getType() );
    ASSERT_EQ( "hejhopp", t.at(0).getContent() );

    t = qt.tokenize( "hej==hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::Equal);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej!=hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::NotEqual);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej<=hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::LessThanOrEqual);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej>=hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::GreaterThanOrEqual);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej<hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::LessThan);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej>hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::GreaterThan);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej,hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::Comma);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej.hopp" );
    ASSERT_EQ( t.size(), 3u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::Dot);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "hej!hopp!" );
    ASSERT_EQ( t.size(), 4u );
    ASSERT_EQ( t.at(0).getType(), T::Identifier );
    ASSERT_EQ( t.at(1).getType(), T::Not);
    ASSERT_EQ( t.at(2).getType(), T::Identifier);
    ASSERT_EQ( t.at(3).getType(), T::Not);
    ASSERT_EQ( t.at(0).getContent(), "hej" );
    ASSERT_EQ( t.at(2).getContent(), "hopp" );

    t = qt.tokenize( "(hej)(hopp)" );
    ASSERT_EQ( t.size(), 6u );
    ASSERT_EQ( t.at(0).getType(), T::LeftPara );
    ASSERT_EQ( t.at(1).getType(), T::Identifier );
    ASSERT_EQ( t.at(2).getType(), T::RightPara);
    ASSERT_EQ( t.at(3).getType(), T::LeftPara );
    ASSERT_EQ( t.at(4).getType(), T::Identifier );
    ASSERT_EQ( t.at(5).getType(), T::RightPara);
    ASSERT_EQ( t.at(1).getContent(), "hej" );
    ASSERT_EQ( t.at(4).getContent(), "hopp" );
}

TEST_F( QueryTest, TestString ) {
    std::vector<M::Token> t;

    t = qt.tokenize( "\"hejhopp\"" );
    ASSERT_EQ( t.size(), 1u );
    ASSERT_EQ( t.at(0).getType(), T::String );
    ASSERT_EQ( t.at(0).getContent(), "hejhopp");

    t = qt.tokenize( "\"hej\\hopp\"" );
    ASSERT_EQ( t.size(), 1u );
    ASSERT_EQ( t.at(0).getType(), T::String);
    ASSERT_EQ( t.at(0).getContent(), "hejhopp" );

    t = qt.tokenize( "\"hej\'hopp\"" );
    ASSERT_EQ( t.size(), 1u );
    ASSERT_EQ( t.at(0).getType(), T::String);
    ASSERT_EQ( t.at(0).getContent(), "hej'hopp" );

    t = qt.tokenize( "\"hej\\\"hopp\"" );
    ASSERT_EQ( t.size(), 1u );
    ASSERT_EQ( t.at(0).getType(), T::String);
    ASSERT_EQ( t.at(0).getContent(), "hej\"hopp" );

    t = qt.tokenize( "\'hej\"hopp\'" );
    ASSERT_EQ( t.size(), 1u );
    ASSERT_EQ( t.at(0).getType(), T::String);
    ASSERT_EQ( t.at(0).getContent(), "hej\"hopp" );
}

TEST_F( QueryTest, TestNumber ) {
    std::vector<M::Token> t;

    t = qt.tokenize( "12345" );
    ASSERT_EQ( t.size(), 1u );
    ASSERT_EQ( t.at(0).getType(), T::Number );
    ASSERT_EQ( t.at(0).getContent(), "12345");

    t = qt.tokenize( ".17" );
    ASSERT_EQ( t.size(), 1u );
    ASSERT_EQ( t.at(0).getType(), T::Number );
    ASSERT_EQ( t.at(0).getContent(), ".17" );
}

TEST_F( QueryTest, TestExpressions) {
    M::Expression e = QP::parseExpression( qt.tokenize( "o.test" ) );
    ASSERT_EQ( e.getType(), E::Dot );
    ASSERT_EQ( e.getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getContent(), "o" );
    ASSERT_EQ( e.getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[1].getContent(), "test" );

    e = QP::parseExpression( qt.tokenize( "o.test == a.max" ) );
    ASSERT_EQ( e.getType(), E::Equal );
    ASSERT_EQ( e.getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getType(), E::Dot );
    ASSERT_EQ( e.getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getContent(), "o" );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getContent(), "test" );
    ASSERT_EQ( e.getArgs()[1].getType(), E::Dot );
    ASSERT_EQ( e.getArgs()[1].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getContent(), "a" );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getContent(), "max" );

    e = QP::parseExpression( qt.tokenize( "(o.test==a.max||o.test==17)&&o.trim!='hej'&&o.len>17" ) );
    ASSERT_EQ( e.getType(), E::And );
    ASSERT_EQ( e.getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getType(), E::Or );
    ASSERT_EQ( e.getArgs()[1].getType(), E::And );
    ASSERT_EQ( e.getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getType(), E::Equal );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getType(), E::Equal );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getType(), E::NotEqual );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getType(), E::GreaterThan );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[0].getType(), E::Dot );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[1].getType(), E::Dot );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[0].getType(), E::Dot );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[1].getType(), E::Number );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[1].getContent(), "17" );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[0].getType(), E::Dot );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[1].getType(), E::String );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[1].getContent(), "hej" );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[0].getType(), E::Dot );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[1].getType(), E::Number );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[1].getContent(), "17" );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[1].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[0].getArgs().size(), 2u );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[0].getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[0].getArgs()[0].getContent(), "o" );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[0].getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[0].getArgs()[1].getContent(), "test" );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[1].getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[1].getArgs()[0].getContent(), "a" );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[1].getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[0].getArgs()[1].getArgs()[1].getContent(), "max" );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[0].getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[0].getArgs()[0].getContent(), "o" );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[0].getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[0].getArgs()[1].getArgs()[0].getArgs()[1].getContent(), "test" );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[0].getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[0].getArgs()[0].getContent(), "o" );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[0].getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[1].getArgs()[0].getArgs()[0].getArgs()[1].getContent(), "trim" );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[0].getArgs()[0].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[0].getArgs()[0].getContent(), "o" );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[0].getArgs()[1].getType(), E::Identifier );
    ASSERT_EQ( e.getArgs()[1].getArgs()[1].getArgs()[0].getArgs()[1].getContent(), "len" );
}

TEST_F( QueryTest, TestSelectExpression ) {
    Mist::Select selectA, selectB, selectC, selectD;

    selectA.parse( "sum(o.len)" );
    selectB.parse( "" );
    selectC.parse( "o.len, o.bar,o.hej" );
    selectD.parse( "o.len" );

    ASSERT_EQ( selectA.getAll(), false );
    ASSERT_EQ( selectA.getFunctionName(), "sum" );
    ASSERT_EQ( selectA.getFunctionAttribute(), "len" );

    ASSERT_EQ( selectB.getAll(), true );
    ASSERT_EQ( selectB.getFunctionName(), "" );

    ASSERT_EQ( selectC.getAll(), false );
    ASSERT_EQ( selectC.getFunctionName(), "" );
    ASSERT_EQ( selectC.getAttributes().size(), 3u );
    ASSERT_EQ( selectC.getAttributes()[0], "len" );
    ASSERT_EQ( selectC.getAttributes()[1], "bar" );
    ASSERT_EQ( selectC.getAttributes()[2], "hej" );

    ASSERT_EQ( selectD.getAll(), false );
    ASSERT_EQ( selectD.getFunctionName(), "" );
    ASSERT_EQ( selectD.getAttributes().size(), 1u );
    ASSERT_EQ( selectD.getAttributes()[0], "len" );
}

TEST_F( QueryTest, TestFilterExpression ) {
    Mist::Filter filterA, filterB;

    filterA.parse( "" );
    filterB.parse( "(a.max>o.len||o.cow==true) && 'hej' == o.name && o.width < 17" );
}

TEST_F( QueryTest, TestSortExpression ) {
    Mist::Sort sortA, sortB, sortC, sortD;

    sortA.parse( "" );
    sortB.parse( "o.name" );
    sortC.parse( "desc(o.name)" );
    sortD.parse( "asc(o.name)" );

    ASSERT_EQ( sortA.getNone(), true );

    ASSERT_EQ( sortB.getNone(), false );
    ASSERT_EQ( sortB.getDesc(), false );
    ASSERT_EQ( sortB.getAttribute(), "name" );

    ASSERT_EQ( sortC.getNone(), false );
    ASSERT_EQ( sortC.getDesc(), true );
    ASSERT_EQ( sortC.getAttribute(), "name" );

    ASSERT_EQ( sortD.getNone(), false );
    ASSERT_EQ( sortD.getDesc(), false );
    ASSERT_EQ( sortD.getAttribute(), "name" );
}

TEST_F( QueryTest, TestMakeSQLQuery ) {
    Mist::Query qA, qB, qC, qD;
    std::map<std::string,Mist::ArgumentVT> args;

    //args[ "max" ] = Mist::ArgumentVT( 17.0 );
    args.emplace( "max", 17.0 );
    qA.parseQuery( 10, 100, "sum(o.len)", "(a.max>o.len||o.cow==true) && 'hej' == o.name && o.width < 17", "", args, 0, false );
    qB.parseQuery( 10, 100, "", "(a.max>o.len||o.cow==true) && 'hej' == o.name && o.width < 17", "o.name", args, 0, false );
    qC.parseQuery( 10, 100, "o.len", "(a.max>o.len||o.cow==true) && 'hej' == o.name && o.width < 17", "o.name", args, 0, true );
    qD.parseQuery( 10, 100, "", "(a.max>o.len||o.cow==true) && 'hej' == o.name && o.width < 17", "o.name", args, 17, false );

    std::cout << qA.getSqlQuery() << "\n";
    std::cout << qA.getArgs().size() << "\n";
}
}
