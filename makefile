###########################################################################################################################
# tells the makefile to use bash as the shell
SHELL := /bin/bash

WARNINGFLAGS=-Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-comment -Werror

# Some default variables
RELEASE?=n
HEADER?=n

# Where are we putting the build stuff?
ifeq "$(RELEASE)" "n"
	BINDIR=bin/debug/x86-64-GCC-Linux/
	CXX_FLAGS= -std=c++17 $(WARNINGFLAGS) -march=native -DLINUX -DDEBUG -ggdb
	CC_FLAGS= -std=c99 $(WARNINGFLAGS) -march=native -DLINUX -DDEBUG -ggdb
	DEPFLAGS= -MT $@ -MMD -MP -MF $@.Td
	POSTCOMPILE= @mv -f $@.Td $@.d && touch $@
else
	BINDIR=bin/release/x86-64-GCC-Linux/
	CXX_FLAGS= -std=c++17 $(WARNINGFLAGS) -DLINUX -o3
	CC_FLAGS= -std=c99 $(WARNINGFLAGS) -DLINUX -o3
	DEPFLAGS= 
	POSTCOMPILE= 
endif

# output directory lists
OBJDIR=$(BINDIR)obj

# find GCC bits
CC=$(shell which gcc)
CXX=$(shell which g++)
AR=$(shell which gcc-ar)

GCCVERSION=$(shell $(CC) -dumpversion)

COMPILE.cxx= @echo "  CXX    "$< && $(CXX) 
COMPILE.c= @echo "  CC     "$< && $(CC)
COMPILE.link= @echo "  LINK   "$@ && $(CXX)

# get source listings for projects
MAIN_SRC=\
main.cpp \
arguments.cpp

# change the extension to .o & add obj/ prefix
MAIN_OBJS=$(addprefix $(OBJDIR)/,$(addsuffix .o, $(basename $(MAIN_SRC))))

###########################################################################################################################
# targets

# top targets
all: $(BINDIR)memcpytest

# target for build directories
.PRECIOUS: $(BINDIR)%/
$(BINDIR)%/:
	@mkdir $@ -p
	@echo "  DIR    "$@
	
# cleanup target
clean:
	rm -rf bin

# source files
.SECONDEXPANSION:
$(OBJDIR)/%.o: %.cpp $(OBJDIR)/%.o.d | $$(@D)/ 
	$(COMPILE.cxx) $(CXX_FLAGS) $(DEPFLAGS) -fpic -o $@ -c $<
	$(POSTCOMPILE)

.SECONDEXPANSION:
$(OBJDIR)/%.o: %.c $(OBJDIR)/%.o.d | $$(@D)/
	$(COMPILE.c) $(CC_FLAGS)) $(DEPFLAGS) -fpic -o $@ -c $<
	$(POSTCOMPILE)
	
# dependency dummy - stops header deps getting killed off
$(OBJDIR)/%.o.d: ;
.PRECIOUS: $(OBJDIR)/%.o.d
	

# output
$(BINDIR)memcpytest: $(MAIN_OBJS)
	$(COMPILE.link) $(MAIN_OBJS) -static-libstdc++ -lpthread -o $@ 

# header dependency includes
include $(wildcard $(patsubst %,%.d,$(MAIN_OBJS)))
