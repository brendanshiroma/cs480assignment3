# Makefile for gpummu
CXX = g++
CXXFLAGS = -std=c++11 -Wall -g3

OBJS = gpummu.o pagetable.o tlb.o log.o tracereader.o

gpummu: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gpummu $(OBJS)

gpummu.o: gpummu.cpp pagetable.h tlb.h log.h tracereader.h
	$(CXX) $(CXXFLAGS) -c gpummu.cpp

pagetable.o: pagetable.cpp pagetable.h
	$(CXX) $(CXXFLAGS) -c pagetable.cpp

tlb.o: tlb.cpp tlb.h
	$(CXX) $(CXXFLAGS) -c tlb.cpp

log.o: log.cpp log.h
	$(CXX) $(CXXFLAGS) -c log.cpp

tracereader.o: tracereader.cpp tracereader.h
	$(CXX) $(CXXFLAGS) -c tracereader.cpp

clean:
	rm -f *.o gpummu
