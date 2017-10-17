CC       = gcc
FLAGS   = -pthread

# change these to proper directories where each file should be
SRCDIR   = src

EXE	 := send recv
SENDER	 := $(SRCDIR)/send.c
RECIEVER := $(SRCDIR)/recv.c
CRC	 := $(SRCDIR)/crc.c
INCLUDES := $(wildcard $(SRCDIR)/*.h)
rm       = rm -f


all: send recv

send: $(SENDER) $(CRC) $(INCLUDES)
	$(CC) $(SENDER) $(CRC) -o send
	echo "Compiled "$<" successfully!"

recv: $(RECIEVER) $(CRC) $(INCLUDES)
	$(CC) $(FLAGS) $(RECIEVER) $(CRC) -o recv
	echo "Compiled "$<" successfully!"

.PHONY: remove
remove:
	$(rm) $(EXE)
	echo "Executable removed!"
