/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <gtest/gtest.h> // Google test framework

// Logger
#include <memory>
#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/std2_make_unique.hpp>
#include "Helper.h"

std::unique_ptr<g3::LogWorker> logWorker;
std::unique_ptr<g3::SinkHandle<g3::FileSink>> logHandle;

TEST(InitAllTest, InitLogger) {
	const std::string dir = "./log/";
	const std::string file = "testAll";
	logWorker = g3::LogWorker::createLogWorker();
	logHandle = logWorker->addDefaultLogger(file, dir);
	g3::initializeLogging(logWorker.get());
	LOG( INFO ) << "Starting test suite.";
}

