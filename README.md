# Metrics Logger Library

C++23 library for multithreaded metrics collection and logging using lock-free queue.

## Quick Start

```cpp
#include "metrics_logger.hpp"

int main() {
    metrics::MetricsLogger logger("metrics.log");
    
    auto cpu_metric = std::make_shared<metrics::Gauge>("CPU");
    auto requests = std::make_shared<metrics::Counter>("HTTP requests");
    
    logger.RegisterMetric(cpu_metric);
    logger.RegisterMetric(requests);
    
    cpu_metric->Set(0.97);
    requests->Increment(42);
    
    return 0;
}
```

## API

### Counter
```cpp
metrics::Counter counter("requests");
counter.Increment();        // +1
counter.Increment(5);       // +5
auto value = counter.GetAndReset();  // get and reset
```

### Gauge
```cpp
metrics::Gauge gauge("CPU");
gauge.Set(0.85);
auto value = gauge.GetAndReset();  // get and reset
```
### Custom Metrics

You can add custom metric types by implementing the `IMetric` interface:

## Examples and Tests

Comprehensive usage examples and test cases can be found in `examples_and_tests/main.cpp`. 
This file includes:
- Lock-free queue tests
- Metric functionality tests
- Multithreaded usage examples
- Edge case testing

## Build

```bash
mkdir build && cd build
cmake ..
make
./bin/examples_and_tests
```

## Testing

Tested on Linux with:
- gcc 13.3.0
- clang 19.1.7

All tests pass with AddressSanitizer/ThreadSanitizer/O3 enabled.

