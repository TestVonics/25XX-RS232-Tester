#if not specified we are compiling for desktop
ifndef ARCH
  ARCH = x86_64
endif

#Same for both ARCH
CFLAGS = -Wall -Wextra -Wformat  -std=gnu11
SRCDIR := src

#ARCH specific
ifeq ($(ARCH), x86_64)
  CC = gcc  
  BUILDDIR := build
  LIBDIR := lib
else ifeq ($(ARCH), ARM)
  CC = $(HOME)/opt/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --sysroot=/mnt/bbb-rootfs
  BUILDDIR := build-arm
  LIBDIR = lib-arm
else
  $(error ARCH UNDEFINED)
endif

#ARCH specific done, TARGET depends on ARCH
TARGET := $(LIBDIR)/lib25XX.a

all: $(TARGET)

debug: CFLAGS += -Wno-unused-parameter -DDEBUG -g
debug: $(TARGET)

#build static library
$(TARGET): $(BUILDDIR)/serial.o $(BUILDDIR)/test.o
	mkdir -p $(@D)
	ar rcs $@ $^

$(BUILDDIR)/serial.o: $(SRCDIR)/serial.c
	mkdir -p $(BUILDDIR)
	mkdir -p log
	$(CC) -c $^ $(CFLAGS) -o $@

$(BUILDDIR)/test.o: $(SRCDIR)/test.c
	mkdir -p $(BUILDDIR)
	mkdir -p log
	$(CC) -c $^ $(CFLAGS) -o $@

clean:
	rm -f $(BUILDDIR)/*.o $(LIBDIR)/*

