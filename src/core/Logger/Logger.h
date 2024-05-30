#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

/**
 * @class Logger
 * @brief Singleton Logger class for handling logging operations.
 *
 * This class implements the singleton pattern to provide a unique logger
 * instance throughout the application. It supports logging messages at
 * various severity levels to a configurable output stream.
 */
class Logger {
public:
  /// Enumerates log severity levels.
  enum Level { DEBUG, INFO, WARNING, ERROR };

  /**
   * Deletes the copy constructor to prevent copying.
   */
  Logger(const Logger &) = delete;

  /**
   * Deletes the assignment operator to prevent copying.
   */
  Logger &operator=(const Logger &) = delete;

  /**
   * @brief Retrieves the singleton instance of Logger.
   * @return Returns the singleton instance of Logger.
   */
  static Logger *instance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
      instance_ = new Logger();
    }
    return instance_;
  }

  /**
   * @brief Logs a message at the specified log level.
   * @param message The message to be logged.
   * @param level The severity level of the message. Defaults to DEBUG.
   */
  void log(const std::string &message, Level level = DEBUG);

  /**
   * @brief Sets the minimum log level.
   * @param level The new log level.
   */
  void setLevel(Level level);

  /**
   * @brief Sets the output stream for logging.
   * @param os Pointer to the output stream.
   */
  void setOutput(std::ostream *os);

private:
  /**
   * @brief Private constructor for the singleton logger.
   */
  Logger() : output_(&null_stream_), logLevel_(DEBUG) {}

  /**
   * @brief Converts a log level to its string representation.
   * @param level Log level to convert.
   * @return String representation of the log level.
   */
  std::string toString(Level level) const;
  std::string getCurrentTime() const;

  static Logger *instance_; ///< Static instance of the logger.
  static std::mutex mutex_; ///< Mutex for thread-safe instance creation.
  std::mutex mu_;           ///< Mutex for thread-safe logging.
  std::ostream *output_;    ///< Pointer to the output stream.
  Level logLevel_;          ///< Current logging level.
  static std::ostringstream null_stream_;
};

#endif // !LOGGER_H
