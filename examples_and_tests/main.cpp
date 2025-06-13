#include "../include/metrics_logger.hpp"

#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include <atomic>

void TestQueueEnqueue() {
    std::cout << "Testing Queue Enqueue..." << std::endl;

    metrics::MPMCBoundedQueue<int, 2> queue;
    assert(queue.Enqueue(2));
    assert(queue.Enqueue(2));
    assert(!queue.Enqueue(2));
    assert(!queue.Enqueue(2));

    std::cout << "Queue Enqueue tests passed!" << std::endl;
}

void TestQueueDequeue() {
    std::cout << "Testing Queue Dequeue..." << std::endl;

    int val;
    metrics::MPMCBoundedQueue<int, 2> queue;
    assert(!queue.Dequeue(val));
    assert(!queue.Dequeue(val));

    std::cout << "Queue Dequeue tests passed!" << std::endl;
}

void TestQueueEmpty() {
    std::cout << "Testing Queue Empty..." << std::endl;

    metrics::MPMCBoundedQueue<int, 4> queue;
    assert(queue.Empty());

    assert(queue.Enqueue(1));
    assert(!queue.Empty());

    int val;
    assert(queue.Dequeue(val));
    assert(queue.Empty());

    assert(queue.Enqueue(1));
    assert(queue.Enqueue(2));
    assert(!queue.Empty());

    assert(queue.Dequeue(val));
    assert(!queue.Empty());
    assert(queue.Dequeue(val));
    assert(queue.Empty());

    std::cout << "Queue Empty tests passed!" << std::endl;
}

void TestQueueEnqueueDequeue() {
    std::cout << "Testing Queue EnqueueDequeue..." << std::endl;

    int val = 0;
    metrics::MPMCBoundedQueue<int, 2> queue;
    assert(queue.Enqueue(1));
    assert(queue.Dequeue(val));
    assert(val == 1);
    assert(!queue.Dequeue(val));

    assert(queue.Enqueue(2));
    assert(queue.Enqueue(3));
    assert(!queue.Enqueue(4));

    assert(queue.Dequeue(val));
    assert(val == 2);
    assert(queue.Dequeue(val));
    assert(val == 3);

    assert(!queue.Dequeue(val));

    std::cout << "Queue EnqueueDequeue tests passed!" << std::endl;
}

void TestQueueNoSpuriousFails() {
    std::cout << "Testing Queue NoSpuriousFails..." << std::endl;

    const int n = 256;
    const int n_threads = 4;
    metrics::MPMCBoundedQueue<int, 2048> queue;

    std::vector<std::thread> writers;
    for (int i = 0; i < n_threads; i++) {
        writers.emplace_back([&] {
            for (int j = 0; j < n; ++j) {
                assert(queue.Enqueue(0));
            }
        });
    }

    for (auto& t : writers) {
        t.join();
    }

    std::vector<std::thread> readers;
    for (int i = 0; i < n_threads; i++) {
        readers.emplace_back([&] {
            for (int j = 0; j < n; ++j) {
                int k;
                assert(queue.Dequeue(k));
            }
        });
    }

    for (auto& t : readers) {
        t.join();
    }

    std::cout << "Queue NoSpuriousFails tests passed!" << std::endl;
}

