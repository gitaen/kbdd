#ifndef _DEV_UINPUT_H
#define _DEV_UINPUT_H

#define KEY_PRESSED	1
#define KEY_RELEASED	0

#define BUS_RS232       0x13                                                                                                                                 
#define BUS_BLUETOOTH   0x05                                                                                                                                 

void dev_uinput_set_debug(int debug);

int dev_uinput_init(void);

int dev_uinput_key(int fd, unsigned short code, int pressed);

void dev_uinput_close(int fd);

#endif
