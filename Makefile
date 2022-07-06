#if not specified we are compiling for desktop
ifndef ARCH
  ARCH = x86_64
endif

#Same for both ARCH
CFLAGS = -Wall -Wextra -Wformat  -std=gnu11
SRCDIR := src
LIB25XX := 25XX
#ARCH specific
ifeq ($(ARCH), x86_64)
  CC = gcc  
  BUILDDIR := build
  BINDIR := bin
  STLIBDIR := lib25XX/lib   
else ifeq ($(ARCH), ARM)
  CC = $(HOME)/opt/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --sysroot=/mnt/bbb-rootfs
  BUILDDIR := build-arm
  BINDIR := bin-arm
  STLIBDIR := lib25XX/lib-arm
else
  $(error ARCH UNDEFINED)
endif

#ARCH specific done, TARGET depends on ARCH
TARGET := $(BINDIR)/25XXTester

all: $(TARGET)

debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

#build static library
$(TARGET): $(BUILDDIR)/main.o $(STLIBDIR)/lib$(LIB25XX).a
	mkdir -p $(@D)
	$(CC) -o $@ $(filter-out $(STLIBDIR)/lib$(LIB25XX).a, $^) -L$(STLIBDIR) -l$(LIB25XX) -lm -lpthread

$(BUILDDIR)/main.o: $(SRCDIR)/main.c
	mkdir -p $(BUILDDIR)	
	$(CC) -Ilib$(LIB25XX)/src -c $^ $(CFLAGS) -o $@

$(STLIBDIR)/lib$(LIB25XX).a:
	cd lib$(LIB25XX) && $(MAKE) $(MAKECMDGOALS)
	
clean:
	rm -f $(BUILDDIR)/*.o $(BINDIR)/*
	cd lib$(LIB25XX) && $(MAKE) $(MAKECMDGOALS)
