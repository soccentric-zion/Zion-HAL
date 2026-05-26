#include "../../src/gpu_lib/gpu_lib.h"
#include "../../src/lib_base.h"
#include <iostream>
#include <vector>

int main()
{
    GpuTesting tester;

    std::cout << "=== Demonstrating Selective Test Execution ===\n\n";

    // Get all available tests
    auto allTests = tester.getAvailableTests();
    std::cout << "Available tests (" << allTests.size() << " total):\n";
    for (size_t i = 0; i < allTests.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << allTests[i] << "\n";
    }

    // Run selected tests in custom order: 1, 3, 5, 4
    std::cout << "\n=== Running tests in order: 1, 3, 5, 4 ===\n";
    std::vector<std::string> selectedTests = {
        allTests[0], // Test 1
        allTests[2], // Test 3
        allTests[4], // Test 5
        allTests[3], // Test 4
    };

    auto results = tester.runSelectedTests(selectedTests);

    std::cout << "\nResults:\n";
    for (const auto& result : results) {
        std::cout << "  " << result.name << ": " << (result.passed ? "PASS" : "FAIL") << "\n";
    }

    return 0;
}
