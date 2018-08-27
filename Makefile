SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
PPC_DLL = ppc.dll

DEBUG_FLAG = -g
CXX_PIC_FLAGS = -fPIC
CXXFLAGS = $(DEBUG_FLAG) $(CXX_PIC_FLAGS) -w -I$(HCC1_SRCDIR)
RM = rm -r -f

all:$(PPC_DLL)


$(PPC_DLL) : $(OBJS)
	$(CXX) $(CXX_DEBUG_FLAGS) -shared -o $@ $(OBJS)

clean:
	$(RM) *.o *~ *.dll *.so .vs x64 Debug Release