void TestQueueNoQueueLock() {
    std::cout << "Testing Queue NoQueueLock..." << std::endl;

    const int n = 256;
    const int n_threads = 4;
    metrics::MPMCBoundedQueue<int, 512> queue;

    std::vector<std::thread> threads;
    std::atomic_int ids{0};
    for (int i = 0; i < n_threads; i++) {
        threads.emplace_back([&] {
            int id = ids++;
            if (id % 2) {
                for (int j = 0; j < n; ++j) {
                    queue.Enqueue(0);
                }
            } else {
                for (int j = 0; j < n; ++j) {
                    int k;
                    queue.Dequeue(k);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    int k;
    while (queue.Dequeue(k)) {
    }
    assert(queue.Enqueue(0));
    assert(queue.Dequeue(k));
    assert(k == 0);

    std::cout << "Queue NoQueueLock tests passed!" << std::endl;
}

void TestQueueMoveSemantics() {
    std::cout << "Testing Queue Move Semantics..." << std::endl;

    metrics::MPMCBoundedQueue<std::string, 4> queue;

    std::string str1 = "test1";
    std::string str2 = "test2";

    assert(queue.Enqueue(std::move(str1)));
    assert(str1.empty() || str1 == "test1");

    assert(queue.Enqueue(std::string("test3")));

    std::string result;
    assert(queue.Dequeue(result));
    assert(result == "test1");

    assert(queue.Dequeue(result));
    assert(result == "test3");

    std::cout << "Queue Move Semantics tests passed!" << std::endl;
}

void TestCounter() {
    std::cout << "Testing Counter metric..." << std::endl;

    metrics::Counter counter("test_counter");
    assert(!counter.HasValue());

    counter.Increment();
    assert(counter.HasValue());

    auto value = counter.GetAndReset();
    assert(std::get<int64_t>(value) == 1);
    assert(!counter.HasValue());

    counter.Increment(5);
    counter.Increment(3);
    value = counter.GetAndReset();
    assert(std::get<int64_t>(value) == 8);

    std::cout << "Counter tests passed!" << std::endl;
}

void TestCounterEdgeCases() {
    std::cout << "Testing Counter edge cases..." << std::endl;

    metrics::Counter counter("edge_counter");

    counter.Increment(0);
    assert(!counter.HasValue());

    counter.Increment(-5);
    assert(counter.HasValue());
    auto value = counter.GetAndReset();
    assert(std::get<int64_t>(value) == -5);
    assert(!counter.HasValue());

    counter.Increment(1000000);
    assert(counter.HasValue());
    value = counter.GetAndReset();
    assert(std::get<int64_t>(value) == 1000000);

    std::cout << "Counter edge cases tests passed!" << std::endl;
}

void TestGauge() {
    std::cout << "Testing Gauge metric..." << std::endl;

    metrics::Gauge gauge("test_gauge");
    assert(!gauge.HasValue());

    gauge.Set(3.14);
    assert(gauge.HasValue());

    auto value = gauge.GetAndReset();
    assert(std::get<double>(value) == 3.14);
    assert(!gauge.HasValue());

    gauge.Set(2.71);
    gauge.Set(1.41);
    value = gauge.GetAndReset();
    assert(std::get<double>(value) == 1.41);

    std::cout << "Gauge tests passed!" << std::endl;
}

void TestGaugeEdgeCases() {
    std::cout << "Testing Gauge edge cases..." << std::endl;

    metrics::Gauge gauge("edge_gauge");

    gauge.Set(0.0);
    assert(gauge.HasValue());
    auto value = gauge.GetAndReset();
    assert(std::get<double>(value) == 0.0);
    assert(!gauge.HasValue());

    gauge.Set(-3.14);
    assert(gauge.HasValue());
    value = gauge.GetAndReset();
    assert(std::get<double>(value) == -3.14);

    gauge.Set(1e-10);
    assert(gauge.HasValue());
    value = gauge.GetAndReset();
    assert(std::get<double>(value) == 1e-10);

    gauge.Set(1e10);
    assert(gauge.HasValue());
    value = gauge.GetAndReset();
    assert(std::get<double>(value) == 1e10);

    std::cout << "Gauge edge cases tests passed!" << std::endl;
}

void TestMultithreadedCounter() {
    std::cout << "Testing multithreaded Counter..." << std::endl;

    metrics::Counter counter("mt_counter");
    std::vector<std::thread> threads;
    const int num_threads = 4;
    const int increments_per_thread = 1000;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.Increment();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto value = counter.GetAndReset();
    assert(std::get<int64_t>(value) == num_threads * increments_per_thread);

    std::cout << "Multithreaded Counter tests passed!" << std::endl;
}

void TestLoggerBasic() {
    std::cout << "Testing basic logger functionality..." << std::endl;

    const std::string test_file = "test_metrics.log";

    {
        metrics::MetricsLogger logger(test_file, std::chrono::milliseconds(100));

        auto counter = std::make_shared<metrics::Counter>("test_requests");
        auto gauge = std::make_shared<metrics::Gauge>("test_cpu");

        logger.RegisterMetric(counter);
        logger.RegisterMetric(gauge);

        counter->Increment(42);
        gauge->Set(0.85);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        counter->Increment(10);
        gauge->Set(0.92);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::ifstream file(test_file);
    assert(file.is_open());

    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        line_count++;
        assert(!line.empty());
        assert(line.find("test_requests") != std::string::npos || line.find("test_cpu") != std::string::npos);
    }

    assert(line_count >= 2);

    std::cout << "Basic logger tests passed!" << std::endl;
}

void TestLoggerStopStart() {
    std::cout << "Testing Logger Stop functionality..." << std::endl;

    const std::string test_file = "test_stop_metrics.log";

    auto counter = std::make_shared<metrics::Counter>("stop_test_counter");

    {
        metrics::MetricsLogger logger(test_file, std::chrono::milliseconds(50));
        logger.RegisterMetric(counter);

        counter->Increment(10);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        logger.Stop();

        counter->Increment(20);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        logger.Stop();
    }

    std::cout << "Logger Stop tests passed!" << std::endl;
}

void TestLoggerMultipleMetrics() {
    std::cout << "Testing Logger with multiple metrics..." << std::endl;

    const std::string test_file = "test_multiple_metrics.log";

    {
        metrics::MetricsLogger logger(test_file, std::chrono::milliseconds(100));

        auto counter1 = std::make_shared<metrics::Counter>("counter1");
        auto counter2 = std::make_shared<metrics::Counter>("counter2");
        auto gauge1 = std::make_shared<metrics::Gauge>("gauge1");
        auto gauge2 = std::make_shared<metrics::Gauge>("gauge2");

        logger.RegisterMetric(counter1);
        logger.RegisterMetric(counter2);
        logger.RegisterMetric(gauge1);
        logger.RegisterMetric(gauge2);

        counter1->Increment(5);
        counter2->Increment(10);
        gauge1->Set(1.5);
        gauge2->Set(2.5);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        counter1->Increment(3);
        gauge1->Set(3.5);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::ifstream file(test_file);
    assert(file.is_open());

    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        line_count++;
        assert(!line.empty());
    }

    assert(line_count >= 2);

    std::cout << "Multiple metrics tests passed!" << std::endl;
}

void TestEmptyMetricsLogger() {
    std::cout << "Testing Logger with no metrics..." << std::endl;

    const std::string test_file = "test_empty_metrics.log";

    {
        metrics::MetricsLogger logger(test_file, std::chrono::milliseconds(50));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::ifstream file(test_file);
    if (file.is_open()) {
        std::string line;
        if (std::getline(file, line)) {
            assert(line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos);
        }
    }

    std::cout << "Empty metrics logger tests passed!" << std::endl;
}

void TestQueueSizeAssertion() {
    std::cout << "Testing Queue size assertion..." << std::endl;

    metrics::MPMCBoundedQueue<int, 1> queue1;
    metrics::MPMCBoundedQueue<int, 2> queue2;
    metrics::MPMCBoundedQueue<int, 4> queue4;
    metrics::MPMCBoundedQueue<int, 1024> queue1024;

    assert(queue1.Empty());
    assert(queue2.Empty());
    assert(queue4.Empty());
    assert(queue1024.Empty());

    std::cout << "Queue size assertion tests passed!" << std::endl;
}

void RunAllTests() {
    std::cout << "=== Running Tests ===" << std::endl;

    TestQueueEnqueue();
    TestQueueDequeue();
    TestQueueEmpty();
    TestQueueEnqueueDequeue();
    TestQueueNoSpuriousFails();
    TestQueueNoQueueLock();
    TestQueueMoveSemantics();
    TestQueueSizeAssertion();

    TestCounter();
    TestGauge();
    TestCounterEdgeCases();
    TestGaugeEdgeCases();
    TestMultithreadedCounter();

    TestLoggerBasic();
    TestLoggerStopStart();
    TestLoggerMultipleMetrics();
    TestEmptyMetricsLogger();

    std::cout << "=== All Tests Passed! ===" << std::endl << std::endl;
}

void RunExamples() {
    std::cout << "=== Running Examples ===" << std::endl;

    metrics::MetricsLogger logger("metrics.log");

    auto cpu_metric = std::make_shared<metrics::Gauge>("CPU");
    auto http_requests = std::make_shared<metrics::Counter>("HTTP requests RPS");
    auto memory_usage = std::make_shared<metrics::Gauge>("Memory Usage MB");
    auto errors = std::make_shared<metrics::Counter>("Error Count");

    logger.RegisterMetric(cpu_metric);
    logger.RegisterMetric(http_requests);
    logger.RegisterMetric(memory_usage);
    logger.RegisterMetric(errors);

    std::vector<std::thread> threads;
    std::atomic_bool stop_threads{false};

    threads.emplace_back([&cpu_metric, &stop_threads]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 4.0);

        while (!stop_threads.load()) {
            cpu_metric->Set(dis(gen));
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    threads.emplace_back([&memory_usage, &stop_threads]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(512.0, 2048.0);

        while (!stop_threads.load()) {
            memory_usage->Set(dis(gen));
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });

    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&http_requests, &stop_threads]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 15);

            while (!stop_threads.load()) {
                http_requests->Increment(dis(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            }
        });
    }

    threads.emplace_back([&errors, &stop_threads]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 100);

        while (!stop_threads.load()) {
            if (dis(gen) < 5) {
                errors->Increment();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    });

    std::cout << "Running example simulation for 8 seconds..." << std::endl;
    std::cout << "Check 'metrics.log' for output" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(8));

    stop_threads.store(true);
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "=== Examples Completed ===" << std::endl;
}

int main() {
    RunAllTests();
    RunExamples();

    return 0;
}
