#pragma once

#include <string>
#include <atomic>
#include <variant>

namespace metrics {

using MetricValue = std::variant<int64_t, double>;

class IMetric {
public:
    virtual ~IMetric() = default;
    virtual std::string GetName() const = 0;
    virtual MetricValue GetAndReset() = 0;
    virtual bool HasValue() const = 0;
};

class Counter : public IMetric {
public:
    explicit Counter(std::string name) : name_(std::move(name)), value_(0) {
    }

    void Increment(int64_t delta = 1) {
        value_.fetch_add(delta);
    }

    std::string GetName() const override {
        return name_;
    }

    MetricValue GetAndReset() override {
        return value_.exchange(0);
    }

    bool HasValue() const override {
        return value_.load() != 0;
    }

private:
    std::string name_;
    std::atomic_int64_t value_;
};

class Gauge : public IMetric {
public:
    explicit Gauge(std::string name) : name_(std::move(name)), has_value_(false) {
    }

    void Set(double value) {
        value_.store(value);
        has_value_.store(true);
    }

    std::string GetName() const override {
        return name_;
    }

    MetricValue GetAndReset() override {
        double val = value_.load();
        has_value_.store(false);
        return val;
    }

    bool HasValue() const override {
        return has_value_.load();
    }

private:
    std::string name_;
    std::atomic<double> value_;
    std::atomic_bool has_value_;
};

}  // namespace metrics
