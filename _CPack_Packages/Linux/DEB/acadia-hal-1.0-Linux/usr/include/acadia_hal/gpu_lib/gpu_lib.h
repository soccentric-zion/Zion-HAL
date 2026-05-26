/**
 * @file gpu_lib.h
 * @brief Header file for GpuTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef GPUTESTING_H
#define GPUTESTING_H

#include "lib_base.h"

/**
 * @class GpuTesting
 * @brief Class for testing GPU functionalities
 * @details This class provides methods to test various GPU functionalities.
 */
class GpuTesting : public TestBase
{
  public:
    /**
     * @brief Constructor for GpuTesting
     */
    GpuTesting();

    /**
     * @brief Destructor for GpuTesting
     */
    ~GpuTesting();

    /**
     * @brief Tests CUDA cores functionality
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCudaCoresFunctionality();

    /**
     * @brief Tests tensor cores operation
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTensorCoresOperation();

    /**
     * @brief Tests GPU clock frequency stability and throttling
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpuClockFrequencyStabilityAndThrottling();

    /**
     * @brief Tests graphics processing clusters (both clusters)
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGraphicsProcessingClustersBothClusters();

    /**
     * @brief Tests texture processing clusters (all 8 TPCs)
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTextureProcessingClustersAll8Tpcs();

    /**
     * @brief Tests streaming multiprocessors (all 16 SMs)
     * @return TestOutput with pass/fail and data
     */
    TestOutput testStreamingMultiprocessorsAll16Sms();

    /**
     * @brief Tests render output units (all 32 ROPs)
     * @return TestOutput with pass/fail and data
     */
    TestOutput testRenderOutputUnitsAll32Rops();

    /**
     * @brief Tests texture mapping units (all 64 TMUs)
     * @return TestOutput with pass/fail and data
     */
    TestOutput testTextureMappingUnitsAll64Tmus();

    /**
     * @brief Tests L1 cache per SM
     * @return TestOutput with pass/fail and data
     */
    TestOutput testL1CachePerSm();

    /**
     * @brief Tests L2 cache
     * @return TestOutput with pass/fail and data
     */
    TestOutput testL2Cache();

    /**
     * @brief Tests cache coherency and hit rates
     * @return TestOutput with pass/fail and data
     */
    TestOutput testCacheCoherencyAndHitRates();

    /**
     * @brief Tests memory bandwidth to GPU
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMemoryBandwidthToGpu();

    /**
     * @brief Tests INT8 performance verification
     * @return TestOutput with pass/fail and data
     */
    TestOutput testInt8PerformanceVerification();

    /**
     * @brief Tests INT8 sparse performance
     * @return TestOutput with pass/fail and data
     */
    TestOutput testInt8SparsePerformance();

    /**
     * @brief Tests FP32 compute performance
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFp32ComputePerformance();

    /**
     * @brief Tests FP16 compute performance
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFp16ComputePerformance();

    /**
     * @brief Tests FP16 tensor core performance
     * @return TestOutput with pass/fail and data
     */
    TestOutput testFp16TensorCorePerformance();

    /**
     * @brief Tests mixed precision operations
     * @return TestOutput with pass/fail and data
     */
    TestOutput testMixedPrecisionOperations();

    /**
     * @brief Tests GPU thermal throttling behavior
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpuThermalThrottlingBehavior();

    /**
     * @brief Tests GPU power consumption at various loads
     * @return TestOutput with pass/fail and data
     */
    TestOutput testGpuPowerConsumptionAtVariousLoads();

    /**
     * @brief Tests sustained performance over time
     * @return TestOutput with pass/fail and data
     */
    TestOutput testSustainedPerformanceOverTime();

    /**
     * @brief Get list of all available test names
     * @return Vector of test names
     */
    std::vector<std::string> getAvailableTests() override;

    /**
     * @brief Run a single test by name
     * @param testName Name of the test to run
     * @return Test result with timing
     */
    TestResult runSingleTest(const std::string& testName) override;

  protected:
    /**
     * @brief Run all GPU tests
     * @return Vector of test results with timing
     */
    std::vector<TestResult> runAllTests() override;
};

#endif // GPUTESTING_H
