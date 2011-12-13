/*
 *  Copyright (C) 2004,2005 Nils Faerber <nils.faerber@kernelconcepts.de>
 *  Parts for Stowaway (c) 2005 by Paul Eggleton
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <linux/apm_bios.h>
#ifdef USELIRC
#include <lirc_client.h>
#endif

#include "keyboards.h"
#include "dev_uinput.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define ARRAY_SIZE(A)	(sizeof(A)/sizeof(A[0]))

char debug=0;
int uindev=0;
static int reinit=0;

#define DEFAULT_TTS "/dev/ttyS0"
char TTY_PORT[PATH_MAX] = DEFAULT_TTS;

void handle_sigterm(int signal)
{
	dev_uinput_close(uindev);
	exit(EXIT_SUCCESS);
}

int open_serial(char *port, speed_t  baud)
{
int fd, res;
struct termios ssetup;

	fd=open(port, O_RDWR /*| O_NONBLOCK*/);
	if (fd <= 0) {
		perror("open serial");
		fprintf(stderr,"opening %s failed\n", port);
		return (-1);
	}

	res = tcgetattr(fd, &ssetup);
	if (res < 0) {
		perror("tcgetattr");
		return (-1);
	}

	ssetup.c_iflag = 0L;
	ssetup.c_oflag = 0L;
	ssetup.c_cflag &= ~(CSTOPB|PARENB|CRTSCTS);
	ssetup.c_cflag |= (CS8|CLOCAL);
	ssetup.c_lflag = 0L;
	ssetup.c_cc[VTIME] = 0;
	ssetup.c_cc[VMIN] = 1;
	cfsetispeed(&ssetup, baud);
	cfsetospeed(&ssetup, baud);

	res=tcsetattr(fd, TCSANOW, &ssetup);
	if (res < 0) {
		perror("tcsetattr");
		return (-1);
	}

return (fd);
}


#ifdef USELIRC
int lirc(char *progname)
{
struct lirc_config *config;

	if (debug) fprintf(stderr, "progname %s\n",progname);
	if (lirc_init(progname,1) == -1) {
		printf("failed to init lirc\n");
		exit(EXIT_FAILURE);
	}
	if (debug) fprintf(stderr, "in lirc\n");
	if (lirc_readconfig(NULL,&config,NULL) == 0) {
		int i, ret;
		char *code, *c;

		while (lirc_nextcode(&code) == 0) {
			if (code == NULL)
				continue;
			while ((ret = lirc_code2char(config, code, &c)) == 0 &&
				c != NULL) {
				sscanf(c,"%x",&i);
				if (debug) fprintf(stderr, "%s %u\n",c,i);
				dev_uinput_key(uindev, i, KEY_PRESSED);
				dev_uinput_key(uindev, i, KEY_RELEASED);
				if (debug) fprintf(stderr, "Code : %s\n",c);
			}
			free(code);
			if (ret == -1)
				break;
		}
		lirc_freeconfig(config);
	}

return lirc_deinit();
}
#endif


int compaq_foldable(void)
{
int fd;
unsigned char buf[16];
char fn=0;

	fd = open_serial(TTY_PORT, B4800);
	if (fd <= 0)
		return (-1);

	while (fd > 0) {
		read (fd, buf, 2);
		// fprintf(stderr, "0x%02x 0x%02x 0x%02x | ", buf[0], buf[1], ~buf[0]);
		if (buf[0] == (unsigned char)~buf[1]) {
			if (debug) fprintf(stderr, "press: %d ", buf[0]);
			if (buf[0] == 0x02) {
				fn=1;
				continue;
			}
			if (fn)
				buf[0]=foldable_function[buf[0]];
			else
				buf[0]=foldable_normal[buf[0]];
			if (debug) fprintf(stderr,"= 0x%02x\n", buf[0]);
			if (buf[0] > 0)
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
		} else if (((unsigned char)buf[0] & (unsigned char)~0x80) == (unsigned char)~buf[1]) {
			if (debug) fprintf(stderr, "rel. : %d ", buf[0] & ~0x80);
			if ((buf[0] & ~0x80) == 0x02) {
				fn = 0;
				continue;
			}
			if (fn)
				buf[0]=foldable_function[(unsigned char)buf[0] & (unsigned char)~0x80];
			else
				buf[0]=foldable_normal[(unsigned char)buf[0] & (unsigned char)~0x80];
			if (debug) fprintf(stderr,"= 0x%02x\n", buf[0]);
			if (buf[0] > 0)
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);
		}
	}

return 0;
}


