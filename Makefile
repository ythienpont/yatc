CXX=g++
BASE_CXXFLAGS=-std=c++17 -Isrc/core -I./lib -I/usr/include/gtest
PROJECT_CXXFLAGS=$(BASE_CXXFLAGS) -Wall 
DEBUG_CXXFLAGS=-g -O0
LDFLAGS=-lcurl -lcrypto
GTEST_LDFLAGS=-lgtest -lgtest_main -pthread $(LDFLAGS)
EXEC=yatc
SOURCEDIR=src/core
TESTDIR=test
BUILDDIR=build
SOURCES=$(wildcard $(SOURCEDIR)/**/*.cc $(SOURCEDIR)/*.cc)
TEST_SOURCES=$(wildcard $(TESTDIR)/**/*.cc $(TESTDIR)/*.cc)
OBJECTS=$(patsubst $(SOURCEDIR)/%.cc,$(BUILDDIR)/%.o,$(SOURCES))
TEST_OBJECTS=$(patsubst $(TESTDIR)/%.cc,$(BUILDDIR)/%.o,$(TEST_SOURCES))

all: $(EXEC)

$(EXEC): $(OBJECTS)
		$(CXX) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.cc
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
