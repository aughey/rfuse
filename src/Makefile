-include Makefile.vars

OBJECTS=helper.o context.o filler.o intern_rfuse.o rfuse.o rfuse_mod.o file_info.o

all: $(PROJECT)

$(PROJECT): $(OBJECTS)
	$(LD) $(LDFLAGS) -shared -o rfuse.so $(OBJECTS)

.c.o:	
	$(CC) $(CFLAGS) -o $@ -c $<
clean:
	rm -f *.o
	rm -f *~
	rm -f *.so