int belkin_infrared(void)
{
int fd;
unsigned char buf[16];
unsigned char key;
unsigned int  key_down;
unsigned char keycode;

	fd = open_serial(TTY_PORT, B9600);
	if (fd <= 0)
		return (-1);

	while (fd > 0) {
		read (fd, buf, 2);
		key             =   buf[1] & 0x7f;
	        key_down        = !(buf[1] & 0x80);
	        keycode         = belkin_irda_normal[key];
		if (debug) fprintf(stderr, "0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], key);

       	        if ( key_down ) {
       	        	if (debug) fprintf(stderr,"press %d\n", keycode);
       	        	dev_uinput_key(uindev, (unsigned short)keycode, KEY_PRESSED);
		} else {
       	        	if (debug) fprintf(stderr,"release %d\n", keycode);
       	        	dev_uinput_key(uindev, (unsigned short)keycode, KEY_RELEASED);
		}
	}

return 0;
}

int freedom_keyboard(void)
{
int fd;
unsigned char buf[16];
unsigned char key;
unsigned int  key_down;
unsigned char keycode;

	fd = open_serial(TTY_PORT, B9600);
	if (fd <= 0)
		return (-1);

	while (fd > 0) {
		read (fd, buf, 1);
		key             =  buf[0];
		//keyboard sends n when pressing a key
		// and n+63 when releasing the key
                // (63 keys)
		key_down	= ( key <= 63 );
		if (!key_down)
			key	= (key-63)&0x3F; // convert key code for key up
                if (key > 63) {
                     fprintf(stdout, "Invalid key: %02d, setting to R\n", key);
                     key = 10;
                }
	        keycode         = freedom_kbd[key];
		if (debug)
			fprintf(stdout, "%02d %02d\n", buf[0], keycode);
       	        if ( key_down ) {
			if (debug)
				fprintf(stdout,"press %d\n", keycode);
       	        	dev_uinput_key(uindev, (unsigned short)keycode, KEY_PRESSED);
		} else {
			if (debug)
				fprintf(stdout,"release %d\n", keycode);
       	        	dev_uinput_key(uindev, (unsigned short)keycode, KEY_RELEASED);
		}
	}

return 0;
}

int select_read(int fd, int timeout_sec, int timeout_us)
{
fd_set fds;
struct timeval tv;
	    
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = timeout_sec;
	tv.tv_usec = timeout_us;
	return select(fd+1, &fds, NULL, NULL, &tv);
}


int stowaway_init(int fd)
{
int status;
unsigned char buf[16];
	
	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_DTR;  /* Set DTR */
	status &= ~TIOCM_RTS; /* Clear RTS */
	ioctl(fd, TIOCMSET, &status);
	
	/* Unfortunately, DCD seems to be high all of the time on H3900, so the following can't be used */
	/* ioctl(fd, TIOCMIWAIT, TIOCM_CAR */
	/* So we just wait instead */
	usleep(1000000);
	
	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_RTS;  /* Set RTS */
	ioctl(fd, TIOCMSET, &status);
	/* Stowaway will send back 0xFA 0xFD indicating successful init */
	if (select_read(fd, 2, 0)) {
		read(fd, buf, 2);
		if ((buf[0] == 0xFA) && (buf[0] == 0xFD))
			if (debug) fprintf(stderr, "keyboard initialised\n");
	}
	
	return 0;
}


int open_apm(void)
{
int fd;
	
	fd = open( "/dev/apm_bios", O_RDONLY | O_NONBLOCK );
	if (fd <= 0)
		return -1;
	
return fd;
}


int check_apm_resume(int fd)
{
apm_event_t ev;
int resumed;
		
	resumed = 0;
	while (read(fd, &ev, sizeof(apm_event_t)) > 0) {
		if (ev == APM_NORMAL_RESUME)
			resumed = 1;
	}

return resumed;
}


void stowaway_sig(int sig)
{
	reinit = 1;
}


int stowaway(void)
{
int fd, apm_fd;
unsigned char buf[16];
char fn=0;
struct sigaction act;
int rc;

	fd = open_serial(TTY_PORT, B9600);
	if (fd <= 0)
		return (-1);
	
	/* Make SIGHUP cause a reinit of the keyboard */
	act.sa_handler = stowaway_sig;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGHUP, &act, NULL);

	/* Open APM so we can listen for resume events */
	apm_fd = open_apm();
	
	while (fd > 0) {
		
		stowaway_init(fd);
			
		while (fd > 0) {
			rc = select_read(fd, 0, 100000);
			if (rc == -1) {
				if (reinit) {
					reinit = 0;
					break;
				} else {
					perror("select");
					return 1;
				}
			} else if (rc == 0) {
				/* Timeout expired with nothing read, do APM resume check */
				if ((apm_fd > 0) && (check_apm_resume(apm_fd)))
					break;
				continue;
			}
			
			rc = read (fd, buf, 1);
			if (rc == -1) {
				if (reinit) {
					reinit = 0;
					break;
				} else {
					perror("read");
					return 1;
				}
			}
			
			if ( ((unsigned char)buf[0] & (unsigned char)0x80) == 0 ) {
				if (debug) fprintf(stderr, "press: %d\n", buf[0]);
				if (buf[0] == 0x08) {
					fn = 1;
					continue;
				}
				if (fn)
					buf[0] = stowaway_function[buf[0]];
				else
					buf[0] = stowaway_normal[buf[0]];
				if (debug) fprintf(stderr,"= 0x%02x\n", buf[0]);
				if (buf[0] > 0)
					dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
			} else {
				if (debug) fprintf(stderr, "rel. : %d\n", buf[0] & ~0x80);
				if ((buf[0] & ~0x80) == 0x08) {
					fn = 0;
					continue;
				}
				if (fn)
					buf[0] = stowaway_function[(unsigned char)buf[0] & (unsigned char)~0x80];
				else
					buf[0] = stowaway_normal[(unsigned char)buf[0] & (unsigned char)~0x80];
				if (debug) fprintf(stderr,"= 0x%02x\n", buf[0]);
				if (buf[0] > 0)
					dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);
			}
		}
	}

return 0;
}


int stowawayxt(void)
{
#define STOWAWAYXT_GR_FN 33
#define STOWAWAYXT_BL_FN 34

int fd, apm_fd;
unsigned char buf[16];
char bluefn=0,greenfn=0;
struct sigaction act;
int rc;

	fd = open_serial(TTY_PORT, B9600);
	if (fd <= 0)
		return (-1);

	/* Make SIGHUP cause a reinit of the keyboard */
	act.sa_handler = stowaway_sig;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGHUP, &act, NULL);

	/* Open APM so we can listen for resume events */
	apm_fd = open_apm();

	while (fd > 0) {
		stowaway_init(fd);

		while (fd > 0) {
			rc = select_read(fd, 0, 100000);
			if (rc == -1) {
				if (reinit) {
					reinit = 0;
					break;
				} else {
					perror("select");
					return 1;
				}
			} else if (rc == 0) {
				/* Timeout expired with nothing read, do APM resume check */
				if ((apm_fd > 0) && (check_apm_resume(apm_fd)))
					break;
				continue;
			}

			rc = read (fd, buf, 1);
			if (rc == -1) {
				if (reinit) {
					reinit = 0;
					break;
				} else {
					perror("read");
					return 1;
				}
			}

			if ( ((unsigned char)buf[0] & (unsigned char)0x80) == 0 ) {
				/* KEY PRESSED */
				if (debug)
					fprintf(stderr, "press: %d\n", buf[0]);
				if (buf[0] == STOWAWAYXT_BL_FN) {
					bluefn = 1;
					continue;
				}

				if (buf[0] == STOWAWAYXT_GR_FN)  {
					greenfn = 1;
					dev_uinput_key(uindev,42,KEY_PRESSED);
					continue;
				}

				if (bluefn)
					buf[0] = stowawayxt_function[buf[0]];
				else if (greenfn) {
					buf[0] = stowawayxt_function[buf[0]];
					/* fixup where green function is not shift blue function */
					switch (buf[0]) {
						case KEY_UP:
							buf[0] = KEY_PAGEUP;
							break;
						case KEY_LEFT:
							buf[0] = KEY_HOME;
							break;
						case KEY_DOWN:
							buf[0] = KEY_PAGEDOWN;
							break;
						case KEY_RIGHT:
							buf[0] = KEY_END;
							break; 
						case KEY_INTL2:
							buf[0] = KEY_INTL3;
							break;
					} /* /switch */
				} else
					buf[0]=stowawayxt_normal[buf[0]];

				if (debug)
					fprintf(stderr,"= 0x%02x\n", buf[0]);
				if (buf[0] != KEY_RESERVED)
					dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);

			} else {
				/* KEY RELEASED */
				if (debug)
					fprintf(stderr, "rel. : %d\n", buf[0] & ~0x80);

				if ((buf[0] & ~0x80) == STOWAWAYXT_BL_FN) {
					bluefn = 0;
					continue;
				}

				if ((buf[0] & ~0x80) == STOWAWAYXT_GR_FN) {
					greenfn = 0;
					dev_uinput_key(uindev,42,KEY_RELEASED);
					continue;
				}

				if (bluefn)
					buf[0] = stowawayxt_function[(unsigned char) buf[0] & (unsigned char)~0x80];
				else if (greenfn) {
					buf[0] = stowawayxt_function[(unsigned char) buf[0] & (unsigned char)~0x80];

					/* fixup where green function is not shift blue function */
					switch(buf[0]) {
						case KEY_UP:
							buf[0] = KEY_PAGEUP;
							break;
						case KEY_LEFT:
							buf[0] = KEY_HOME;
							break;
						case KEY_DOWN:
							buf[0] = KEY_PAGEDOWN;
							break;
						case KEY_RIGHT:
							buf[0] = KEY_END;
							break;
						case KEY_INTL2:
							buf[0] = KEY_INTL3;
							break;
					} /* /switch */
				} else
					buf[0] = stowawayxt_normal[(unsigned char)buf[0] & (unsigned char)~0x80];

				if (debug)
					fprintf(stderr,"= 0x%02x\n", buf[0]);
				if (buf[0] != KEY_RESERVED)
					dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);
			}
		}
	}

