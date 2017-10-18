CC       = gcc
FLAGS   = -pthread

# change these to proper directories where each file should be
SRCDIR   = src

EXE	 := sendfile recvfile
SENDER	 := $(SRCDIR)/sendfile.c
RECIEVER := $(SRCDIR)/recvfile.c
CRC	 := $(SRCDIR)/crc.c
INCLUDES := $(wildcard $(SRCDIR)/*.h)
rm       = rm -f


all: sendfile recvfile log

log:
	mkdir -p log

sendfile: $(SENDER) $(CRC) $(INCLUDES)
	$(CC) $(SENDER) $(CRC) -o sendfile
#echo "Compiled "$<" successfully!"

recvfile: $(RECIEVER) $(CRC) $(INCLUDES)
	$(CC) $(FLAGS) $(RECIEVER) $(CRC) -o recvfile
#echo "Compiled "$<" successfully!"

.PHONY: remove
remove:
	$(rm) $(EXE)
#echo "Executable removed!"
