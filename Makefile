CC = g++
CFLAGS = -g -Wall
LDFLAGS = -lpthread
objects = proxy.o  utilities.o Rio.o WorkBuffer.o RequestHandler.o Cache.o HttpResponse.o GetHandler.o PostHandler.o ConnectHandler.o
all: proxy

utilities.o:
	$(CC) $(CFLAGS) -c utilities.cpp

Rio.o:
	$(CC) $(CFLAGS) -c Rio.cpp

WorkBuffer.o:
	$(CC) $(CFLAGS) -c WorkBuffer.cpp

Cache.o:
	$(CC) $(CFLAGS) -c Cache.cpp

RequestHandler.o:
	$(CC) $(CFLAGS) -c RequestHandler.cpp

HttpResponse.o:
	$(CC) $(CFLAGS) -c HttpResponse.cpp

GetHandler.o:
	$(CC) $(CFLAGS) -c GetHandler.cpp

PostHandler.o:
	$(CC) $(CFLAGS) -c PostHandler.cpp

ConnectHandler.o:
	$(CC) $(CFLAGS) -c ConnectHandler.cpp

proxy.o: proxy.cpp
	$(CC) $(CFLAGS) -c proxy.cpp

proxy: $(objects)
	g++ $(CFLAGS) $(objects) -o proxy $(LDFLAGS)


clean:
	rm -f *~ *.o proxy *.gch
	rm -rf proxydir  noproxy	# test dir
	rm -rf noproxy_* proxy_*	# test dir
	rm -rf scriptdir/noproxy_* scriptdir/proxy_*	# test dir
	rm -rf logs/proxy.log

# apt update && apt install -y g++ make net-tools curl python3.8 vim telnet gdb
