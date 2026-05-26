#include "lib_base.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "gpu_lib/gpu_lib.h"

std::atomic<bool> stop_flag(false);

void signal_handler(int signal)
{
    (void)signal;
    stop_flag = true;
}

int main(int argc, char* argv[])
{
    TestMode mode = TestMode::SINGLE_SHOT;
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "monitoring") {
            mode = TestMode::MONITORING;
        }
    }

    auto printResults = [&](const std::vector<TestResult>& results) {
        if (mode == TestMode::MONITORING) {
            system("clear");
        }
        static int cycle = 0;
        cycle++;
        std::cout << "\nTest Results (Cycle " << cycle << "):\n";
        std::cout << std::left << std::setw(40) << "Test Name" << std::setw(10) << "Result" << std::setw(15) << "Duration (s)" << std::endl;
        std::cout << std::string(65, '-') << std::endl;
        int passed = 0;
        int failed = 0;
        std::chrono::duration<double> totalTime = std::chrono::duration<double>::zero();
        for (const auto& res : results) {
            std::cout << std::left << std::setw(40) << res.name << std::setw(10) << (res.passed ? "PASS" : "FAIL") << std::setw(15)
                      << std::fixed << std::setprecision(6) << res.duration.count() << std::endl;
            if (res.passed)
                passed++;
            else
                failed++;
            totalTime += res.duration;
        }
        std::cout << std::string(65, '-') << std::endl;
        std::cout << "Total Tests Run: " << results.size() << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Total Time: " << std::fixed << std::setprecision(6) << totalTime.count() << " seconds" << std::endl;
    };

    GpuTesting tester;
    std::cout << "GpuTesting test started in " << (mode == TestMode::SINGLE_SHOT ? "SINGLE_SHOT" : "MONITORING") << " mode." << std::endl;

    if (mode == TestMode::MONITORING) {
        std::signal(SIGINT, signal_handler);
        std::cout << "Press Ctrl+C to stop monitoring." << std::endl;
    }

    // EXAMPLE: Run SELECTED tests in custom order (1, 10, 15, 19)
    std::cout << "\n=== Example: Running Selected Tests ===\n" << std::endl;
    std::vector<std::string> selectedTests = {
        "testCudaCoresFunctionality",       // Test 1
        "testL2Cache",                      // Test 10
        "testFp32ComputePerformance",       // Test 15
        "testGpuThermalThrottlingBehavior", // Test 19
    };
    auto results = tester.runSelectedTests(selectedTests);
    printResults(results);

    // EXAMPLE: Run individual tests one by one
    std::cout << "\n\n=== Example: Running Individual Tests ===\n" << std::endl;
    std::vector<TestResult> individualResults;
    individualResults.push_back(tester.runSingleTest("testTensorCoresOperation"));
    individualResults.push_back(tester.runSingleTest("testMemoryBandwidthToGpu"));
    printResults(individualResults);

    if (mode == TestMode::MONITORING) {
        // Wait for stop signal
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        tester.stop();
        std::cout << "Monitoring stopped." << std::endl;
    }

    std::cout << "GpuTesting test completed." << std::endl;
    return 0;
}