return 0;
}


int snapntype(void)
{
int fd;
unsigned char buf[16];
char symb=0;

	fd = open_serial(TTY_PORT, B2400);

	while (fd > 0) {
		read (fd, buf, 1);
		if (debug) fprintf(stderr, "got %d\n", buf[0]);
		if (buf[0] & 0x80) { /* release */
			read(fd,buf+1,1);
			buf[0] = buf[0] & 0x7f;
			if (buf[0] == 27) {
				symb = 0;
				continue;
			}
			if (symb)
				buf[0] = snapntype_symbol[buf[0]];
			else
				buf[0] = snapntype_normal[buf[0]];
			if (debug) fprintf(stderr, " release %d\n", buf[0]);
			if (buf[0] != 0)
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);
		} else { /* press */
			if (buf[0] == 27) {
				symb=1;
				continue;
			}
			if (symb)
				buf[0] = snapntype_symbol[buf[0]];
			else
				buf[0] = snapntype_normal[buf[0]];
			if (debug) fprintf(stderr, " press %d\n", buf[0]);
			if (buf[0] != 0)
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
		}
	}

return 0;
}


int snapntypebt(void)
{
int fd;
unsigned char buf[16];
unsigned char key, last_key=0;
unsigned char pressed;
unsigned char fn = 0;

	fd = open_serial(TTY_PORT, B9600);

	while (fd > 0) {
		read (fd, buf, 1);
		read (fd, buf+1, 1);
		if (debug) fprintf(stderr, "got %02x%02x\n", buf[0],buf[1]);
		if (buf[1] & 0x80) { /* release */
			key = buf[1] & 0x7F;

			if (key == 30) {
				fn = 0;
				if (last_key) {
					dev_uinput_key(uindev, (unsigned short)snapntypebt_fn[last_key], KEY_RELEASED);
				}
				continue;
			}

			pressed = KEY_RELEASED;			
			if (debug) fprintf(stderr, "  release:");
				
				
		} else {
			key = buf[1];

			if (key == 30) {
				fn = 1;
				last_key = 0;
				continue;
			}

			pressed = KEY_PRESSED;
			if (debug) fprintf(stderr, "  press: ");


		}
		if (debug) fprintf(stderr, " %d (%d)\n",key,snapntypebt_normal[key]);

		if (!fn)
			key = snapntypebt_normal[key];
		else {
			last_key = key;
			key = snapntypebt_fn[key];
		}
		if (key != 0 && !debug)
			dev_uinput_key(uindev, (unsigned short)key, pressed);

	}

return 0;
}

int hpslim(void)
{
int fd;
unsigned char cin;
unsigned char cnew;
char symb=0;
char shft=0;
char symshft=0;
char numlck=0;

	fd = open_serial(TTY_PORT, B4800);
	if (fd < 0)
		return -1;

	for (;;) {
		if (read (fd, &cin, 1) < 0) {
			perror(TTY_PORT);
			return -1;
		}
		if (debug) fprintf(stderr, "got %d 0x%x\n", cin,cin);
		if (cin & 0x80) { /* release */
			cin &= 0x7f;
			switch (cin) {
				case MKBD_HPS_FNKEY: 	/* Fn key release */
					symb = 0;
					continue;
				case KEY_LEFTSHIFT:
				case KEY_RIGHTSHIFT:
					shft = 0;
					break;
			}
			if (symb)
				cnew = hpslim_symbol[cin];
			else {
				/* if numlock, convert QWERTYUIOP to 1-9,0 */
				if (numlck && cin >= KEY_Q && cin <= KEY_P)
					cnew = cin + KEY_1 - KEY_Q;
				else cnew = hpslim_normal[cin];
			}
			if (debug) fprintf(stderr, " release cnew=%d 0x%x\n", cnew,cnew);
			if (cnew != 0) {
				dev_uinput_key(uindev, (unsigned short)cnew, KEY_RELEASED);
			        if (symshft)
					dev_uinput_key(uindev, (unsigned short)KEY_RIGHTSHIFT, KEY_RELEASED);
				symshft = 0;
				// toggle numlck on release
				if (cnew == KEY_NUMLOCK) numlck = !numlck;
			}
		} else { /* press */
			switch (cin) {
				case MKBD_HPS_FNKEY: 	/* Fn key press */
					symb = 1;
					continue;
				case KEY_LEFTSHIFT:
				case KEY_RIGHTSHIFT:
					shft = 1;
					break;
			}
			if (symb) {
				/* if shift needed to get correct char */
				symshft =  hpslim_symshft[cin] && !shft;
				cnew = hpslim_symbol[cin];
			} else {
				/* if numlock, convert QWERTYUIOP to 1-9,0 */
				if (numlck && cin >= KEY_Q && cin <= KEY_P)
					cnew = cin + KEY_1 - KEY_Q;
				else cnew = hpslim_normal[cin];
			}
			if (debug) fprintf(stderr, " press cnew=%d 0x%x\n", cnew,cnew);
			if (cnew != 0) {
				if (symshft)
					dev_uinput_key(uindev, (unsigned short)KEY_RIGHTSHIFT, KEY_PRESSED);
				dev_uinput_key(uindev, (unsigned short)cnew, KEY_PRESSED);
			}
		}
	}

return 0;
}

