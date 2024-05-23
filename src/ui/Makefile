# Define variables
CXX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags gtk4`
LDFLAGS = `pkg-config --libs gtk4`
UI_SOURCES = main.cc
MAINWINDOW_SOURCES = MainWindow/MainWindow.cc
TORRENTINFO_SOURCES = TorrentInfo/TorrentInfo.cc
SOURCES = $(UI_SOURCES) $(MAINWINDOW_SOURCES) $(TORRENTINFO_SOURCES)
# Generate object file names from source files
OBJECTS = $(SOURCES:.cc=.o)

# Define the target executable
TARGET = BitTorrentClient

# Default target
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Rule to compile source files
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# PHONY targets
.PHONY: all clean
