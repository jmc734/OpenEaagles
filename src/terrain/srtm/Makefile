#
include ../../makedefs

LIB = $(OPENEAAGLES_LIB_DIR)/liboeTerrain.a

OBJS =  SrtmHgtFile.o 

all:
	$(MAKE) compile
	$(MAKE) archive
    
compile: ${OBJS}
    
archive:
	ar rv $(LIB) ${OBJS}

clean:
	-rm -f *.o
