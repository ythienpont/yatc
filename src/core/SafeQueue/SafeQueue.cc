#include "SafeQueue.h"

template <typename T> void SafeQueue<T>::push(T value) {
  std::lock_guard<std::mutex> lock(mtx_);
  queue_.push(std::move(value));
  cv_.notify_one();
}

template <typename T> bool SafeQueue<T>::try_pop(T &value) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (queue_.empty()) {
    return false;
  }
  value = std::move(queue_.front());
  queue_.pop();
  return true;
}
