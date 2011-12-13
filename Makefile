#CC	=	gcc
CFLAGS	=	-Wall -Os

VERSION	=	\"V0.10\"
CFLAGS	+=	-DVERSION=$(VERSION)

# for use with LIRC, uncomment the following two lines
# CFLAGS += -DUSELIRC
# LDFLAGS += -llirc_client

#######################################################################

SRC	=	kbd.c dev_uinput.c
OBJ	=	kbd.o dev_uinput.o

all:	kbdd

kbdd:	$(OBJ)
	$(CC) -s -o kbdd $(OBJ) $(LDFLAGS)

kbd.o:	kbd.c keyboards.h

libvirtkeys.o:	libvirtkeys.c libvirtkeys.h

clean:
	rm -f $(OBJ) kbdd
