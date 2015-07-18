CXX = g++
CXXFLAGS = -O3 -std=c++11

file = good.x bad.x
libs = Good.o Bad.o

all: $(file)

debug: CXXFLAGS = -O0 -DDEBUG -g -std=c++11
debug: $(file)

good.x: main.cc $(libs)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DGOOD

bad.x: main.cc $(libs)
	$(CXX) $(CXXFLAGS) $^ -o $@ -DBAD

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf *.x *.o

.PHONY: test
test: $(file)
	for a in $(file); do ./$$a test.dat; done