int smartbt(void)
{
int fd;
unsigned char buf[16];

	fd = open_serial(TTY_PORT, B2400);

	while (fd > 0) {
		read (fd, buf, 1);
		if (buf[0] > 126) {
			if (debug) fprintf(stderr, "error: unexpected char %d\n", buf[0]);
			continue;
		}
		if (buf[0] > 63) { 
			/* release */
			buf[0] = buf[0] - 63;
			if (debug) fprintf(stderr, "rel.: %d\n", buf[0]);
			buf[0] = smartbt_normal[buf[0]];
			if (debug) fprintf(stderr,"= 0x%02x\n", buf[0]);
			if (buf[0] > 0)
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);
		} else { 
			/* press */
			if (debug) fprintf(stderr, "press: %d\n", buf[0]);
			buf[0] = smartbt_normal[buf[0]];
			if (debug) fprintf(stderr,"= 0x%02x\n", buf[0]);
			if (buf[0] > 0)
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
		}
	}

return 0;
}


int flexis(void)
{
int fd;
unsigned char buf[16];
char symb=0;

	fd = open_serial(TTY_PORT, B9600);

	while (fd > 0) {
		read (fd, buf, 1);
		if (debug) fprintf(stderr, "got %d\n", buf[0]);
		
		if (buf[0] & 0x80) { /* key press */
			buf[0] = buf[0] & 0x7f;
		
			if (symb)
				buf[0] = flexis_fx100_function[buf[0]];
			else
				buf[0] = flexis_fx100_normal[buf[0]];
			
			/* FIXME: function key */
			/* FIXME: this code is a little broken, it should check scancode not keycode for set 1/2 */
			
			if (buf[0] == KEY_LEFTSHIFT || buf[0] == KEY_RIGHTSHIFT ||
					buf[0] == KEY_LEFTCTRL || buf[0] == KEY_RIGHTCTRL ||
					buf[0] == KEY_LEFTALT || buf[0] == KEY_RIGHTALT ) {
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
			} else {
				read(fd,buf+1,1);
				if (debug) fprintf(stderr, "got %d\n", buf[1]);
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);
			}
		} else { /* release of key from Set 2 */
			if (symb)
				buf[0] = flexis_fx100_function[buf[0]];
			else
				buf[0] = flexis_fx100_normal[buf[0]];
			if (buf[0] != 0)
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);
		}
	}

	return 0;
}


int benqgamepad(void) {
int fd;
unsigned char buf[16];
int i;
unsigned char keycode;

	fd = open_serial(TTY_PORT, B4800);

	while (fd > 0) {
		keycode = 0;
		read (fd, buf, 2);
		if (buf[0] & 0x80) { /* release */
			if (debug) fprintf(stderr, "release: %d %d\n", buf[0], buf[1]);
			buf[0] = buf[0] & 0x7f;
			for (i=0; i<10; i++) {
				if (benq_gamepad_map[i][0] == buf[0]) {
					keycode = benq_gamepad_map[i][1];
					break;
				}
			}
			if (keycode != 0)
				dev_uinput_key(uindev, (unsigned short)keycode, KEY_RELEASED);
			
		} else {
			if (debug) fprintf(stderr, "press: %d %d\n", buf[0], buf[1]);
			for (i=0; i<10; i++) {
				if (benq_gamepad_map[i][0] == buf[0]) {
					keycode = benq_gamepad_map[i][1];
					break;
				}
			}
			if (keycode != 0)
				dev_uinput_key(uindev, (unsigned short)keycode, KEY_PRESSED);
		}
	}

	return 0;
}


int pocketvik(void)
{
int fd;
unsigned char buf[16];
unsigned char mods=0;
int res;
struct termios ssetup;
int i;

	fd = open_serial(TTY_PORT, B57600);

	/* PocketVIK needs CRTSCTS */
	res = tcgetattr(fd, &ssetup);
	if (res < 0) {
		perror("tcgetattr");
		return (-1);
	}
	ssetup.c_cflag |= CRTSCTS;
	res = tcsetattr(fd, TCSANOW, &ssetup);
	if (res < 0) {
		perror("tcgetattr");
		return (-1);
	}
		
	while (fd > 0) {
		read (fd, buf, 1);
		if (debug) fprintf(stderr, "got %d\n", buf[0]);
		
		if (buf[0] & 0x80) { /* press with modifiers */
			mods = buf[0] & 0x7f;
			read (fd, buf, 1);
		} else
			mods = 0;
		
		if (mods & POCKETVIK_Fn)
			buf[0]=pocketvik_function[buf[0]];
		else
			buf[0]=pocketvik_normal[buf[0]];
		
		if (buf[0] != 0) {
			if (debug) fprintf(stderr, "press/release %d\n", buf[0]);
			
			/* Modifiers down */
			for ( i = 0 ; i < ARRAY_SIZE(pocketvik_modifiers) ; i++ ) {
				if ( pocketvik_modifiers[i].mask & mods )
					dev_uinput_key(uindev, pocketvik_modifiers[i].key, KEY_PRESSED);
			}

			/* Key down */
			dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
			/* Key up */
			dev_uinput_key(uindev, (unsigned short)buf[0], KEY_RELEASED);

			/* Modifiers up */
			for ( i = 0 ; i < ARRAY_SIZE(pocketvik_modifiers) ; i++ ) {
				if ( pocketvik_modifiers[i].mask & mods )
					dev_uinput_key(uindev, pocketvik_modifiers[i].key, KEY_RELEASED);
			}
			
		}
	}

	return 0;
}


