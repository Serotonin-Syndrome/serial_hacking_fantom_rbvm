CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
CPPFLAGS :=
LDFLAGS :=

all: vm da

vm: RBVM.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) RBVM.cpp -o vm $(LDFLAGS)

da: disassembler.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) disassembler.cpp -o da $(LDFLAGS)

clean:
	$(RM) vm da

.PHONY: all clean
