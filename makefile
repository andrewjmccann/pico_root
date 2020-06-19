#######################################################################
#
#       Makefile for example1
#
#       A. McCann 27-JUNE-2007
#
#######################################################################

#########################################################################
# General stuff
#########################################################################
CPP    = g++
CFLAGS = -Wunused-result -fpermissive  -o $@ -Wall -g -O2 -fPIC -rdynamic $(ROOTCFLAGS) $(PICOINC)	
CP     = /bin/cp

#########################################################################
# ROOT stuff
#########################################################################
ROOTPATH = /usr/local/src/root-6.16.00_install/bin



ROOTCFLAGS	= $(shell $(ROOTPATH)/root-config --cflags) 
ROOTLIBS	= $(shell $(ROOTPATH)/root-config --libs) -lGeom -lRGL -lHistPainter
PICOLIBS	= -L/opt/picoscope/lib/ -lusbtc08
PICOINC		= -I/opt/picoscope/include/
ALLLIBS		= $(PICOLIBS) $(ROOTLIBS)

#########################################################################
# Compilations/dependencies
#########################################################################

all:readPicoRoot

readPicoRoot.o: readPicoRoot.cpp
	$(CPP) -c $(CFLAGS) readPicoRoot.cpp

readPicoRoot: 	readPicoRoot.o
	$(CPP) $(CFLAGS) readPicoRoot.o $(ALLLIBS)


#########################################################################
# Tidying up
#########################################################################
clean:
	rm -f readPicoRoot
	rm -f *.o
