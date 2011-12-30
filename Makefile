#CC	=	gcc
CFLAGS	=	-Wall -Os

VERSIONNR =     0.10Maemo0
VERSION	=	\"V$(VERSIONNR)\"
CFLAGS	+=	-DVERSION=$(VERSION)

# for use with LIRC, uncomment the following two lines
# CFLAGS += -DUSELIRC
# LDFLAGS += -llirc_client

#######################################################################

SRC	=	kbd.c dev_uinput.c
OBJ	=	kbd.o dev_uinput.o

all:	kbdd

install: kbdd kbdd_create_config
	mkdir -p $(DESTDIR)/usr/bin
	cp -a $^ $(DESTDIR)/usr/bin
	cp -ar install/etc install/usr $(DESTDIR)

kbdd:	$(OBJ)
	$(CC) -s -o kbdd $(OBJ) $(LDFLAGS)

kbd.o:	kbd.c keyboards.h

clean:
	rm -f $(OBJ) kbdd

tgz:  kbdd-$(VERSIONNR)

kbdd-$(VERSIONNR).tgz:
	tar czvf $@ Makefile kbd.c keyboards.h README COPYING install

preparedeb: kbdd-$(VERSIONNR).tgz
	mkdir kbdd-$(VERSIONNR)
	cd kbdd-$(VERSIONNR) && tar xzvf ../kbdd-$(VERSIONNR).tgz

deb:
	dpkg-buildpackage -sa -rfakeroot
