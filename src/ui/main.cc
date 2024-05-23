#include "MainWindow/MainWindow.h"

int main(int argc, char **argv) {
  srand(time(NULL)); // Seed the random number generator
  MainWindow mainWindow;
  mainWindow.run(argc, argv);
  return 0;
}
