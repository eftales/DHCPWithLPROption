build-DHCPInit:
	g++ main.c dhcp/*.h dhcp/*.c -std=c++11  -o libDHCPInit.so

clean :
	rm -f $(TARGET)
	rm -f *.o *.lease *.so
