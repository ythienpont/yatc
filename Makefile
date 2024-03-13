CXX=g++
BASE_CXXFLAGS=-std=c++17 -Isrc -I./lib
PROJECT_CXXFLAGS=$(BASE_CXXFLAGS) -Wall 
#PROJECT_CXXFLAGS=$(BASE_CXXFLAGS) -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual -Wconversion -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2
LDFLAGS=-lcurl -lcrypto
EXEC=yatc
SOURCEDIR=src
BUILDDIR=build
SOURCES=$(wildcard $(SOURCEDIR)/**/*.cc $(SOURCEDIR)/*.cc)
OBJECTS=$(patsubst $(SOURCEDIR)/%.cc,$(BUILDDIR)/%.o,$(SOURCES))

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/lib/%.o: lib/%.cc
	@mkdir -p $(@D)
	$(CXX) $(BASE_CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.cc
	@mkdir -p $(@D)
	$(CXX) $(PROJECT_CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(EXEC)

.PHONY: all clean

