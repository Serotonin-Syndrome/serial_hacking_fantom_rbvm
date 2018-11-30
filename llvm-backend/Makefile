LLVM_CONFIG := $(shell command -v llvm-config-7 || command -v llvm-config-6)

CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LDLIBS := $(shell $(LLVM_CONFIG) --libs)

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))

PROGRAM := llvm-rbvm

$(PROGRAM): $(OBJECTS)
	$(CXX) $(LDLIBS) $(OBJECTS) -o $(PROGRAM) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^

clean:
	$(RM) $(OBJECTS) $(PROGRAM)

.PHONY: clean
