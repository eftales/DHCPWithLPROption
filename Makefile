CC     := gcc
CFLAGS := 
TARGET := thclient
OBJS   := main.o interface.o dhcp.o socket.o

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

build-DHCPInit:
	g++ main.c dhcp/*.h dhcp/*.c -std=c++11  -o libDHCPInit.so

clean :
	rm -f $(TARGET)
	rm -f *.o *.lease *.so
