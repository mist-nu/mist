cmake_minimum_required(VERSION 2.8.12)

# GCC required to be in system path
# TODO: do NOT set the compiler and make this more cross platform
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(test "Build all tests." OFF) # Makes boolean 'test' available.

# Make PROJECT_SOURCE_DIR, PROJECT_BINARY_DIR, and PROJECT_NAME available.
set(PROJECT_NAME Mist)
project(${PROJECT_NAME})

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    if(test)
        set(CMAKE_BUILD_TYPE Debug)
    else()
        set(CMAKE_BUILD_TYPE Release)
    endif()
endif()

# TODO: make this more cross platform
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11")

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g2 -ggdb -pedantic -frtti -fexceptions -Wall -Wextra")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHsc -W3")
endif(CMAKE_COMPILER_IS_GNUCXX)

# If you want your own include/ directory, set this, and then you can do
# include_directories(${COMMON_INCLUDES}) in other CMakeLists.txt files.
# set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)

set(PROJECT_ROOT ..)

##############
# SQLite
##############
# Disable dynamic extensions for sqlite with DSQLITE_OMIT_LOAD_EXTENSION (removes libdl dependency)
add_definitions(
	-DSQLITE_OMIT_LOAD_EXTENSION 
	-DSQLITE_ENABLE_COLUMN_METADATA 
	-DSQLITE_ENABLE_DBSTAT_VTAB 
	-DSQLITE_ENABLE_FTS3 
	-DSQLITE_ENABLE_FTS3_PARENTHESIS 
	-DSQLITE_ENABLE_FTS4 
	-DSQLITE_ENABLE_FTS5 
	-DSQLITE_ENABLE_JSON1 
	-DSQLITE_ENABLE_STAT4 
	-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT
	)
set(SQLITE3_LIB_SOURCE ${PROJECT_ROOT}/lib/sqlite3)
set(SQLITECPP_LIB_SOURCE ${PROJECT_ROOT}/lib/sqlitecpp/src)
include_directories( ${PROJECT_ROOT}/lib/sqlite3 ${PROJECT_ROOT}/lib/sqlitecpp/include )

##############
# Boost
##############
find_package(Boost 1.50.0 REQUIRED COMPONENTS filesystem )
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)

##############
# g3log
##############
set( G3LOG_LIB_INCLUDE_DIRS ${PROJECT_ROOT}/lib/g3log/src )
set( G3LOG_LIB ${PROJECT_ROOT}/lib/g3log/build )
#find_library( ${G3LOG_LIB} g3logger )
link_directories( ${G3LOG_LIB} )
include_directories( ${G3LOG_LIB_INCLUDE_DIRS} )
#add_definitions( -DCHANGE_G3LOG_DEBUG_TO_DBUG )

##############
# mist_conn
##############
add_subdirectory(${PROJECT_ROOT}/lib/mist_conn "${CMAKE_CURRENT_BINARY_DIR}/mist_conn_build")
set( MISTCONN_INCLUDE_DIRS ${PROJECT_ROOT}/lib/mist_conn/include )
include_directories( ${MISTCONN_INCLUDE_DIRS} )

################################
# Normal Libraries & Executables
################################
set(MIST_SOURCE ${PROJECT_ROOT}/src)
include_directories(${PROJECT_ROOT}/include)
add_library(mist STATIC 
	${SQLITE3_LIB_SOURCE}/sqlite3.c
	
	${SQLITECPP_LIB_SOURCE}/Backup.cpp
	${SQLITECPP_LIB_SOURCE}/Column.cpp
	${SQLITECPP_LIB_SOURCE}/Database.cpp
	${SQLITECPP_LIB_SOURCE}/Exception.cpp
	${SQLITECPP_LIB_SOURCE}/Statement.cpp
	${SQLITECPP_LIB_SOURCE}/Transaction.cpp
	
	${MIST_SOURCE}/Central.cpp
	${MIST_SOURCE}/CryptoHelper.cpp
	${MIST_SOURCE}/Database.cpp
	${MIST_SOURCE}/Error.cpp
	${MIST_SOURCE}/Exception.cpp
	${MIST_SOURCE}/ExchangeFormat.cpp
	${MIST_SOURCE}/Helper.cpp
	${MIST_SOURCE}/JSONstream.cpp
	${MIST_SOURCE}/PrivateUserAccount.cpp
	${MIST_SOURCE}/Query.cpp
	${MIST_SOURCE}/RemoteTransaction.cpp
	${MIST_SOURCE}/Transaction.cpp
	${MIST_SOURCE}/UserAccount.cpp
	)

# -- not needed anymore, disabled dynamic extensions for sqlite. TODO: make this more cross platform
#target_link_libraries(mist -lpthread -lm -lrt -ldl)

# TODO: The following line isn't working.
#target_compile_options(mist BEFORE PUBLIC "-g -Wall -Wextra -pedantic -fexception -std=c++11")
	
# add_executable(mist main.cpp)
# Key idea: SEPARATE OUT your main() function into its own file so it can be its
# own executable. Separating out main() means you can add this library to be
# used elsewhere.

# target_link_libraries(mist mist_lib)
target_link_libraries(mist g3logger )

################################
# Testing
################################
if (test)
  enable_testing()
  #include(CTest)

  ##############
  # Google Test
  ##############
  # This adds another subdirectory, which has 'project(gtest)'.
  add_subdirectory(${PROJECT_ROOT}/googletest/googletest gtest)

  # Include the gtest library. gtest_SOURCE_DIR is available due to
  # 'project(gtest)' above.
  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  
  ##############
  # Unit Tests
  ##############
  
  add_executable(testAll
    testAll.cpp
    testQuery.cpp
    testJson.cpp
    testDatabase.cpp
    testTransaction.cpp
    #testRemoteTransaction.cpp
    testConn.cpp
    testCentral.cpp
    #testExchangeFormat.cpp
  )
  # Standard linking to gtest stuff.
  target_link_libraries(testAll ${Boost_LIBRARIES} gtest_main mist mist_conn)
  
  add_executable( remoteSimulator
  	testAll.cpp
  	remoteSimulator.cpp
  )
  target_link_libraries( remoteSimulator ${Boost_LIBRARIES} gtest_main mist mist_conn )
  
  add_executable( localSimulator
  	testAll.cpp
  	localSimulator.cpp
  )
  target_link_libraries( localSimulator ${Boost_LIBRARIES} gtest_main mist mist_conn )
  
  add_executable( simulator1
  	testAll.cpp
  	simulator1.cpp
  )
  target_link_libraries( simulator1 ${Boost_LIBRARIES} gtest_main mist mist_conn )

  add_executable( simulator2
  	testAll.cpp
  	simulator2.cpp
  )
  target_link_libraries( simulator2 ${Boost_LIBRARIES} gtest_main mist mist_conn )
  
  add_executable( simulator3
  	testAll.cpp
  	simulator3.cpp
  )
  target_link_libraries( simulator3 ${Boost_LIBRARIES} gtest_main mist mist_conn )
  
  add_executable( simulator4
  	testAll.cpp
  	simulator4.cpp
  )
  target_link_libraries( simulator4 ${Boost_LIBRARIES} gtest_main mist mist_conn )
  
  # This is so you can do 'make test' to see all your tests run, instead of
  # manually running the executable runUnitTests to see those specific tests.
  # add_test(NAME that-test-I-made COMMAND runTests)

  # You can also omit NAME and COMMAND. The second argument could be some other
  # test executable.
  #add_test(DatabaseTest testDatabase)
  
  # If the gtest module is loaded you can use the following:
  #GTEST_ADD_TESTS(DatabaseTest testAll)
endif()
