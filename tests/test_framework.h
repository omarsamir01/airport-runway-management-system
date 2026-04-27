#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
using namespace std;

/*
    A tiny test framework written from scratch. No exceptions are used:
    when an ASSERT macro fires it sets a global failure flag and `return`s
    out of the test body. The runner inspects the flag after each test.

    How to use:

        TEST_CASE("descriptive name") {
            ASSERT_EQ(actual, expected);
            ASSERT_TRUE(condition);
            ASSERT_FALSE(condition);
        }

        // In test_main.cpp:
        int main() { return runAllTests(); }

    Each TEST_CASE expands to a static function plus an automatic
    registration object that pushes the test into a global list.
    runAllTests() walks the list and reports pass/fail counts.
*/

// One registered test: a name and a function pointer to its body.
struct TestCase {
    string name;
    void (*body)();
};

// The single shared list of every registered test case (defined in
// test_framework.cpp). Function-local static so order of construction
// across translation units does not matter.
vector<TestCase>& getTestRegistry();

// Carries the failure result of the test currently running.
// Reset before each test by runAllTests, written by ASSERT macros.
struct TestState {
    bool   failed;
    string message;
};
TestState& currentTestState();

// Helper class: every TEST_CASE creates one of these at file scope,
// and its constructor adds the test to the registry.
struct TestRegistrar {
    TestRegistrar(string name, void (*body)());
};

// Runs every registered test, prints results, and returns
// 0 if all passed, 1 if any failed.
int runAllTests();

// Helper macros to glue tokens together so each TEST_CASE
// produces a unique function name.
#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)

#define TEST_CASE(name)                                                        \
    static void TEST_CONCAT(_test_body_, __LINE__)();                          \
    static TestRegistrar TEST_CONCAT(_test_reg_, __LINE__)(                    \
        name, &TEST_CONCAT(_test_body_, __LINE__));                            \
    static void TEST_CONCAT(_test_body_, __LINE__)()

// On failure each macro:
//   1. Builds a message describing what failed.
//   2. Stores it in currentTestState().
//   3. Returns from the enclosing test body so the rest is skipped.

#define ASSERT_TRUE(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            stringstream _oss;                                                 \
            _oss << "ASSERT_TRUE failed: " #cond                               \
                 << "  (" << __FILE__ << ":" << __LINE__ << ")";               \
            currentTestState().failed  = true;                                 \
            currentTestState().message = _oss.str();                           \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(actual, expected)                                            \
    do {                                                                       \
        auto _a = (actual);                                                    \
        auto _e = (expected);                                                  \
        if (!(_a == _e)) {                                                     \
            stringstream _oss;                                                 \
            _oss << "ASSERT_EQ failed: " #actual " == " #expected              \
                 << "\n           actual:   " << _a                            \
                 << "\n           expected: " << _e                            \
                 << "\n           at " << __FILE__ << ":" << __LINE__;         \
            currentTestState().failed  = true;                                 \
            currentTestState().message = _oss.str();                           \
            return;                                                            \
        }                                                                      \
    } while (0)

#endif
