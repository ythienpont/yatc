CXX=g++
CXXFLAGS=-std=c++17 -Wall -Isrc
LDFLAGS=-lcurl -lcrypto
EXEC=yatc
SOURCEDIR=src
BUILDDIR=build
SOURCES=$(wildcard $(SOURCEDIR)/**/*.cc $(SOURCEDIR)/*.cc)
OBJECTS=$(patsubst $(SOURCEDIR)/%.cc,$(BUILDDIR)/%.o,$(SOURCES))

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(EXEC)

.PHONY: all clean
