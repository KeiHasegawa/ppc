SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
PPC_DLL = ppc.dll

XX_OBJS = $(SRCS:.cpp=.obj)
PPC_XX_DLL = ppcxx.dll

DEBUG_FLAG = -g
CXX_PIC_FLAGS = -fPIC
CXXFLAGS = $(DEBUG_FLAG) $(CXX_PIC_FLAGS) -w -I$(HCC1_SRCDIR)

CXXFLAGS_FOR_XX = $(DEBUG_FLAG) $(CXX_PIC_FLAGS) -w -I$(HCXX1_SRCDIR) -DCXX_GENERATOR
RM = rm -r -f

all:$(PPC_DLL) $(PPC_XX_DLL)

$(PPC_DLL) : $(OBJS)
	$(CXX) $(DEBUG_FLAG) -shared -o $@ $(OBJS)

$(PPC_XX_DLL) : $(XX_OBJS)
	$(CXX) $(DEBUG_FLAG) -shared -o $@ $(XX_OBJS)

%.obj : %.cpp
	$(CXX) $(CXXFLAGS_FOR_XX) -c $< -o $@

clean:
	$(RM) *.o *~ *.dll *.so .vs x64 Debug Release
	$(RM) *.obj
