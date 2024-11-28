#pragma once 

#include <iostream>

// ANSI escape codes for colors
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

// Macro to log the result with the name of the function being tested
#define LOG_TEST_RESULT(testFunc) logTestResult((testFunc()), #testFunc)

// Helper function to log the result
void logTestResult(bool result, const char* testName) {
    if (result) {
        std::cout << COLOR_GREEN << "[PASS]: " << COLOR_RESET << testName << '\n';
    } else {
        std::cout << COLOR_RED << "[FAIL]: " << COLOR_RESET << testName << '\n';
    }
}

// Custom test_test_assertion macro
#define TEST_ASSERT(condition) \
    if (!(condition)) { \
        std::cerr << "assertion failed: " << #condition \
                  << " at " <<  "line: " << __LINE__ << COLOR_RESET << '\n'; \
        return false; \
    }

void basicEcsSpeedTest(int amount);

bool createEntitiesTest();

bool addingComponentTest();

bool removingComponentTest();

bool initializeComponentTest();

bool gettingComponentNameTest();

bool entityGUIDTest();

bool referencesTest();

bool referencesTest();

bool forEachTest();

bool serializationTest();

bool parentingTest();

bool clearingTest();

bool removingEntityTest();