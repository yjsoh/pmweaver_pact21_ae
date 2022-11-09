CC=../../../clang+llvm-13.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang
CXX=../../../clang+llvm-13.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang++
CLOBBERCLANG:=~/agr/Clobber-NVM/build/bin/clang-7
CLOBBERCLANGPP:=~/agr/Clobber-NVM/build/bin/clang++
CLOBBERLOGCLANG:=~/agr/Clobber-NVM/clobberlogclang
CLOBBERLOGCLANGPP:=~/agr/Clobber-NVM/clobberlogclangpp

CFLAGS=-O1 -g -mclwb -D_SKIP_INITIALIZATION
CLOBBER_CFLAGS=-mclwb
SOURCES=tatp_db.cc tatp_nvm.cc common.cc
LDLIBS:=-lpmem -pthread -lpmemobj
LDFLAGS_CLOBBER:=-Wl,--wrap=pthread_join -Wl,--wrap=pthread_create\
				-Wl,--wrap=pthread_mutex_lock\
				-Wl,--wrap=pthread_mutex_unlock\
				-Wl,--wrap=malloc -Wl,--wrap=free

EADR_FLAG:=-D PMEM_NO_FLUSH=1

all: tatp.adr.undo tatp.eadr.undo tatp.adr.pmdk tatp.eadr.pmdk tatp.adr.clobber tatp.eadr.clobber

admin_pop.o: wrap/admin_pop.c
	$(CLOBBERCLANGPP) $(CFLAGS) -c -o $@ $^

context.o: wrap/context.c wrap/context.h
	$(CLOBBERCLANGPP) $(CFLAGS) -pthread -c -o $@ $<

clobber.o: wrap/clobber.c
	$(CLOBBERCLANGPP) $(CFLAGS) -c -o $@ $^

tatp.adr.undo: $(SOURCES)
	$(CXX) $(CFLAGS) $(LDLIBS) $^ -D_ENABLE_LOGGING -o $@

tatp.eadr.undo: $(SOURCES)
	$(CXX) $(CFLAGS) $(EADR_FLAG) $(LDLIBS) $^ -D_ENABLE_LOGGING -o $@

tatp.adr.pmdk: $(SOURCES)
	$(CXX) $(CFLAGS) $(LDLIBS) $^ -D_ENABLE_LIBPMEMOBJ -o $@

tatp.eadr.pmdk: $(SOURCES)
	$(CXX) $(CFLAGS) $(EADR_FLAG) $(LDLIBS) $^ -D_ENABLE_LIBPMEMOBJ -o $@

tatp.adr.clobber: $(SOURCES) clobber.o context.o admin_pop.o
	$(CLOBBERLOGCLANGPP) -c $(CLOBBER_CFLAGS) tatp_db.cc -o tatp_db.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) tatp_nvm.cc -o tatp_nvm.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) common.cc -o common.o
	$(CLOBBERCLANGPP) tatp_db.o tatp_nvm.o common.o clobber.o context.o admin_pop.o $(CFLAGS) $(LDLIBS) $(LDFLAGS_CLOBBER) -o $@

tatp.eadr.clobber: $(SOURCES) clobber.o context.o admin_pop.o
	$(CLOBBERLOGCLANGPP) -c $(CLOBBER_CFLAGS) $(EADR_FLAG) tatp_db.cc -o tatp_db.eadr.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) $(EADR_FLAG) tatp_nvm.cc -o tatp_nvm.eadr.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) $(EADR_FLAG) common.cc -o common.eadr.o
	$(CLOBBERCLANGPP) tatp_db.eadr.o tatp_nvm.eadr.o common.eadr.o clobber.o context.o admin_pop.o $(CFLAGS) $(EADR_FLAG) $(LDLIBS) $(LDFLAGS_CLOBBER) -o $@

clean:
	rm -f tatp.adr.undo tatp.adr.clobber
	rm -f tatp.eadr.undo tatp.eadr.clobber
	rm -f *.o
