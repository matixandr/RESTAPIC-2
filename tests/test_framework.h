#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <functional>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>

#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

typedef std::function<void()> TestFunction;

struct TestCase {
    std::string name;
    TestFunction function;
    bool passed;
    std::string error_message;
    double execution_time_ms;
};

struct TestSuite {
    std::string name;
    std::vector<TestCase> tests;
    std::function<void()> setup;
    std::function<void()> teardown;
};

class TestFramework {
private:
    static inline std::vector<TestSuite> test_suites;
    static inline TestSuite* current_suite = nullptr;
    static inline int passed_tests = 0;
    static inline int failed_tests = 0;
    static inline std::string current_assertion_message;
public:
    static void suite(const std::string& name) {
        TestSuite suite;
        suite.name = name;
        test_suites.push_back(suite);
        current_suite = &test_suites.back();
    }
    static void test(const std::string& name, TestFunction func) {
        if (!current_suite) {
            suite("Default");
        }
        TestCase test_case;
        test_case.name = name;
        test_case.function = func;
        test_case.passed = true;
        current_suite->tests.push_back(test_case);
    }
    static void setup(std::function<void()> setup_func) {
        if (current_suite) {
            current_suite->setup = setup_func;
        }
    }
    static void teardown(std::function<void()> teardown_func) {
        if (current_suite) {
            current_suite->teardown = teardown_func;
        }
    }
    static bool expect(bool condition, const std::string& message) {
        if (!condition) {
            current_assertion_message = message;
            return false;
        }
        return true;
    }
    static void run_all() {
        auto start_time = std::chrono::high_resolution_clock::now();
        passed_tests = 0;
        failed_tests = 0;
        std::cout << "Starting test execution...\n\n";
        for (auto& suite : test_suites) {
            std::cout << "Suite: " << suite.name << "\n";
            std::cout << "----------------------------------------\n";
            for (auto& test : suite.tests) {
                if (suite.setup) {
                    suite.setup();
                }
                auto test_start = std::chrono::high_resolution_clock::now();
                try {
                    test.function();
                    test.passed = true;
                    passed_tests++;
                    auto test_end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> test_duration = test_end - test_start;
                    test.execution_time_ms = test_duration.count();
                    std::cout << "  " << COLOR_GREEN << "✓" << COLOR_RESET << " "
                              << test.name << " (" << test.execution_time_ms << " ms)\n";
                }
                catch (const std::exception& e) {
                    test.passed = false;
                    test.error_message = e.what();
                    failed_tests++;
                    auto test_end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> test_duration = test_end - test_start;
                    test.execution_time_ms = test_duration.count();
                    std::cout << "  " << COLOR_RED << "✗" << COLOR_RESET << " "
                              << test.name << " (" << test.execution_time_ms << " ms)\n";
                    std::cout << "    Error: " << test.error_message << "\n";
                }
                catch (...) {
                    test.passed = false;
                    test.error_message = "Unknown error";
                    failed_tests++;
                    auto test_end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> test_duration = test_end - test_start;
                    test.execution_time_ms = test_duration.count();
                    std::cout << "  " << COLOR_RED << "✗" << COLOR_RESET << " "
                              << test.name << " (" << test.execution_time_ms << " ms)\n";
                    std::cout << "    Error: Unknown error\n";
                }
                if (suite.teardown) {
                    suite.teardown();
                }
            }
            std::cout << "\n";
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> total_duration = end_time - start_time;
        std::cout << "Test Summary\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Total tests: " << passed_tests + failed_tests << "\n";
        std::cout << "Passed: " << COLOR_GREEN << passed_tests << COLOR_RESET << "\n";
        std::cout << "Failed: " << COLOR_RED << failed_tests << COLOR_RESET << "\n";
        std::cout << "Execution time: " << total_duration.count() << " ms\n";
    }
    static std::string generate_json_report() {
        std::string report = "{\n";
        report += "  \"summary\": {\n";
        report += "    \"total\": " + std::to_string(passed_tests + failed_tests) + ",\n";
        report += "    \"passed\": " + std::to_string(passed_tests) + ",\n";
        report += "    \"failed\": " + std::to_string(failed_tests) + "\n";
        report += "  },\n";
        report += "  \"suites\": [\n";
        for (size_t i = 0; i < test_suites.size(); i++) {
            const auto& suite = test_suites[i];
            report += "    {\n";
            report += "      \"name\": \"" + suite.name + "\",\n";
            report += "      \"tests\": [\n";
            for (size_t j = 0; j < suite.tests.size(); j++) {
                const auto& test = suite.tests[j];
                report += "        {\n";
                report += "          \"name\": \"" + test.name + "\",\n";
                report += "          \"passed\": " + std::string(test.passed ? "true" : "false") + ",\n";
                report += "          \"execution_time_ms\": " + std::to_string(test.execution_time_ms);
                if (!test.error_message.empty()) {
                    report += ",\n          \"error\": \"" + test.error_message + "\"";
                }
                report += "\n        }";
                if (j < suite.tests.size() - 1) report += ",";
                report += "\n";
            }
            report += "      ]\n";
            report += "    }";
            if (i < test_suites.size() - 1) report += ",";
            report += "\n";
        }
        report += "  ]\n";
        report += "}\n";
        return report;
    }
};

#define TEST_SUITE(name) TestFramework::suite(name)
#define TEST_CASE(name) TestFramework::test(name, []()
#define TEST_SETUP TestFramework::setup([]()
#define TEST_TEARDOWN TestFramework::teardown([]()
#define EXPECT(condition) if (!TestFramework::expect(condition, #condition)) throw std::runtime_error("Assertion failed: " #condition)

#endif
