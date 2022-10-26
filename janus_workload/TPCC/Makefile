CC=g++
CFLAGS=-mclwb
SOURCES= tpcc_db.cc tpcc_nvm.cc common.cc
LIBS=-pthread -lpmem
EXECUTABLE=tpcc_nvm

all:
	$(CXX) $(SOURCES) $(CFLAGS) -o ${EXECUTABLE} ${LIBS}

clean:
	rm -f *.o $(EXECUTABLE)
