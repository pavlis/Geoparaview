BIN=pathtosurface
PF=pathtosurface.pf
#MAN1=pathtosurface.1
cflags=-g
ldlibs=$(DBLIBS) -lseispp -lgclgrid -lperf -lm -lseispp
SUBDIR=/contrib

include $(ANTELOPEMAKE)  	
include $(ANTELOPE)/contrib/include/antelopemake.local
CXXFLAGS += -I$(BOOSTINCLUDE)
LDFLAGS += -L$(BOOSTLIB)

OBJS=pathtosurface.o
$(BIN) : $(OBJS)
	$(RM) $@
	$(CXX) $(CCFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)
