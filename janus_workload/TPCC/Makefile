CC=../../../clang+llvm-13.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang
CXX=../../../clang+llvm-13.0.0-x86_64-linux-gnu-ubuntu-20.04/bin/clang++
CLOBBERCLANG:=~/agr/Clobber-NVM/build/bin/clang-7
CLOBBERCLANGPP:=~/agr/Clobber-NVM/build/bin/clang++
CLOBBERLOGCLANG:=~/agr/Clobber-NVM/clobberlogclang
CLOBBERLOGCLANGPP:=~/agr/Clobber-NVM/clobberlogclangpp

CFLAGS=-O1 -g -mclwb
CLOBBER_CFLAGS=-mclwb
SOURCES=tpcc_db.cc tpcc_nvm.cc common.cc
LDLIBS=-pthread -lpmem -lpmemobj
LDFLAGS_CLOBBER:=-Wl,--wrap=pthread_join -Wl,--wrap=pthread_create\
				-Wl,--wrap=pthread_mutex_lock\
				-Wl,--wrap=pthread_mutex_unlock\
				-Wl,--wrap=malloc -Wl,--wrap=free

EADR_FLAG:=-D PMEM_NO_FLUSH=1

all: tpcc.adr.volt tpcc.eadr.volt tpcc.adr.undo tpcc.eadr.undo tpcc.adr.pmdk tpcc.eadr.pmdk tpcc.adr.clobber tpcc.eadr.clobber

admin_pop.o: wrap/admin_pop.c
	$(CLOBBERCLANGPP) $(CFLAGS) -c -o $@ $^

context.o: wrap/context.c wrap/context.h
	$(CLOBBERCLANGPP) $(CFLAGS) -pthread -c -o $@ $<

clobber.o: wrap/clobber.c
	$(CLOBBERCLANGPP) $(CFLAGS) -c -o $@ $^

tpcc.adr.volt: $(SOURCES)
	$(CXX) $(CFLAGS) $(LDLIBS) $^ -D_ENABLE_VOLATILE -o $@

tpcc.eadr.volt: $(SOURCES)
	$(CXX) $(CFLAGS) $(EADR_FLAG) $(LDLIBS) $^ -D_ENABLE_VOLATILE -o $@

tpcc.adr.undo: $(SOURCES)
	$(CXX) $(CFLAGS) $(LDLIBS) $^ -D_ENABLE_LOGGING -o $@

tpcc.eadr.undo: $(SOURCES)
	$(CXX) $(CFLAGS) $(EADR_FLAG) $(LDLIBS) $^ -D_ENABLE_LOGGING -o $@

tpcc.adr.pmdk: $(SOURCES)
	$(CXX) $(CFLAGS) $(LDLIBS) $^ -D_ENABLE_LIBPMEMOBJ -o $@

tpcc.eadr.pmdk: $(SOURCES)
	$(CXX) $(CFLAGS) $(EADR_FLAG) $(LDLIBS) $^ -D_ENABLE_LIBPMEMOBJ -o $@

tpcc.adr.clobber: $(SOURCES) clobber.o context.o admin_pop.o
	$(CLOBBERLOGCLANGPP) -c $(CLOBBER_CFLAGS) tpcc_db.cc -o tpcc_db.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) tpcc_nvm.cc -o tpcc_nvm.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) common.cc -o common.o
	$(CLOBBERCLANGPP) tpcc_db.o tpcc_nvm.o common.o clobber.o context.o admin_pop.o $(CFLAGS) $(LDLIBS) $(LDFLAGS_CLOBBER) -o $@

tpcc.eadr.clobber: $(SOURCES) clobber.o context.o admin_pop.o
	$(CLOBBERLOGCLANGPP) -c $(CLOBBER_CFLAGS) $(EADR_FLAG) tpcc_db.cc -o tpcc_db.eadr.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) $(EADR_FLAG) tpcc_nvm.cc -o tpcc_nvm.eadr.o
	$(CLOBBERCLANGPP) -c $(CFLAGS) $(EADR_FLAG) common.cc -o common.eadr.o
	$(CLOBBERCLANGPP) tpcc_db.eadr.o tpcc_nvm.eadr.o common.eadr.o clobber.o context.o admin_pop.o $(CFLAGS) $(EADR_FLAG) $(LDLIBS) $(LDFLAGS_CLOBBER) -o $@

clean:
	rm -f tpcc.adr.volt tpcc.adr.undo tpcc.adr.clobber tpcc.adr.pmdk
	rm -f tpcc.eadr.volt tpcc.eadr.undo tpcc.eadr.clobber tpcc.eadr.pmdk
	rm -f *.o
