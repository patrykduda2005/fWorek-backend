OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c))
OBJFOLDER=./obj/
CFLAGS=-Wall -Wextra -ggdb
LIBS=-lmysqlclient -lssl

exe: $(OBJECTS:%=$(OBJFOLDER)%)
	gcc $(OBJECTS:%=$(OBJFOLDER)%) -o exe $(LIBS)

$(OBJFOLDER)%.o: %.c %.h 
	gcc $(CFLAGS) -c $<
	@[ -d $(OBJFOLDER) ] || mkdir obj
	@mv *.o $(OBJFOLDER)

$(OBJFOLDER)main.o: main.c
	gcc $(CFLAGS) -c main.c
	mv main.o $(OBJFOLDER)

clean:
	rm $(OBJFOLDER)*.o *.o exe
