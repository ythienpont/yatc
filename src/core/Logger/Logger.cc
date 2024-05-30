#include "Logger.h"
#include <chrono>
#include <iomanip>

Logger *Logger::instance_ = nullptr;
std::mutex Logger::mutex_;

void Logger::log(const std::string &message, Level level) {
  std::lock_guard<std::mutex> lock(mu_);
  if (level >= logLevel_) {
    *output_ << "[" << getCurrentTime() << "] " << toString(level) << ": "
             << message << std::endl;
  }
}

std::string Logger::toString(Level level) const {
  switch (level) {
  case DEBUG:
    return "DEBUG";
  case INFO:
    return "INFO";
  case WARNING:
    return "WARNING";
  case ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}
std::string Logger::getCurrentTime() const {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t),
                      "%Y-%m-%d %X"); // Format: YYYY-MM-DD HH:MM:SS
  return ss.str();
}

void Logger::setLevel(Level level) { logLevel_ = level; }

void Logger::setOutput(std::ostream *os) { output_ = os; }
