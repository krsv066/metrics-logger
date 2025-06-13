#pragma once

#include "metric.hpp"
#include "lock_free_queue.hpp"

#include <memory>
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace metrics {

struct MetricSnapshot {
    std::string name;
    MetricValue value;
    std::chrono::system_clock::time_point timestamp;
};

class MetricsLogger {
public:
    explicit MetricsLogger(std::string filename, std::chrono::milliseconds flush_interval = std::chrono::milliseconds(1000))
        : filename_(std::move(filename)), flush_interval_(flush_interval), running_(true) {

        output_thread_ = std::thread(&MetricsLogger::OutputLoop, this);
    }

    ~MetricsLogger() noexcept {
        Stop();
    }

    void RegisterMetric(std::shared_ptr<IMetric> metric) {
        metrics_.emplace_back(std::move(metric));
    }

    void Stop() noexcept {
        bool expected = true;
        if (running_.compare_exchange_strong(expected, false)) {
            if (output_thread_.joinable()) {
                output_thread_.join();
            }
        }
    }

private:
    void OutputLoop() noexcept {
        try {
            std::ofstream file(filename_, std::ios::app);
            if (!file.is_open()) {
                return;
            }

            while (running_.load()) {
                CollectMetrics();
                WriteSnapshots(file);
                std::this_thread::sleep_for(flush_interval_);
            }

            CollectMetrics();
            WriteSnapshots(file);
        } catch (...) {
        }
    }

    void CollectMetrics() noexcept {
        try {
            auto now = std::chrono::system_clock::now();

            for (const auto& metric : metrics_) {
                if (metric && metric->HasValue()) {
                    MetricSnapshot snapshot{metric->GetName(), metric->GetAndReset(), now};
                    queue_.Enqueue(std::move(snapshot));
                }
            }
        } catch (...) {
        }
    }

    void WriteSnapshots(std::ofstream& file) noexcept {
        try {
            std::vector<MetricSnapshot> snapshots;
            snapshots.reserve(64);
            MetricSnapshot snapshot;

            while (queue_.Dequeue(snapshot)) {
                snapshots.push_back(std::move(snapshot));
            }

            if (snapshots.empty()) {
                return;
            }

            auto timestamp = snapshots[0].timestamp;
            file << FormatTimestamp(timestamp);

            for (const auto& snap : snapshots) {
                file << " \"" << snap.name << "\" ";
                std::visit([&file](const auto& value) { file << value; }, snap.value);
            }
            file << "\n";
            file.flush();
        } catch (...) {
        }
    }

    std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) const {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    const std::string filename_;
    const std::chrono::milliseconds flush_interval_;
    std::vector<std::shared_ptr<IMetric>> metrics_;
    MPMCBoundedQueue<MetricSnapshot, 4096> queue_;
    std::atomic<bool> running_;
    std::thread output_thread_;
};

}  // namespace metrics
