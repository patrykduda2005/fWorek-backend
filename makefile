OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c))
CFLAGS=-ggdb
LIBS=-lmysqlclient -lssl

exe: $(OBJECTS)
	gcc $(OBJECTS) -o exe $(LIBS)

%.o: %.c %.h
	gcc $(CFLAGS) -c $<

upload:
	scp -P 11339 ./main.c frog@frog01.mikr.us:/home/frog/dziennik/main.c
	scp -P 11339 ./makefile frog@frog01.mikr.us:/home/frog/dziennik/makefile
	scp -P 11339 ./mysqldata.c frog@frog01.mikr.us:/home/frog/dziennik/mysqldata.c
	scp -P 11339 ./mysqldata.h frog@frog01.mikr.us:/home/frog/dziennik/mysqldata.h

clean:
	rm *.o exe
