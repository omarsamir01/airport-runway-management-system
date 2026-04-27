#include "test_framework.h"
using namespace std;

// Function-local statics: one shared instance for the whole program,
// initialized on first call. Keeps construction-order issues away.

vector<TestCase>& getTestRegistry() {
    static vector<TestCase> registry;
    return registry;
}

TestState& currentTestState() {
    static TestState s;
    return s;
}

TestRegistrar::TestRegistrar(string name, void (*body)()) {
    TestCase tc;
    tc.name = name;
    tc.body = body;
    getTestRegistry().push_back(tc);
}

int runAllTests() {
    int passed = 0;
    int failed = 0;

    vector<TestCase>& reg = getTestRegistry();
    for (int i = 0; i < (int)reg.size(); i++) {
        TestState& s = currentTestState();
        s.failed  = false;
        s.message = "";

        // Invoke the test body. If an ASSERT inside it fires it sets
        // s.failed/s.message and returns early; otherwise it runs to
        // completion and the test counts as passing.
        reg[i].body();

        if (s.failed) {
            cout << "  [FAIL] " << reg[i].name << endl
                 << "         " << s.message << endl;
            failed++;
        } else {
            cout << "  [PASS] " << reg[i].name << endl;
            passed++;
        }
    }

    cout << endl
         << "----------------------------------------" << endl
         << "Total: "  << (passed + failed)
         << "   Passed: " << passed
         << "   Failed: " << failed << endl;

    return failed == 0 ? 0 : 1;
}