int micro_foldaway(void) {
int fd;
unsigned char buf[16];
unsigned char lastkey = 0;
// char symb=0;
int checkkey = 0;

	fd = open_serial(TTY_PORT, B9600);

	while (fd > 0) {
		read (fd, buf, 1);
		if (debug) fprintf(stderr, "got: %d\n", buf[0]);
		if (buf[0] & 0x80) { /* possible release */
			checkkey = (159 - buf[0]);
			if (checkkey > 0 && checkkey <= 4) {
				buf[0] = micro_foldaway_normal[159 - buf[0]];
				dev_uinput_key(uindev, buf[0], KEY_RELEASED);
			}
			else if (buf[0] == 133 && lastkey > 0) {
				dev_uinput_key(uindev, lastkey, KEY_RELEASED);
				lastkey = 0;
			}
			/* Eat repeated code */
			read (fd, buf, 1);
		} else {
			checkkey = buf[0];
/*			if (symb)
				buf[0] = micro_foldaway_function[buf[0]];
			else*/
				buf[0] = micro_foldaway_normal[buf[0]];
	
			if (buf[0] != 0) {
				if (checkkey > 4)
					lastkey = buf[0];  /* Not a modifier, record key */
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
			}
		}
	}

	return 0;
}


int micro_datapad(void) {
int fd;
unsigned char buf[16];
unsigned char lastkey = 0;
int checkkey = 0;

	fd = open_serial(TTY_PORT, B9600);

	while (fd > 0) {
		read (fd, buf, 1);
		if (debug) fprintf(stderr, "got: %d\n", buf[0]);
		if (buf[0] & 0x80) { /* possible release */
			checkkey = (159 - buf[0]);
			if (checkkey > 0 && checkkey <= 4) {
				buf[0] = micro_foldaway_normal[159 - buf[0]];
				dev_uinput_key(uindev, buf[0], KEY_RELEASED);
			}
			else if (buf[0] == 133 && lastkey > 0) {
				dev_uinput_key(uindev, lastkey, KEY_RELEASED);
				lastkey = 0;
			}
			/* Eat repeated code */
			read (fd, buf, 1);
		} else {
			checkkey = buf[0];
/*			if (symb)
				buf[0]=micro_datapad_function[buf[0]];
			else*/
			buf[0]=micro_datapad_normal[buf[0]];
	
			if (buf[0] != 0) {
				if (checkkey > 4)
					lastkey = buf[0];  /* Not a modifier, record key */
				dev_uinput_key(uindev, (unsigned short)buf[0], KEY_PRESSED);
			}
		}
	}

return 0;
}

int hp_bt_foldable(void){
int fd;
unsigned char buf[1];
unsigned char cnew;
char symb=0;

	fd = open_serial(TTY_PORT, B9600);
	if (fd < 0)
		return -1;

	for (;;) {
		if (read (fd, buf, 1) < 0) {
			perror(TTY_PORT);
			return -1;
		}
		if (debug) fprintf(stderr, "got %d (0x%x)\n", buf[0], buf[0]);
		if (buf[0] & 0x80) { /* release */
			buf[0] &= 0x7f;
			if (buf[0] == 2) {
				symb = 0;
				continue;
			}
			if (symb)
				cnew = btfoldable_function[buf[0]];
			else
				cnew = btfoldable_normal[buf[0]];
		
			if (debug) fprintf(stderr, " release cnew=%d (0x%x)\n", cnew, cnew);
			if (cnew != 0)
				dev_uinput_key(uindev, (unsigned short)cnew, KEY_RELEASED);
		} else { /* press */
			if (buf[0] == 2) {
				symb = 1;
				continue;
			}
			if (symb)
				cnew = btfoldable_function[buf[0]];
			else
				cnew = btfoldable_normal[buf[0]];
			if (debug) fprintf(stderr, " press cnew=%d (0x%x)\n", cnew, cnew);
			if (cnew != 0)
				dev_uinput_key(uindev, (unsigned short)cnew, KEY_PRESSED);
		}
	}
return 0;
};

int compaq_microkbd(void)
{
int fd;
unsigned char buf[2];
unsigned char cnew;
char symb=0;

	fd = open_serial(TTY_PORT, B4800);
	if (fd < 0)
		return -1;

	for (;;) {
		if (read (fd, buf, 2) < 0) {
			perror(TTY_PORT);
			return -1;
		}
		if (debug) fprintf(stderr, "got %d %d (0x%x 0x%x)\n", buf[0], buf[1], buf[0], buf[1]);
		if (buf[0] & 0x80) { /* release */
			buf[0] &= 0x7f;
			if (buf[0] == 2) {
				symb = 0;
				continue;
			}
			if (symb)
				cnew = compaq_function[buf[0]];
			else
				cnew = compaq_normal[buf[0]];
		
			if (debug) fprintf(stderr, " release cnew=%d (0x%x)\n", cnew, cnew);
			if (cnew != 0)
				dev_uinput_key(uindev, (unsigned short)cnew, KEY_RELEASED);
		} else { /* press */
			if (buf[0] == 2) {
				symb = 1;
				continue;
			}
			if (symb)
				cnew = compaq_function[buf[0]];
			else
				cnew = compaq_normal[buf[0]];
			if (debug) fprintf(stderr, " press cnew=%d (0x%x)\n", cnew, cnew);
			if (cnew != 0)
				dev_uinput_key(uindev, (unsigned short)cnew, KEY_PRESSED);
		}
	}

return 0;
}

int targus_infrared(void)
{
int fd;
unsigned char buf;
unsigned char key;
unsigned int  key_down;
unsigned char keycode;

	fd = open_serial(TTY_PORT, B9600);
	if (fd <= 0)
		return (-1);

	while (fd > 0) {
		read (fd, &buf, 1); 
		key             =   buf & 0x7f;
	        key_down        = !(buf & 0x80);
	        /* keycode         = targus_irda_normal[key]; */
		keycode = key;
		if (debug) fprintf(stderr, "0x%02x 0x%02x\n", buf, key);

       	        if ( key_down ) {
       	        	if (debug) fprintf(stderr,"press %d\n", keycode);
       	        	dev_uinput_key(uindev, (unsigned short)keycode, KEY_PRESSED);
		} else {
       	        	if (debug) fprintf(stderr,"release %d\n", keycode);
       	        	dev_uinput_key(uindev, (unsigned short)keycode, KEY_RELEASED);
		}
	}

return 0;
}

#define TRUE (0==0)
#define FALSE (0!=0)

