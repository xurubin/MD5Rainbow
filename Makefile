##----------------------------------------------------------------------------
# Created with genmake.pl v1.1 on Sat Apr 10 21:22:00 2010
# genmake.pl home: http://muquit.com/muquit/software/
# Copryright: GNU GPL (http://www.gnu.org/copyleft/gpl.html)
##----------------------------------------------------------------------------
rm=/bin/rm -f
CC= g++
DEFS=  
PROGNAME= MD5Rainbow
INCLUDES=  -I.
LIBS=-lpthread
#For 64 bit linux
MD5=md5-amd64
##MD5=mx86-cof x86 VC
##MD5=mx86-elf x86 linux
DEFINES=$(INCLUDES) $(DEFS) -DSYS_UNIX=1 -U_FORTIFY_SOURCE
CFLAGS=-O3 -g $(DEFINES) 

SRCS = Alphabet.cpp ConfigFile.cpp DiskFile.cpp worker.cpp HashReduce.cpp main.cpp TableManager.cpp UIManager.cpp IndexFile.cpp $(MD5).s

OBJS = Alphabet.o ConfigFile.o DiskFile.o worker.o HashReduce.o main.o TableManager.o UIManager.o IndexFile.o $(MD5).o

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

all: $(PROGNAME)

$(PROGNAME) : $(OBJS)
	$(CC) $(CFLAGS) -o $(PROGNAME) $(OBJS) $(LIBS)

clean:
	$(rm) $(OBJS) $(PROGNAME) core *~
