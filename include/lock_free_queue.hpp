#pragma once

#include <atomic>
#include <array>

namespace metrics {

template <class T, size_t Size = 4096>
class MPMCBoundedQueue {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

public:
    explicit MPMCBoundedQueue() : mask_(Size - 1), head_(0), tail_(0) {
        for (size_t i = 0; i < Size; ++i) {
            data_[i].gen.store(i);
        }
    }

    bool Enqueue(const T& value) {
        return EnqueueImpl(value);
    }

    bool Enqueue(T&& value) {
        return EnqueueImpl(std::move(value));
    }

    bool Dequeue(T& data) {
        size_t head_pos = head_.load();

        while (true) {
            Elem& element = data_[head_pos & mask_];
            size_t generation = element.gen.load();

            if (generation == head_pos + 1) {
                if (head_.compare_exchange_weak(head_pos, head_pos + 1)) {
                    data = std::move(element.val);
                    element.gen.store(head_pos + mask_ + 1);
                    return true;
                }
            } else if (generation < head_pos + 1) {
                return false;
            } else {
                head_pos = head_.load();
            }
        }
    }

    bool Empty() const {
        return head_.load() == tail_.load();
    }

private:
    template <typename U>
    bool EnqueueImpl(U&& value) {
        size_t tail_pos = tail_.load();

        while (true) {
            Elem& element = data_[tail_pos & mask_];
            size_t generation = element.gen.load();

            if (generation == tail_pos) {
                if (tail_.compare_exchange_weak(tail_pos, tail_pos + 1)) {
                    element.val = std::forward<U>(value);
                    element.gen.store(tail_pos + 1);
                    return true;
                }
            } else if (generation < tail_pos) {
                return false;
            } else {
                tail_pos = tail_.load();
            }
        }
    }

    struct Elem {
        T val;
        std::atomic_size_t gen;
    };

    const size_t mask_;
    std::atomic_size_t head_;
    std::atomic_size_t tail_;
    std::array<Elem, Size> data_;
};

}  // namespace metrics