int elektex_keyboard(int mapped) {
  int fd, pressed = 0, state = 0;
  int shift = FALSE, ctrl = FALSE, alt = FALSE, alt_gr = FALSE;
  unsigned char buf[16], key, pressed_key1 = 0, pressed_key2 = 0;
  fd = open_serial(TTY_PORT, B9600);
  if (debug) fprintf(stderr, "Elektex\n");
  while (fd > 0) {
    read (fd, buf, 1);
    if (debug) fprintf(stderr, "got: %d\n", buf[0]);
    if (buf[0]==165&&state==0&&pressed==0) {
      state = 1;
      if (debug) fprintf(stderr, "transitioning to state 1\n");}
    else if (buf[0]==0&&state==1&&pressed==0) {
      state = 2;
      if (debug) fprintf(stderr, "transitioning to state 2\n");}
    else if (buf[0]==1&&state==2&&pressed==0) {
      state = 3;
      if (debug) fprintf(stderr, "transitioning to state 3\n");}
    else if (buf[0]==90&&state==3&&pressed==0) {
      state = 0;
      if (debug) fprintf(stderr, "165, 0, 1, 90 ignored\n");}
    else if (buf[0]==255&&state==0) {
      /* It is ambiguous as to whether this is a release of the DEL key but
	 treat it as if it is. */
      if (pressed==1&&pressed_key1==KEY_DELETE) {
	pressed = 0;
	dev_uinput_key(uindev, (unsigned short)KEY_DELETE, KEY_RELEASED);
        if (debug) fprintf(stderr, "DEL key release\n");}
      else if (debug) fprintf(stderr, "255 ignored\n");}
    else if ((buf[0]&0x80)&&state==0) {
      if (pressed==0) {
	if (debug) fprintf(stderr, "not pressed\n");}
      else if (pressed==1) {
	key = elektex[buf[0]&0x7f];
	if (key!=0) {
	  if (key==pressed_key1) {
	    pressed = 0;
            if (mapped&&shift&&key==KEY_2) {
	      dev_uinput_key(uindev,
			     (unsigned short)KEY_APOSTROPHE,
			     KEY_RELEASED);
	      if (debug) fprintf(stderr, "key release \" mapped to @\n");}
	    else if (mapped&&shift&&key==KEY_APOSTROPHE) {
	      dev_uinput_key(uindev,
			     (unsigned short)KEY_2,
			     KEY_RELEASED);
	      if (debug) fprintf(stderr, "key release @ mapped to \"\n");}
	    else if (mapped&&shift&&key==KEY_3) {
	      dev_uinput_key(uindev, (unsigned short)KEY_GRAVE, KEY_RELEASED);
	      dev_uinput_key(uindev,
			     (unsigned short)KEY_LEFTSHIFT,
			     KEY_PRESSED);
	      if (debug) fprintf(stderr,
				 "key release sterling mapped to `\n");}
	    else if (mapped&&!shift&&key==KEY_GRAVE) {
	      dev_uinput_key(uindev, (unsigned short)KEY_3, KEY_RELEASED);
	      dev_uinput_key(uindev,
			     (unsigned short)KEY_LEFTSHIFT,
			     KEY_RELEASED);
	      if (debug) fprintf(stderr, "key release ` mapped to #\n");}
	    else {
	      dev_uinput_key(uindev, (unsigned short)key, KEY_RELEASED);
	      if (debug) fprintf(stderr, "key release: %d\n", key);}
	    if (key==KEY_LEFTSHIFT) shift = !shift;
	    else if (key==KEY_LEFTCTRL) ctrl = !ctrl;
	    else if (key==KEY_LEFTALT) alt = !alt;
	    else if (key==KEY_RIGHTALT) alt_gr = !alt_gr;
	    else {
	      if (shift) {
		dev_uinput_key(uindev,
			       (unsigned short)KEY_LEFTSHIFT,
			       KEY_RELEASED);
		if (debug) fprintf(stderr, "prefix shift key release\n");}
	      if (ctrl) {
		dev_uinput_key(uindev,
			       (unsigned short)KEY_LEFTCTRL,
			       KEY_RELEASED);
		if (debug) fprintf(stderr, "prefix ctrl key release\n");}
	      if (alt) {
		dev_uinput_key(uindev,
			       (unsigned short)KEY_LEFTALT,
			       KEY_RELEASED);
		if (debug) fprintf(stderr, "prefix alt key release\n");}
	      if (alt_gr) {
		dev_uinput_key(uindev,
			       (unsigned short)KEY_RIGHTALT,
			       KEY_RELEASED);
		if (debug) fprintf(stderr, "prefix alt_gr key release\n");}
	      shift = FALSE;
	      ctrl = FALSE;
	      alt = FALSE;
	      alt_gr = FALSE;}}
	  else if (debug) fprintf(stderr, "unmatched press code\n");}
	else if (debug) fprintf(stderr, "unrecognized release code\n");}
      /* pressed==2 */
      else {
	key = elektex[buf[0]&0x7f];
	if (key!=0) {
	  if (key==pressed_key2) {
	    pressed = 1;
	    dev_uinput_key(uindev, (unsigned short)key, KEY_RELEASED);
            if (debug) fprintf(stderr, "chorded key release: %d\n", key);}
	  else if (debug) fprintf(stderr, "unmatched chorded press code\n");}
	else if (debug) fprintf(stderr,
				"unrecognized chorded release code\n");}}
    else if (state==0) {
      if (pressed==0) {
	key = elektex[buf[0]];
	if (key!=0) {
	  pressed = 1;
	  pressed_key1 = key;
	  if (key!=KEY_LEFTSHIFT&&
	      key!=KEY_LEFTCTRL&&
	      key!=KEY_LEFTALT&&
	      key!=KEY_RIGHTALT) {
	    if (shift) {
	      dev_uinput_key(uindev,
			     (unsigned short)KEY_LEFTSHIFT,
			     KEY_PRESSED);
	      if (debug) fprintf(stderr, "prefix shift key press\n");}
	    if (ctrl) {
	      dev_uinput_key(uindev,
			     (unsigned short)KEY_LEFTCTRL,
			     KEY_PRESSED);
	      if (debug) fprintf(stderr, "prefix ctrl key press\n");}
	    if (alt) {
	      dev_uinput_key(uindev, (unsigned short)KEY_LEFTALT, KEY_PRESSED);
	      if (debug) fprintf(stderr, "prefix alt key press\n");}
	    if (alt_gr) {
	      dev_uinput_key(uindev,
			     (unsigned short)KEY_RIGHTALT,
			     KEY_PRESSED);
	      if (debug) fprintf(stderr, "prefix alt_gr key press\n");}}
          if (mapped&&shift&&key==KEY_2) {
	    dev_uinput_key(uindev,
			   (unsigned short)KEY_APOSTROPHE,
			   KEY_PRESSED);
	    if (debug) fprintf(stderr, "key press \" mapped to @\n");}
	  else if (mapped&&shift&&key==KEY_APOSTROPHE) {
	    dev_uinput_key(uindev,
			   (unsigned short)KEY_2,
			   KEY_PRESSED);
	    if (debug) fprintf(stderr, "key press @ mapped to \"\n");}
	  else if (mapped&&shift&&key==KEY_3) {
	    dev_uinput_key(uindev,
			   (unsigned short)KEY_LEFTSHIFT,
			   KEY_RELEASED);
	    dev_uinput_key(uindev, (unsigned short)KEY_GRAVE, KEY_PRESSED);
	    if (debug) fprintf(stderr, "key press sterling mapped to `\n");}
	  else if (mapped&&!shift&&key==KEY_GRAVE) {
	    dev_uinput_key(uindev, (unsigned short)KEY_LEFTSHIFT, KEY_PRESSED);
	    dev_uinput_key(uindev, (unsigned short)KEY_3, KEY_PRESSED);
	    if (debug) fprintf(stderr, "key press ` mapped to #\n");}
	  else {
	    dev_uinput_key(uindev, (unsigned short)key, KEY_PRESSED);
	    if (debug) fprintf(stderr, "key press: %d\n", key);}}
	else if (debug) fprintf(stderr, "unrecognized press code\n");}
      else if (pressed==1) {
	key = elektex[buf[0]];
	if (key!=0) {
	  if (key==pressed_key1) {
	    dev_uinput_key(uindev, (unsigned short)key, KEY_PRESSED);
	    if (debug) fprintf(stderr, "repeat key press: %d\n", key);}
	  else {
	    pressed = 2;
	    pressed_key2 = key;
	    dev_uinput_key(uindev, (unsigned short)key, KEY_PRESSED);
	    if (debug) fprintf(stderr, "chorded key press: %d\n", key);}}
	else if (debug) fprintf(stderr, "unrecognized chorded press code\n");}
      /* pressed==2 */
      else if (debug) fprintf(stderr, "double bucky\n");}
    else if (debug) fprintf(stderr, "in middle of 165, 0, 1, 90\n");}
  return 0;}

