# CC=g++
CC=../../../clang+llvm-13.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang
CXX=../../../clang+llvm-13.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang++
CFLAGS=-mclwb
LDFLAGS=
SOURCES=tatp_db.cc tatp_nvm.cc ../common/common.c
LDLIBS:=-lpmem -pthread -lpmemobj

CLOBBERCLANG:=~/agr/Clobber-NVM/build/bin/clang
CLOBBERCLANGPP:=~/agr/Clobber-NVM/build/bin/clang++
CLOBBERLOGCLANG:=~/agr/Clobber-NVM/clobberlogclang
CLOBBERLOGCLANGPP:=~/agr/Clobber-NVM/clobberlogclangpp

LDFLAGS:=-Wl,--wrap=pthread_join -Wl,--wrap=pthread_create\
				-Wl,--wrap=pthread_mutex_lock\
				-Wl,--wrap=pthread_mutex_unlock\
				-Wl,--wrap=malloc -Wl,--wrap=free

all: tatp-clobber

admin_pop.o: wrap/admin_pop.c
	$(CLOBBERCLANGPP) $(CFLAGS) $(MOREFLAGS) -c -o $@ $^

context.o: wrap/context.c wrap/context.h
	$(CLOBBERCLANGPP) -pthread $(CFLAGS) $(MOREFLAGS) -c -o $@ $<

clobber.o: wrap/clobber.c
	$(CLOBBERCLANGPP) $(CFLAGS) $(MOREFLAGS) -c -o $@ $^

tatp-clobber: $(SOURCES) clobber.o context.o admin_pop.o
	$(CLOBBERLOGCLANGPP) -c tatp_db.cc -o tatp_db.o
	$(CLOBBERCLANGPP) -c tatp_nvm.cc -o tatp_nvm.o
	$(CLOBBERCLANGPP) -c ../common/common.cc -o common.o
	$(CLOBBERCLANGPP) -o $@ $(LDLIBS) $(LDFLAGS) context.o clobber.o admin_pop.o tatp_db.o tatp_nvm.o common.o

original:
	$(CXX) -ggdb -O0 -mclwb -I../asm $(SOURCES) -o ${EXECUTABLE} $(CFLAGS)


../m5_mmap.o: ../m5_mmap.h

../m5op_x86.o:
	$(CC) -I../asm -c ../m5op_x86.S  ${CFLAGS} -o ../m5op_x86.o

clean:
	rm -f *.o $(EXECUTABLE) tatp-clobber
