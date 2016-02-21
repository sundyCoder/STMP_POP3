OBJS=main.o pop3.o tcp.o md5.o
CC=gcc
pop3:$(OBJS)
	$(CC) $^ -lsqlite3 -lpthread -o $@

main.o:pop3_main.c tcp.h pop3_ser.h
	$(CC) -c pop3_main.c -o $@
	
pop3.o:pop3_ser.c tcp.h pop3_ser.h md5.h 
	$(CC) -c pop3_ser.c -o $@
	
tcp.o:tcp.c tcp.h
	$(CC) -c tcp.c -o $@
	
md5.o:md5.c md5.h
	$(CC) -c md5.c -o $@

PHONY:clean
clean:
	rm -f *.o pop3