void print_usage(char *arg0)
{
	fprintf (stderr, "kbdd %s\n", VERSION);
	fprintf (stderr, "Usage:\n");
	fprintf (stderr, "%s [-d] [-h] [-c <config file>] -p <serial-port> -t <kbd type>\n", arg0);
	fprintf (stderr, "-d\tenable debugging output\n");
	fprintf (stderr, "-h\tprint this help\n");
	fprintf (stderr, "-c <config file>\n");
	fprintf (stderr, "\tRead port and type from config file\n");
	fprintf (stderr, "-p <serial-port>\n");
	fprintf (stderr, "\tspecify serial port device, default %s\n", DEFAULT_TTS);
	fprintf (stderr, "-t <kbd type>\n");
	fprintf (stderr, "\tspecify the serial keyboard type, supported are:\n");
	fprintf (stderr, "\tfoldable  - Compaq/HP foldable keyboard\n");
	fprintf (stderr, "\tstowaway  - Targus Stowaway keyboard\n");
	fprintf (stderr, "\tstowawayxt - Stowaway XT\n");
	fprintf (stderr, "\tsnapntype - Snap'n'Type\n");
	fprintf (stderr, "\tsnapntypebt - Snap'n'Type Bluetooth\n");
	fprintf (stderr, "\thpslim    - HP Slim keyboard\n");
	fprintf (stderr, "\tsmartbt   - Smart Bluetooth keyboard\n");
	fprintf (stderr, "\tlirc      - LIRC consumer IR\n");
	fprintf (stderr, "\tbelkinir  - Belkin IR (not IrDA)\n");
	fprintf (stderr, "\tflexis    - Flexis FX-100 keyboard\n");
	fprintf (stderr, "\tg250      - Benq G250 gamepad\n");
	fprintf (stderr, "\tpocketvik - GrandTec PocketVIK\n");
	fprintf (stderr, "\tmicrofold - Micro Innovations Foldaway keyboard\n");
	fprintf (stderr, "\tmicropad  - Micro Innovations Datapad\n");
	fprintf (stderr, "\tmicrokbd  - Compaq MicroKeyboard\n");
	fprintf (stderr, "\ttargusir  - Targus Universal Wireless keyboard\n");
	fprintf (stderr, "\tfreedom   - Freedom keyboard\n");
	fprintf (stderr, "\tbtfoldable - HP iPAQ Bluetooth Foldable Keyboard\n");
	fprintf (stderr, "\telektex - Elektex Cloth Keyboard\n");
	fprintf (stderr, "\telektex_unmapped - Elektex Cloth Keyboard (Unmapped)\n");
	fprintf (stderr, "Example:\n\t%s -t foldable\n", arg0);
}


#define KBD_TYPE_NONE		0
#define KBD_TYPE_FOLDABLE	1
#define KBD_TYPE_SNAPNTYPE	2
#define KBD_TYPE_STOWAWAY	3
#define KBD_TYPE_STOWAWAYXT	4
#define KBD_TYPE_HPSLIM		5
#define KBD_TYPE_SMARTBT	6
#define KBD_TYPE_LIRC		7
#define KBD_TYPE_BELKINIR	8
#define KBD_TYPE_FLEXIS		9
#define KBD_TYPE_BENQ_GAMEPAD	10
#define KBD_TYPE_POCKETVIK	11
#define KBD_TYPE_MICRO_FOLDAWAY	12
#define KBD_TYPE_MICRO_DATAPAD	13
#define KBD_TYPE_COMPAQ_MICROKBD	14
#define KBD_TYPE_TARGUSIR		15
#define KBD_TYPE_BTFOLDABLE		16
#define KBD_TYPE_FREEDOM		17
#define KBD_TYPE_SNAPNTYPEBT		18
#define KBD_TYPE_ELEKTEX		19
#define KBD_TYPE_ELEKTEX_UNMAPPED	20

