#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T> class SafeQueue {
public:
  void push(T value);

  bool try_pop(T &value);

private:
  std::queue<T> queue_;
  std::mutex mtx_;
  std::condition_variable cv_;
};
