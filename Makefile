CXX=g++
BASE_CXXFLAGS=-std=c++17 -Isrc/core -Isrc/ui -I./lib -I/usr/include/gtest `pkg-config --cflags gtk4`
PROJECT_CXXFLAGS=$(BASE_CXXFLAGS) -Wall 
DEBUG_CXXFLAGS=-g -O0
LDFLAGS=-lcurl -lcrypto `pkg-config --libs gtk4`
GTEST_LDFLAGS=-lgtest -lgtest_main -pthread $(LDFLAGS)
EXEC=yatc
SOURCEDIR_CORE=src/core
SOURCEDIR_UI=src/ui
TESTDIR=test
BUILDDIR=build
SOURCES=$(wildcard $(SOURCEDIR_CORE)/**/*.cc $(SOURCEDIR_CORE)/*.cc $(SOURCEDIR_UI)/**/*.cc $(SOURCEDIR_UI)/*.cc)
TEST_SOURCES=$(wildcard $(TESTDIR)/**/*.cc $(TESTDIR)/*.cc)
OBJECTS=$(patsubst $(SOURCEDIR_CORE)/%.cc,$(BUILDDIR)/%.o,$(wildcard $(SOURCEDIR_CORE)/**/*.cc $(SOURCEDIR_CORE)/*.cc)) $(patsubst $(SOURCEDIR_UI)/%.cc,$(BUILDDIR)/%.o,$(wildcard $(SOURCEDIR_UI)/**/*.cc $(SOURCEDIR_UI)/*.cc))
TEST_OBJECTS=$(patsubst $(TESTDIR)/%.cc,$(BUILDDIR)/%.o,$(TEST_SOURCES))

all: $(EXEC)

$(EXEC): $(OBJECTS)
				$(CXX) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SOURCEDIR_CORE)/%.cc
				@mkdir -p $(@D)
				$(CXX) $(PROJECT_CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SOURCEDIR_UI)/%.cc
				@mkdir -p $(@D)
				$(CXX) $(PROJECT_CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(TESTDIR)/%.cc
				@mkdir -p $(@D)
				$(CXX) $(PROJECT_CXXFLAGS) -c $< -o $@

test: $(TEST_OBJECTS) $(filter-out $(BUILDDIR)/main.o, $(OBJECTS))
				$(CXX) -o $(BUILDDIR)/test_exec $^ $(GTEST_LDFLAGS)
				./$(BUILDDIR)/test_exec

debug: PROJECT_CXXFLAGS += $(DEBUG_CXXFLAGS)
debug: all

clean:
				rm -rf $(BUILDDIR) $(EXEC)

.PHONY: all clean test