int find_kbd_type(const char *ktype)
{
	if (strncmp("foldable", ktype, 8) == 0)
		return KBD_TYPE_FOLDABLE;
	else if (strncmp("snapntypebt", ktype, 11) == 0)
               return KBD_TYPE_SNAPNTYPEBT;
	else if (strncmp("snapntype", ktype, 9) == 0)
		return KBD_TYPE_SNAPNTYPE;
	else if (strncmp("stowawayxt", ktype, 10) == 0)
		return KBD_TYPE_STOWAWAYXT;
	else if (strncmp("stowaway", ktype, 8) == 0)
		return KBD_TYPE_STOWAWAY;
	else if (strncmp("hpslim", ktype, 6) == 0)
		return KBD_TYPE_HPSLIM;
	else if (strncmp("smartbt", ktype, 7) == 0)
		return KBD_TYPE_SMARTBT;
	else if (strncmp("flexis", ktype, 6) == 0)
		return KBD_TYPE_FLEXIS;
	else if (strncmp("lirc", ktype, 4) == 0)
		return KBD_TYPE_LIRC;
	else if (strncmp("belkinir", ktype, 8) == 0)
		return KBD_TYPE_BELKINIR;
	else if (strncmp("g250", ktype, 4) == 0)
		return KBD_TYPE_BENQ_GAMEPAD;
	else if (strncmp("pocketvik", ktype, 9) == 0)
		return KBD_TYPE_POCKETVIK;
	else if (strncmp("microfold", ktype, 9) == 0)
		return KBD_TYPE_MICRO_FOLDAWAY;
	else if (strncmp("micropad", ktype, 8) == 0)
		return KBD_TYPE_MICRO_DATAPAD;
	else if (strncmp("microkbd", ktype, 8) == 0)
		return KBD_TYPE_COMPAQ_MICROKBD;
	else if (strncmp("targusir", ktype, 8) == 0)
		return KBD_TYPE_TARGUSIR;
	else if (strncmp("freedom", ktype, 7) == 0)
		return KBD_TYPE_FREEDOM;
	else if (strncmp("btfoldable", ktype, 10) == 0)
		return KBD_TYPE_BTFOLDABLE;
	else if (strncmp("elektex", ktype, 7) == 0)
		return KBD_TYPE_ELEKTEX;
	else if (strncmp("elektex_unmapped", ktype, 16) == 0)
		return KBD_TYPE_ELEKTEX_UNMAPPED;

	fprintf(stderr, "unrecognised keyboard type %s\n", ktype);

return KBD_TYPE_NONE;
}


void parse_config(const char *path, int *kbdtype, char port[])
{
char *needle;
FILE *fd;
char buf[PATH_MAX];
        
        fd = fopen(path, "r");
	if (!fd) {
		fprintf(stderr, "could not open config file %s\n", path);
		return;
	}
        
        while (!feof(fd)) {
		fgets(buf, PATH_MAX, fd);
		
		if (*buf == '#' || *buf == '\0') {
			/* It's a comment or a blank line */
			continue;
		}
		
		if ((needle = strstr(buf, "port:")) != 0) {
			needle += 5; 
			/* Trim whitespaces */
			while (isspace(*needle)) {
				needle++;
			}
			
			while (isspace(needle[strlen(needle)-1])) {
				needle[strlen(needle)-1] = '\0';
			}
			strncpy(port, needle, PATH_MAX);
		} else if ((needle = strstr(buf, "type:")) != 0) {
				needle += 5; 
				/* Trim whitespaces */
				while (isspace(*needle)) {
					needle++;
				}
			
				while (isspace(needle[strlen(needle)-1])) {
					needle[strlen(needle)-1] = '\0';
				}
					
				*kbdtype = find_kbd_type(needle);
			}
	}
}
          

int main(int argc, char **argv)
{
int optc;
int kbdtype=KBD_TYPE_NONE;

	while ((optc = getopt(argc, argv, "c:t:p:dh")) != -1) {
		switch ((char)optc) {
			case 'h':
				print_usage(argv[0]);
				exit(1);
				break;
			case 'd':
				debug = 1;
				break;
			case 't':
				kbdtype = find_kbd_type(optarg);
				break;
			case 'p':
				strncpy(TTY_PORT, optarg, PATH_MAX);
				break;
			case 'c':
				parse_config(optarg, &kbdtype, TTY_PORT);
				break;
		}
	}

	if (kbdtype == 0) {
		print_usage(argv[0]);
		exit(1);
	}
        
	uindev = dev_uinput_init();
	if (uindev <= 0) {
		fprintf(stderr, "init uinput failed\n");
		exit (1);
	}

	signal(SIGTERM, handle_sigterm);
	signal(SIGINT, handle_sigterm);

	if (kbdtype == KBD_TYPE_FOLDABLE)
		compaq_foldable();
	else if (kbdtype == KBD_TYPE_BTFOLDABLE)
		hp_bt_foldable();
	else if (kbdtype == KBD_TYPE_SNAPNTYPE)
		snapntype();
	else if (kbdtype == KBD_TYPE_SNAPNTYPEBT)
		snapntypebt();
	else if (kbdtype == KBD_TYPE_STOWAWAY)
		stowaway();
	else if (kbdtype == KBD_TYPE_STOWAWAYXT)
		stowawayxt();
	else if (kbdtype == KBD_TYPE_HPSLIM)
		hpslim();
	else if (kbdtype == KBD_TYPE_SMARTBT)
		smartbt();
	else if (kbdtype == KBD_TYPE_FLEXIS)
		flexis();
	else if (kbdtype == KBD_TYPE_BENQ_GAMEPAD)
		benqgamepad();
	else if (kbdtype == KBD_TYPE_POCKETVIK)
		pocketvik();
	else if (kbdtype == KBD_TYPE_MICRO_FOLDAWAY)
		micro_foldaway();
	else if (kbdtype == KBD_TYPE_MICRO_DATAPAD)
		micro_datapad();
	else if (kbdtype == KBD_TYPE_COMPAQ_MICROKBD)
		compaq_microkbd();
	else if (kbdtype == KBD_TYPE_LIRC)
#ifdef USELIRC
		lirc(basename(argv[0]);
#else
	{
		fprintf(stderr, "keyboard type 'lirc' is not supported in this version\n");
		exit(1);
	}
#endif
	else if (kbdtype == KBD_TYPE_BELKINIR)
		belkin_infrared();
	else if (kbdtype == KBD_TYPE_TARGUSIR)
		targus_infrared();
	else if (kbdtype == KBD_TYPE_FREEDOM)
		freedom_keyboard();
	else if (kbdtype == KBD_TYPE_ELEKTEX)
		elektex_keyboard(TRUE);
	else if (kbdtype == KBD_TYPE_ELEKTEX_UNMAPPED)
		elektex_keyboard(FALSE);

	dev_uinput_close(uindev);
return 0;
}
