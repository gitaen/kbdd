/*
 *  Copyright (C) 2004,2005 Nils Faerber <nils.faerber@kernelconcepts.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "uinput.h"

#include "dev_uinput.h"

/* taken from linux/bitops.h */
static inline int test_bit(int nr, const volatile unsigned long *p){
	return (p[nr >> 5] >> (nr & 31)) & 1UL;
}

static inline void set_bit(int nr, volatile unsigned long *p){
	p[nr >> 5] |= (1UL << (nr & 31));
}

static inline void clear_bit(int nr, volatile unsigned long *p){
	p[nr >> 5] &= ~(1UL << (nr & 31));
}

unsigned long pressed_keys[(KEY_MAX+1)/(8*sizeof(unsigned long))];

int dev_uinput_init(void)
{
struct uinput_dev dev;
int fd, aux;

	fd = open("/dev/uinput", O_RDWR);
	if (fd < 0)
		fd = open("/dev/misc/uinput", O_RDWR);
	if (fd < 0)
		fd = open("/dev/devfs/misc/uinput", O_RDWR);
	if (fd < 0)
		fd = open("/dev/input/uinput", O_RDWR);
	if (fd < 0) {
		perror("failed to open uinput device");
		return -1;
	}

	memset(&dev, 0, sizeof(dev));
	strncpy(dev.name, "SerKBD", UINPUT_MAX_NAME_SIZE);
	dev.idbus = BUS_RS232;
	dev.idvendor = 0x00;
	dev.idproduct = 0x00;
	dev.idversion = 0x00;

	if (write(fd, &dev, sizeof(dev)) < 0) {
		fprintf(stderr,"failed to write uinputdev: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) != 0) {
		close(fd);
		return -1;
	}
	if (ioctl(fd, UI_SET_EVBIT, EV_REP) != 0) {
		close(fd);
		return -1;
	}
	for (aux = KEY_RESERVED; aux <= KEY_UNKNOWN; aux++)
		if (ioctl(fd, UI_SET_KEYBIT, aux) != 0) {
			close(fd);
			return -1;
		}
	if (ioctl(fd, UI_DEV_CREATE) != 0) {
		close(fd);
		return -1;
	}
	memset(pressed_keys,0,sizeof(pressed_keys));
return fd;
}

int dev_uinput_key(int fd, unsigned short code, int pressed)
{
struct uinput_event event;
	if (code<=KEY_MAX){
		/* remember pressed keys, we release them on exit */
		if (pressed)
			set_bit(code,pressed_keys);
		else
			clear_bit(code,pressed_keys);	
	}
	memset(&event, 0, sizeof(event));
	event.type = EV_KEY;
	event.code = code;
	event.value = pressed; // (0 release, 1 press?)

return (write(fd, &event, sizeof(event)));
}

void dev_uinput_close(int fd)
{
struct uinput_event event;
unsigned short i;
	memset(&event, 0, sizeof(event));
	event.type = EV_KEY;
	// event.value = 0; // (0 release, 1 press?)
	for (i=0;i<=KEY_MAX;i++) 
		if (test_bit(i,pressed_keys)){	
			event.code = i;
			write(fd, &event, sizeof(event));
		}

	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
}
