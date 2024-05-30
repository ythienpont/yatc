#include "MainWindow/MainWindow.h"
#include <sstream>

std::ostringstream Logger::null_stream_;

int main(int argc, char **argv) {
  MainWindow mainWindow;
  mainWindow.run(argc, argv);
  return 0;
}
