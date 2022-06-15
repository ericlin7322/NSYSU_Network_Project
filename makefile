CC = g++
TARGET1 = DNSServer
TARGET2 = server
TARGET3 = client
FLAG = -std=c++11

all: 
	$(CC) $(FLAG) Servers/$(TARGET1).cpp -o Servers/DNS
	$(CC) $(FLAG) Servers/$(TARGET2).cpp -o Servers/server
	$(CC) $(FLAG) Clients1/$(TARGET3).cpp -o Clients1/client
	$(CC) $(FLAG) Clients2/$(TARGET3).cpp -o Clients2/client

clean:
	rm -f Servers/DNS Servers/server Clients1/client Clients2/client
	
