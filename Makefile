# Percorsi di ricerca
vpath %.cpp src
vpath %.h   include
vpath %.h   include/rtlog

TOOLCHAIN_ROOT  = /opt/OSELAS.Toolchain-2012.12.0
GCCFILTER       = /home/corbelli/devel/gccfilter --remove-path
CROSS_COMPILE   =
# GCCFILTER       =

AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(GCCFILTER) $(CROSS_COMPILE)gcc
CXX             = $(GCCFILTER) $(CROSS_COMPILE)g++
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

INCLUDE := \
    -Iinclude/stdafx.h.gch \
    -I/usr/include/boost \
	-I/home/corbelli/devel/fmt

LFLAGS := \
	-L/home/corbelli/devel/fmt/build/fmt

LFLAGSD :=
MAKEOPTS :=
LIBS := -lfmt
LIBSD :=

WARNINGS := -Winvalid-pch -Wno-unknown-pragmas -Wall
WARNINGSD := -Winvalid-pch -Wno-unknown-pragmas -Wall
DEFINES := -D_MT -DNDEBUG -DUSE_INTERNAL_GETTID
DEFINESD :=
CFLAGS := -pthread -fno-strict-aliasing -fwrapv -fexceptions -fPIC -O2 -pipe -g $(WARNINGS) -Wstrict-prototypes $(DEFINES) $(INCLUDE)
CFLAGSD := -pthread -fno-strict-aliasing -fwrapv -fexceptions -fPIC -O2 -pipe -ggdb $(WARNINGSD) -Wstrict-prototypes $(DEFINESD) $(INCLUDE)
CXXFLAGS := -pthread -std=c++11 -fno-strict-aliasing -fwrapv -fexceptions -fPIC -O2 -pipe -g $(WARNINGS) $(DEFINES) $(INCLUDE)
#Si potrebbe usare -D_GLIBCXX_DEBUG ma vanno compilate così anche tutte le librerie
#CXXFLAGSD := -Winvalid-pch -O2 -Wall -pipe -ggdb -D_MT -DDEBUG -DBUILDING_DLL -D_GLIBCXX_DEBUG -fPIC $(INCLUDE)
CXXFLAGSD := -pthread -fno-strict-aliasing -fwrapv -fexceptions -fPIC -O2 -pipe -ggdb $(WARNINGSD) $(DEFINESD) $(INCLUDE)

ARCH := $(shell uname -m)
BINDIR := bin
INCDIR := include
SRCDIR := src
STDAFXDIR := include/stdafx.h.gch
OBJDIR := $(BINDIR)/$(ARCH)
OBJDIRD := $(BINDIR)/$(ARCH)d

#Il file stdafx.cpp e' usato solo con MSVC
lib_sources := $(shell ls -t src/|grep -v stdafx|grep cpp | sed -e 's/src\///')
lib_objects := $(lib_sources:%.cpp=$(OBJDIR)/%.o)
lib_objectsd := $(lib_sources:%.cpp=$(OBJDIRD)/%.o)

sharedLib = $(BINDIR)/librtlog.so
sharedLibD = $(BINDIR)/librtlog-d.so
staticLib = $(BINDIR)/librtlog.a
staticLibD = $(BINDIR)/librtlog-d.a
gchIncludeD = $(INCDIR)/stdafx.h.gch

.PHONY: all debug static static-debug setup clean distclean

all: setup $(staticLib)
debug: setupd
static: setup_static $(staticLib)
static-debug: setupd_static $(staticLibD)

setup:
	mkdir -p $(OBJDIR)
	mkdir -p $(STDAFXDIR)
setupd:
	mkdir -p $(OBJDIRD)
	mkdir -p $(STDAFXDIR)

setup_static: setup
setupd_static: setupd

clean: setup setupd
	$(RM) $(OBJDIR)/* $(OBJDIRD)/* $(sharedLib) $(sharedLibD) $(staticLib) $(staticLibD) $(gchIncludeD)/*

distclean: clean
	$(RM) -rf $(STDAFXDIR)

$(OBJDIR)/example1.o: examples/example1.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
example1: $(STDAFXDIR)/stdafx.h.gch $(staticLib) $(OBJDIR)/example1.o
	$(CXX) $(CXXFLAGS) $(LFLAGS) -o $(BINDIR)/$@ $(OBJDIR)/example1.o $(LIBS) -L$(BINDIR) -lrtlog

#~ $(sharedLib): override CXXFLAGS += -DBUILDING_DLL
#~ $(sharedLib): $(lib_objects)
#~ 	$(CXX) $(CXXFLAGS) $(LFLAGS) -shared -o $@ $(lib_objects) $(LIBS)

#~ $(sharedLibD): override CXXFLAGSD += -DBUILDING_DLL
#~ $(sharedLibD): $(lib_objectsd)
#~ 	$(CXX) $(CXXFLAGSD) $(LFLAGSD) -shared -o $@ $(lib_objectsd) $(LIBSD)

$(staticLib): $(lib_objects)
	$(AR) crv $@ $(lib_objects)

$(staticLibD): $(lib_objectsd)
	$(AR) crv $@ $(lib_objectsd)

# Precompiled headers
$(STDAFXDIR)/stdafx.h.gch: $(INCDIR)/stdafx.h
	$(CXX) $(CXXFLAGS) -x c++-header $(INCDIR)/stdafx.h -o $@
$(STDAFXDIR)/stdafx.h.gchd: $(INCDIR)/stdafx.h
	$(CXX) $(CXXFLAGSD) -x c++-header $(INCDIR)/stdafx.h -o $@

#Il primo prerequisito sono i pch, quindi non posso usare $<
#$^ sono TUTTI i prerequisiti, in questo caso il file .cpp e' la seconda parola
$(OBJDIR)/%.o: $(STDAFXDIR)/stdafx.h.gch %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(word 2, $^)

$(OBJDIRD)/%.o: $(STDAFXDIR)/stdafx.h.gchd %.cpp %.h
	$(CXX) $(CXXFLAGSD) -c -o $@ $(word 2, $^)
