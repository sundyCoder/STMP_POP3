
OBJS=main.o smtp.o tcp.o
CC=gcc
smtp:$(OBJS)
	$(CC) $^ -lpthread -lsqlite3 -o $@
main.o:smtp_main.c smtp_ser.h tcp.h
	$(CC) -c smtp_main.c -o $@
smtp.o:smtp_ser.c smtp_ser.h tcp.h
	$(CC) -c smtp_ser.c -o $@
tcp.o: tcp.c tcp.h
	$(CC) -c tcp.c -o $@
PHNOY:clean
clean:
	rm -f *.o smtp
