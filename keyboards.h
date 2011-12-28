/*
 * Modified for userspace dameon purposes by
 * Nils Faerber <nils.faerber@kernelconcepts.de>
 * Siegen (Germany), 2004
 *
 * Based on:
 * Compaq Microkeyboard serial driver for the iPAQ H3800
 * 
 * Copyright 2002 Compaq Computer Corporation.
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * COMPAQ COMPUTER CORPORATION MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Author: Andrew Christian 
 *         <andyc@handhelds.org>
 *         13 February 2002
 *
 */

#include "uinput.h"

/* buttons on IPAQ 5450 (others too?) */
#define IPAQ_BUTTON_CALENDAR	219	/* keycode of calendar button */
#define IPAQ_BUTTON_CONTACTS	220	/* keycode of contacts button */
#define IPAQ_BUTTON_INBOX	155	/* keycode of email button */
#define IPAQ_BUTTON_ITASK	183	/* keycode of task (flip arrow) button */

/***********************************************************************************
 *   iConcepts
 * 
 *  9600 baud, 8N1
 *
 *  Notes:
 *   The shift, control, alt, and fn keys are the only keys that can be held
 *      down while pressing another key.  
 *   All other keys generate keycode for down and "0x85 0x85" for up.
 *
 *  It seems to want an ACK of some kind?
 *
 *              Down       Up
 *   Control    1          9e 9e
 *   Function   2          9d 9d       (we use this as AltGr)
 *   Shift      3          9c 9c 
 *   Alt        4          9b 9b       (we use this as Meta or Alt)
 *   OTHERS     keycode    85 85 
 *   
 ***********************************************************************************/

 unsigned char iconcepts_normal[128] = { 
        0, 29, 100, 42, 56, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        2, 10, 34, 24, 4, 30, 23, 16, 6, 46, 37, 31, 8, 18, 50, 22, 
        0, 0, 0, 0, 0, 0, 0, 0, 45, 39, 28, 105, 44, 51, 1, 123, 
        13, 53, 111, 125, 26, 15, 108, 87, 0, 0, 0, 0, 0, 0, 0, 0, 
        47, 49, 33, 9, 20, 38, 32, 7, 19, 36, 48, 5, 25, 35, 11, 3, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        122, 110, 52, 12, 124, 57, 40, 21, 106, 58, 43, 17, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 103, 41, 27,  };

 unsigned char iconcepts_numlock[128] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 73, 0, 77, 0, 0, 76, 0, 0, 0, 80, 0, 75, 0, 82, 75, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 74, 0, 0, 0, 51, 0, 0, 
        0, 78, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 72, 0, 81, 0, 0, 0, 79, 0, 0, 55, 0, 98, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 83, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };


/***********************************************************************************
 *   Snap N Type keyboard                                                         
 *
 * Key down sends single byte: KEY
 * Key up sends two bytes:     (KEY | 0x80), 0x7f
 *
 ***********************************************************************************/

#if 0
unsigned char snapntype_normal[128] = {
      /* 000, 001, 002, 003, 004, 005, 006, 007, 008, 009 */
/*000*/    0,   0,   0,   0,   0,  36,   0,   0,  22,   0,
/*010*/   47,  34,  48,   0,   0, 115,  50,   0,   0,   0,
/*020*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*030*/    0,   0,  65,   0,   0,   0,   0,   0,   0,   0,
/*040*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*050*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*060*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*070*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*080*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*090*/    0,   0,   0,   0,   0,   0,   0,  38,  56,  54,
/*100*/   40,  26,  41,  42,  43,  31,  44,  45,  46,  58,
/*110*/   57,  32,  33,  24,  27,  39,  28,  30,  55,  25,
/*120*/   53,  29,  52,   0,   0,   0,   0,   0 };

#else
static unsigned char snapntype_normal[128] = { 
        0, 0, 0, 0, 0, 29, 0, 0, 14, 0, 28, 104, 109, 0, 0, 1, 
        42, 0, 122, 123, 124, 125, 0, 0, 0, 0, 0, 100, 0, 0, 0, 0, 
        57, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 
        25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 0, 0, 0, 0, 0,  };

unsigned char snapntype_symbol[128] = {
      /*   0, 001, 002, 003, 004, 005, 006, 007, 008, 009 */
/*000*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*010*/    0,  99, 105,   0,   0,   0,  66,   0,   0,   0,
/*020*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*030*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*040*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*050*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*060*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*070*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*080*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*090*/    0,   0,   0,   0,   0,   0,   0,   0,  59,   0,
/*100*/    0,  12,   0,   0,   0,  17, 100,  98, 102, 104,
/*110*/   60,  18,  19,  10,  13,   0,  14,  16,   0,  11,
/*120*/    0,  15,  61,   0,   0,   0,   0,   0 };
#endif


/***********************************************************************************
 *   Snap N Type Bluetooth keyboard                                                         
 *
 * Key down sends tow bytes: 0x01, KEY
 * Key up sends two bytes:  (KEY | 0x80), (KEY | 0x80)
 *
 *
 * This keyboard is missing some useful keys, so I made Fn key a special modifier. You
 * can get the Page Up / Page Down button with Fn + Cursor Up / Fn + Cursor Down keys.
 * Pressing Fn + Cursor Left / Cursor Right you get Home / End keys.
 * Pressing Fn + 1 to 0 you get F1 to F10, and Fn + next two keys (minus and equal)
 * you get F11 and F12.
 * Finally you get Escape by pressing Fn + Backslash (labeled "Cancel"), and Compose key
 * with Fn + Del
 * Also, the following bindings apply for the special application keys in the left:
 * 1st app (Calendar): Right Alt (AltGr)
 * 2nd app (?): Insert
 * 3rd app (To-Do): SysRequest / PrintScreen
 * 4rd app (Clipboard): Pause
 *
 ***********************************************************************************/

unsigned char snapntypebt_normal[128] = {
/*000*/    0,   0,   0,   0,   0,  0,   0,   0,  KEY_BACKSPACE,   KEY_TAB,
/*010*/    0,   0,   0,   KEY_ENTER,   0,  0,   KEY_LEFTSHIFT,   KEY_LEFTCTRL,  KEY_RIGHTSHIFT,   KEY_DELETE,
/*020*/    KEY_CAPSLOCK,   0,   0,   0,   0,   0,   0,   KEY_GRAVE,   KEY_LEFTMETA,   KEY_LEFTALT,
/*030*/    0,   0,   KEY_SPACE,   KEY_SPACE,   0,   0,   0,   KEY_RIGHT,   KEY_UP,   KEY_LEFT,
/*040*/    KEY_DOWN,   0,   0,   0,   KEY_APOSTROPHE,   KEY_MINUS,   KEY_DOT,   KEY_SLASH,   KEY_0,   KEY_1,
/*050*/    KEY_2,   KEY_3,   KEY_4,   KEY_5,   KEY_6,   KEY_7,   KEY_8,   KEY_9,   0,   KEY_SEMICOLON,
/*060*/    KEY_COMMA,   KEY_EQUAL,   0,   0,   0,   0, KEY_B,   KEY_C,   KEY_D,   KEY_E, 
/*070*/    KEY_F,   KEY_G,   KEY_H,   KEY_I,   KEY_J,   KEY_K,   KEY_L,   KEY_M,   KEY_N,   KEY_O, 
/*080*/    KEY_P,   KEY_Q,   KEY_R,   KEY_S,   KEY_T,   KEY_U,   KEY_V,   KEY_W,   KEY_X,   KEY_Y,
/*090*/    KEY_Z,   KEY_LEFTBRACE,   KEY_BACKSLASH, KEY_RIGHTBRACE,   KEY_A,   0,   0,   0,  0,  0,  
/*100*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*010*/    0,   0,   0,   KEY_RIGHTALT,   KEY_INSERT,   KEY_SYSRQ,   KEY_PAUSE,   0,   0,   0,
/*120*/    0,   0,   0,   0,   0,   0,   0,   0 };


unsigned char snapntypebt_fn[128] = {
      /*   0, 001, 002, 003, 004, 005, 006, 007, 008, 009 */
/*000*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*010*/    0,  0, 0,   0,   0,   0,  0,   0,   0,   KEY_COMPOSE,
/*020*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*030*/    0,   0,   0,   0,   0,   0,   0,   KEY_END,   KEY_PAGEUP,   KEY_HOME,
/*040*/    KEY_PAGEDOWN,   0,   0,   0,   0,   KEY_F11,   0,   0,   KEY_F10,   KEY_F1,
/*050*/    KEY_F2,   KEY_F3,   KEY_F4,   KEY_F5,   KEY_F6,   KEY_F7,   KEY_F8,   KEY_F9,   0,   0,
/*060*/    0,   KEY_F12,   0,   0,   0,   0,   0,   0,   0,   0,
/*070*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*080*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*090*/    0,   0,   KEY_ESC,   0,   0,   0,   0,   0,  0,   0,
/*100*/    0,  0,   0,   0,   0,  0, 0,  0, 0, 0,
/*110*/   0,  0,  0,  0,  0,   0,  0,  0,   0,  0,
/*120*/    0,  0,  0,   0,   0,   0,   0,   0 };



/***********************************************************************************
 *   Compaq Microkeyboard
 *
 * 4800 baud, 8N1
 *
 * Key down sends two bytes:  KEY          ~KEY (complement of KEY)
 * Key up sends two bytes:    (KEY | 0x80) ~KEY
 ***********************************************************************************/

 unsigned char compaq_normal[128] = { 
        0, 0, 100, 0, IPAQ_BUTTON_INBOX, IPAQ_BUTTON_CALENDAR, IPAQ_BUTTON_CONTACTS, 0, 0, 0, 0, 0, IPAQ_BUTTON_ITASK, 0, 0, 0, 
        0, 0, 42, 0, 0, 16, 0, 0, 0, 29, 44, 31, 30, 17, 0, 0, 
        0, 46, 45, 32, 18, 0, 0, 0, 103, 0, 47, 33, 20, 19, 0, 106, 
        0, 49, 48, 35, 34, 21, 0, 0, 0, 0, 50, 36, 22, 0, 0, 0, 
        0, 0, 37, 23, 24, 0, 0, 0, 0, 0, 0, 38, 0, 25, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 58, 0, 28, 0, 57, 0, 105, 0, 
        108, 0, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

 unsigned char compaq_function[128] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 42, 0, 0, KEY_1, 0, 0, 0, KEY_ESC, 44, KEY_SLASH, 30, KEY_2, 0, 0, 
        0, 46, 45, 32, KEY_3, 0, 0, 0, 103, 0, 47, KEY_MINUS, KEY_5, KEY_4, 0, 106, 
        0, KEY_COMMA, 48, KEY_SEMICOLON, KEY_EQUAL, KEY_6, 0, 0, 0, 0, KEY_DOT, KEY_APOSTROPHE, KEY_7, 0, 0, 0, 
        0, 0, 37, KEY_8, KEY_9, 0, 0, 0, 0, 0, 0, 38, 0, KEY_0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, KEY_NUMLOCK, 0, KEY_TAB, 0, 57, 0, 105, 0, 
        108, 0, 0, 0, 0, 0, KEY_DELETE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

/***********************************************************************************
 *   HP iPAQ  Bluetooth Foldable
 *
 * Key down sends one byte:  (KEY)
 * Key up sends  one byte:    (KEY | 0x80)
 ***********************************************************************************/

 /* normal keys */

 unsigned char btfoldable_normal[128] = { 
 /*0x00*/ 0, 0, 0, 0, 0, 0, 0, KEY_LEFTMETA, KEY_ESC, 0, 0, 0, 0, KEY_TAB, KEY_GRAVE, 0, 
 /*0x10*/ KEY_LEFTALT, 0, KEY_LEFTSHIFT,KEY_RIGHTALT, KEY_LEFTCTRL, KEY_Q, KEY_1, 0, 0, 0, KEY_Z, KEY_S, KEY_A, KEY_W, KEY_2, 0, 
 /*0x20*/ 0, KEY_C, KEY_X, KEY_D, KEY_E, KEY_4, KEY_3, 0, KEY_UP, 0, KEY_V, KEY_F, KEY_T, KEY_R, KEY_5, KEY_RIGHT, 
 /*0x30*/ 0, KEY_N, KEY_B, KEY_H, KEY_G, KEY_Y, KEY_6, 0, 0, 0, KEY_M, KEY_J, KEY_U, KEY_7, KEY_8, 0, 
 /*0x40*/ 0, KEY_COMMA, KEY_K, KEY_I, KEY_O, KEY_0, KEY_9, 0, 0, KEY_DOT, KEY_SLASH, KEY_L, KEY_SEMICOLON, KEY_P, KEY_MINUS, 0, 
 /*0x50*/ 0, 0, KEY_APOSTROPHE, 0, KEY_LEFTBRACE, KEY_EQUAL, 0, 0, KEY_CAPSLOCK, KEY_RIGHTSHIFT, KEY_ENTER, KEY_RIGHTBRACE, KEY_SPACE, KEY_BACKSLASH, KEY_LEFT, 0, 
 /*0x60*/ KEY_DOWN, 0, 0, 0, 0, 0, KEY_DELETE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
 /*0x70*/ 0, KEY_BACKSPACE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

/*
 keys with blue Fn key, contains also modifiers (shift,alt,ctrl,..) to let them work also with Fn pressed 
 some blue symbols are also mapped to its equivalent but some are not

 Fn keyboard labels:
 q=mail,r=calendar,t=todo,y=document viewer?,backspace=off,baskslash=OK,a=file manager?,s=today?,d=web browser,f=itask,
 ;=pgup, '=pgdown, /=battery symbol, left=home,right=end

 real Fn mappings:
 q=mail, d=www, f=menu, ;=pageup, '=pagedown,left=home,right=end,backspace=sleep
 1-0,-,= =F1-F12
 up=pageup,down=pagedown
*/

unsigned char btfoldable_function[128] = { 
/*0x00*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/*0x10*/ KEY_LEFTALT, 0, KEY_LEFTSHIFT, KEY_RIGHTALT, KEY_LEFTCTRL, KEY_MAIL, KEY_F1, 0, 0, 0, 0, 0, 0, 0, KEY_F2, 0, 
/*0x20*/ 0, 0, 0, KEY_WWW, 0, KEY_F4, KEY_F3, 0, KEY_PAGEUP, 0, 0, KEY_MENU, 0, 0, KEY_F5, KEY_END, 
/*0x30*/ 0, 0, 0, 0, 0, 0, KEY_F6, 0, 0, 0, 0, 0, 0, KEY_F7, KEY_F8, 0,
/*0x40*/ 0, 0, 0, 0, 0, KEY_F10, KEY_F9, 0, 0, 0, 0, 0, KEY_PAGEUP, 0, KEY_F11, 0, 
/*0x50*/ 0, 0, KEY_PAGEDOWN, 0, 0, KEY_F12, 0, 0, 0, 0, 0, 0, 0, 0, KEY_HOME, 0, 
/*0x60*/ KEY_PAGEDOWN, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/*0x70*/ KEY_SLEEP, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };
/***********************************************************************************
 *   Targus Stowaway keyboard                                                     
 *
 * 9600 baud, 8N1
 *
 * Initialisation: raise DTR and drop RTS, wait for DCD pulse, then raise RTS
 *   keyboard will then send back 0xFA 0xFD
 *
 * Key down sends one byte
 * Key up sends one byte & 0x80, and if the key up is the last key up (ie, no more
 *   keys held down), then the key code & 0x80 is repeated
 ***********************************************************************************/

static unsigned char stowaway_normal[128] = {
      /*   0, 001, 002, 003, 004, 005, 006, 007, 008, 009 */
/*000*/    2,   3,   4,  44,   5,   6,   7,   8,   0,  16,
/*010*/   17,  18,  19,  20,  21,  41,  45,  30,  31,  32,
/*020*/   33,  34,  35,  57,  58,  15,  29,   0,   0,   0,
/*030*/    0,   0,   0,   0,   0,  56,   0,   0,   0,   0,
/*040*/    0,   0,   0,   0,  46,  47,  48,  49,  12,  13,
/*050*/   14,  87,   9,  10,  11,  57,  26,  27,  43, 220,
/*060*/   22,  23,  24,  25,  40,  28, 219,   0,  36,  37,
/*070*/   38,  39,  53, 144, 183,   0,  50,  51,  52,   0,
/*080*/  111, 146, 155, 151,   0,   0,   0,   0,  42,  54,
/*090*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*100*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*110*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*120*/    0,   0,   0,   0,   0,   0,   0,   0,  };

static unsigned char stowaway_function[128] = {
      /*   0, 001, 002, 003, 004, 005, 006, 007, 008, 009 */
/*000*/   59,  60,  61,   0,  62,  63,  64,  65,   0,   0,
/*010*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*020*/    0,   0,   0,   0,   0,   1,   0,   0,   0,   0,
/*030*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*040*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*050*/   92,   0,  66,  67,  68,   0,   0,   0,   0,   0,
/*060*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*070*/    0,   0,   0, 104,   0,   0,   0,   0,   0,   0,
/*080*/    0, 102, 109, 107,   0,   0,   0,   0,   0,   0,
/*090*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*100*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*110*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
/*120*/    0,   0,   0,   0,   0,   0,   0,   0,  };
 
/***********************************************************************************
 *   Stowaway XT
 *
 *  9600 baud, 8N1
 *
 *  Notes:
 *   the green function key is basically shift + scancode - handled elsewhere
 *
 ***********************************************************************************/
unsigned char stowawayxt_normal[128] = {
  /* 000 */ 0, 0, 0, KEY_Z, 0, 0, 0, 0, KEY_LEFTMETA, KEY_Q,
  /* 010 */ KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, 0,KEY_X, KEY_A, KEY_S, KEY_D,
  /* 020 */ KEY_F, KEY_G, KEY_H, KEY_SPACE, KEY_CAPSLOCK, KEY_TAB, KEY_LEFTCTRL, 0, 0, 0,
  /* 030 */ 0, 0, 0, 0, 0, KEY_LEFTALT, 0, 0, 0, 0,
  /* 040 */ 0, 0, 0, 0, KEY_C, KEY_V, KEY_B, KEY_N, 0, 0,
  /* 050 */ KEY_BACKSPACE, 0, 0, 0, 0, KEY_SPACE, KEY_MINUS, KEY_EQUAL, KEY_SLASH, 0,
  /* 060 */ KEY_U, KEY_I, KEY_O, KEY_P, KEY_APOSTROPHE, KEY_ENTER, 0, 0, KEY_J, KEY_K,
  /* 070 */ KEY_L, KEY_SEMICOLON, KEY_UP, 0, 0, 0, KEY_M, KEY_COMMA, KEY_DOT, 0,
  /* 080 */ KEY_DELETE, KEY_LEFT, KEY_DOWN, KEY_RIGHT, 0, 0, 0, 0, KEY_LEFTSHIFT, KEY_RIGHTSHIFT,
  /* 090 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 100 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 110 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 120 */ 0, 0, 0, 0, 0, 0, 0, 0
};

unsigned stowawayxt_function[128] = {
  /* 000 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_1,
  /* 010 */ KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, 0, 0, KEY_F9, KEY_F10, KEY_F11,
  /* 020 */ KEY_F12, 0, 0, 0, KEY_NUMLOCK, KEY_ESC, 0, 0, 0, 0,
  /* 030 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 040 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 050 */ 0, 0, 0, 0, 0, 0, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_BACKSLASH, 0,
  /* 060 */ KEY_7, KEY_8, KEY_9, KEY_0, 0, 0, 0, 0, 0, 0,
  /* 070 */ 0, KEY_WWW, KEY_UP, 0, 0, 0, 0, KEY_INTL1, KEY_INTL2, 0,
  /* 080 */ 0, KEY_LEFT, KEY_DOWN, KEY_RIGHT, 0, 0, 0, 0, 0, 0,
  /* 090 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 100 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 110 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 120 */ 0, 0, 0, 0, 0, 0, 0, 0
};

/***********************************************************************************
 *   HP foldable keyboard                                                     
 *
 * 4800 baud, 8N1
 *
 * Key down sends two bytes:  KEY          ~KEY (complement of KEY)
 * Key up sends two bytes:    (KEY | 0x80) ~KEY
 ***********************************************************************************/
#if 0
unsigned char foldable_normal[128] = {
      /* 000,   001,   002,   003,   004,   005,   006,   007,   008,   009 */
/*000*/  000,   000,   000,   000,   000,   000,   000,   115,   000,   000,
/*010*/  000,   000,   000,    23,    49,   000,   000,   113,    50,   000,
/*020*/   37,    24, KEY_1,   000,   000,   000,    52,    39,    38,    25,
/*030*/  KEY_2, 000,   000,    54,    53,    40,    26, KEY_4, KEY_3,   000,
/*040*/   98,   000,    55,    41,    28,    27, KEY_5,   102,   000,    57,
/*050*/   56,    43,    42,    29, KEY_6,   000,   000,   000,    58,    44,
/*060*/   30, KEY_7, KEY_8,   000,   000,    59,    45,    31,    32, KEY_0,
/*070*/  KEY_9, 000,   000,    60,    61,    46,    47,    33,    20,   000,
/*080*/  000,   000,    51,   000,    34,    21,   000,   000,    66,    62,
/*090*/   36,    35,    65,    48,   100,   000,   104,   000,   000,   000,
/*100*/  000,   000,   107,   000,   000,   000,   000,   000,   000,   000,
/*110*/  000,   000,   112,    22,   000,   109,    94,   000,   000,   000,
/*120*/  000,   000,   000,   000,   000,   000,   000,   000 };

 unsigned char foldable_function[128] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
        0, 0, 0, 0, 0, 124, 59, 0, 0, 0, 0, 0, 123, 85, 60, 0, 
        0, 0, 0, 0, 89, 62, 61, 0, 104, 0, 0, 0, 125, 122, 63, 107, 
        0, 0, 0, 0, 0, 90, 64, 0, 0, 0, 0, 0, 0, 65, 66, 0, 
        0, 0, 0, 0, 0, 68, 67, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 69, 0, 91, 0, 0, 0, 102, 0, 
        109, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        125, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

 unsigned char foldable_numlock[128] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 79, 75, 71, 72, 0, 
        0, 0, 80, 76, 77, 55, 73, 0, 0, 83, 98, 81, 78, 74, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };
#else
static unsigned short foldable_normal[128] = { 
        0,  0, 128, 0, 0, 0, 0, 87, 0, 0, 0, 0, 0, 15, 41, 0, 
        0, 134 /*100*/ /*AltGR*/, 42, 0, 29, 16, 2, 0, 0, 0, 44, 31, 30, 17, 3, 0, 
        0, 46, 45, 32, 18, 5, 4, 0, 144/*103*/, 0, 47, 33, 20, 19, 6, 151/*106*/, 
        0, 49, 48, 35, 34, 21, 7, 0, 0, 0, 50, 36, 22, 8, 9, 0, 
        0, 51, 37, 23, 24, 11, 10, 0, 0, 52, 53, 38, 39, 25, 12, 0, 
        0, 0, 43, 0, 26, 13, 0, 0, 58, 54, 28, 27, 57, 40, 146 /*105 lft*/, 0, 
        161/*108*/, 0, 0, 0, 0, 0, 111, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        92, 14, 0, 97, 86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

static unsigned short foldable_function[128] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
        0, 0, 0, 0, 0, 124, 59, 0, 0, 0, 0, 0, 123, 85, 60, 0, 
        0, 0, 0, 0, 89, 62, 61, 0, 104, 0, 0, 0, 125, 122, 63, 107, 
        0, 0, 0, 0, 0, 90, 64, 0, 0, 0, 0, 0, 0, 65, 66, 0, 
        0, 0, 0, 0, 0, 68, 67, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 69, 0, 91, 0, 0, 0, 102, 0, 
        109, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        125, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

#if 0
static unsigned short foldable_numlock[128] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 82, 79, 75, 71, 72, 0, 
        0, 0, 80, 76, 77, 55, 73, 0, 0, 83, 98, 81, 78, 74, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };
#endif
#endif


/***********************************************************************************
 * Eagle-Touch Portable PDA keyboard
 *
 * 9600 baud, 8N1
 *
 * Key down sends:  KEY
 * Key up sends    ~KEY
 *
 * After a delay, keyboard sends 74 to indicate it's going low power
 * When you press a key after the delay, it sends:  75 6d 61 78 
 *   followed by the normal KEY, ~KEY
 ***********************************************************************************/

 unsigned char eagletouch_normal[128] = { 
        0, 0, 128, 122, 88, 87, 124, 0, 85, 0, 89, 123, 90, 15, 40, 0, 
        0, 125, 42, 0, 29, 16, 2, 0, 0, 0, 44, 31, 30, 17, 3, 111, 
        0, 46, 45, 32, 18, 5, 4, 0, 103, 57, 47, 33, 20, 19, 6, 106, 
        100, 49, 48, 35, 34, 21, 7, 0, 91, 0, 50, 36, 22, 8, 9, 0, 
        56, 51, 37, 23, 24, 11, 10, 1, 0, 52, 53, 38, 39, 25, 12, 0, 
        0, 0, 40, 0, 26, 13, 0, 0, 58, 54, 28, 27, 0, 43, 105, 0, 
        108, 129, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 129, 0, 0, 
        0, 0, 0, 0, 129, 129, 0, 0, 129, 0, 0, 0, 0, 0, 0, 0,  };

 unsigned char eagletouch_function[128] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 59, 0, 0, 0, 0, 0, 0, 0, 60, 0, 
        0, 0, 0, 0, 0, 62, 61, 0, 104, 0, 0, 0, 0, 0, 63, 107, 
        0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 65, 66, 0, 
        0, 0, 0, 0, 0, 68, 67, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 102, 0, 
        109, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };


/***********************************************************************************
 * Micro Innovations IR keyboard
 *
 * 9600 baud, 8N1
 *
 * Key down sends one byte:  KEY
 * Key up sends one byte:    KEY | 0x80
 * Last key up repeats key up
 ***********************************************************************************/

 unsigned char microinnovations_normal[128] = { 
        0, 16, 0, 44, 0, 0, 0, 30, 0, 17, 0, 45, 0, 0, 0, 31, 
        0, 18, 0, 46, 0, 0, 0, 32, 0, 19, 0, 47, 0, 0, 0, 33, 
        0, 20, 0, 48, 0, 0, 0, 34, 0, 21, 0, 57, 0, 0, 0, 35, 
        0, 22, 36, 49, 57, 0, 0, 0, 0, 23, 37, 50, 128, 0, 0, 0, 
        122, 24, 38, 51, 105, 0, 0, 0, 123, 25, 39, 52, 108, 0, 0, 0, 
        125, 12, 40, 103, 106, 0, 0, 0, 90, 14, 28, 111, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 42, 56, 0, 0, 15, 0, 0, 0, 29, 69, 0, 
        0, 0, 0, 0, 0, 87, 100, 58, 0, 0, 0, 0, 0, 54, 0, 0,  };

 unsigned char microinnovations_function[128] = { 
        0, 59, 0, 117, 0, 0, 0, 92, 0, 60, 0, 126, 0, 0, 0, 93, 
        0, 61, 0, 127, 0, 0, 0, 111, 0, 62, 0, 113, 0, 0, 0, 94, 
        0, 63, 0, 124, 0, 0, 0, 95, 0, 64, 0, 0, 0, 0, 0, 99, 
        0, 65, 0, 0, 0, 0, 0, 0, 0, 66, 0, 0, 0, 0, 0, 0, 
        88, 67, 0, 0, 102, 0, 0, 0, 85, 68, 0, 0, 109, 0, 0, 0, 
        89, 0, 0, 104, 107, 0, 0, 0, 119, 91, 116, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

 unsigned char microinnovations_numlock[128] = { 
        0, 79, 0, 0, 0, 0, 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 
        0, 81, 0, 0, 0, 0, 0, 0, 0, 75, 0, 0, 0, 0, 0, 0, 
        0, 76, 0, 0, 0, 0, 0, 0, 0, 77, 0, 0, 0, 0, 0, 0, 
        0, 71, 75, 0, 0, 0, 0, 0, 0, 72, 76, 79, 82, 0, 0, 0, 
        0, 73, 77, 80, 83, 0, 0, 0, 0, 82, 55, 81, 78, 0, 0, 0, 
        0, 98, 0, 74, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };


/***********************************************************************************
 * Micro Innovations Foldaway keyboard
 *
 * 9600 baud, 8N1
 *
 * Key down sends one byte:  KEY
 * Key up sends one byte:    KEY | 0x80
 * Last key up repeats key up
 ***********************************************************************************/

unsigned char micro_foldaway_normal[128] = {
  /* 00 */ 0,              KEY_LEFTCTRL,  0,              KEY_LEFTSHIFT, KEY_LEFTALT,
  /* 05 */ 0,              0,             0,              0,             0,
  /* 10 */ 0,              0,             0,              0,             0,
  /* 15 */ 0,              KEY_1,         KEY_9,          KEY_G,         KEY_O,
  /* 20 */ KEY_3,          KEY_A,         KEY_I,          KEY_Q,         KEY_5,
  /* 25 */ KEY_C,          KEY_K,         KEY_S,          KEY_7,         KEY_E,
  /* 30 */ KEY_M,          KEY_U,         0,              0,             0,
  /* 35 */ 0,              0,             0,              0,             0,
  /* 40 */ KEY_X,          KEY_SEMICOLON, KEY_ENTER,      KEY_LEFT,      KEY_Z,
  /* 45 */ KEY_COMMA,      KEY_SPACE,     IPAQ_BUTTON_CONTACTS, KEY_EQUAL, KEY_SLASH,
  /* 50 */ KEY_DELETE,     IPAQ_BUTTON_ITASK, KEY_LEFTBRACE, KEY_TAB,    KEY_DOWN,
  /* 55 */ KEY_LEFTMETA,   0,             0,              0,             0,
  /* 60 */ 0,              0,             0,              0,             KEY_V,
  /* 65 */ KEY_N,          KEY_F,         KEY_8,          KEY_T,         KEY_L,
  /* 70 */ KEY_D,          KEY_6,         KEY_R,          KEY_J,         KEY_B,
  /* 75 */ KEY_4,          KEY_P,         KEY_H,          KEY_0,         KEY_2,
  /* 80 */ 0,              0,             0,              0,             0,
  /* 85 */ 0,              0,             0,              0,             0,
  /* 90 */ 0,              0,             0,              0,             0,
  /* 95 */ 0,              IPAQ_BUTTON_CALENDAR, 0,       KEY_DOT,       KEY_MINUS,
  /*100 */ IPAQ_BUTTON_INBOX, KEY_SPACE,   KEY_APOSTROPHE, KEY_Y,        KEY_RIGHT,
  /*105 */ KEY_CAPSLOCK,   KEY_BACKSLASH, KEY_W,          0,             0,
  /*110 */ 0,              0,             0,              0,             0,
  /*115 */ 0,              0,             0,              0,             0,
  /*120 */ 0,              0,             0,              0,             KEY_BACKSPACE,
  /*125 */ KEY_UP,         KEY_GRAVE,     KEY_RIGHTBRACE
};

/***********************************************************************************
 * Micro Innovations Datapad
 *
 * 9600 baud, 8N1
 *
 * Same protocol as MicroInnovations Foldaway
 ***********************************************************************************/

unsigned char micro_datapad_normal[128] = {
  /* 00 */ 0,              0,             0,              0,             0,
  /* 05 */ 0,              0,             0,              0,             0,
  /* 10 */ 0,              0,             0,              0,             0,
  /* 15 */ 0,              KEY_A,         KEY_B,          KEY_C,         KEY_D,
  /* 20 */ KEY_Q,          KEY_R,         KEY_S,          KEY_T,         KEY_RIGHT,
  /* 25 */ KEY_LEFTMETA,   0,             KEY_SPACE,      0,             0,
  /* 30 */ 0,              0,             0,              0,             0,
  /* 35 */ 0,              0,             0,              0,             0,
  /* 40 */ KEY_M,          KEY_N,         KEY_O,          KEY_P,         KEY_ENTER,
  /* 45 */ KEY_UP,         KEY_DOWN,      KEY_LEFT,       KEY_LEFTBRACE, KEY_RIGHTBRACE,
  /* 50 */ KEY_CAPSLOCK,   0,             0,              0,             0,
  /* 55 */ 0,              0,             0,              0,             0,
  /* 60 */ 0,              0,             0,              0,             0,
  /* 65 */ 0,              0,             0,   IPAQ_BUTTON_ITASK, IPAQ_BUTTON_CALENDAR,
  /* 70 */ IPAQ_BUTTON_CONTACTS, IPAQ_BUTTON_INBOX, KEY_DELETE, KEY_BACKSPACE, KEY_Z,
  /* 75 */ KEY_Y,          KEY_L,         KEY_K,          KEY_J,         KEY_I,
  /* 80 */ 0,              0,             0,              0,             0,
  /* 85 */ 0,              0,             0,              0,             0,
  /* 90 */ 0,              0,             0,              0,             0,
  /* 95 */ 0,              KEY_TAB,       KEY_APOSTROPHE, KEY_GRAVE,     KEY_SEMICOLON,
  /*100 */ KEY_X,          KEY_W,         KEY_V,          KEY_U,         KEY_H,
  /*105 */ KEY_G,          KEY_F,         KEY_E,          0,             0,
  /*110 */ 0,              0,             0,              0,             0,
  /*115 */ 0,              0,             0,              0,             0,
  /*120 */ 0,              0,             0,              0,             0,
  /*125 */ 0,              0,             0
};


/***********************************************************************************
 * Grandtec PocketVIK     (www.grandtec.com)
 *
 * 57600 baud, 8N1
 *
 *  This keyboard autorepeats
 *  It does not generate key up
 *
 *  It may have a low power sleep mode that needs to be cycled????
 *
 *  Set 1:   ShiftL, ShiftR, CtrlL, AltL, AltR, CtrlR, Fn
 *  Set 2:   All others
 *
 *  Set 1 keys do not generate keystrokes.  They set fields in a bit mask.
 *
 *  MASK = 0x80 | (sum of active modifiers)
 *
 *  On keydown, you either get
 *
 *  Case 1 (no bit mask field) 	   KEY
 *  Case 2 (bitmask field)         MASK, KEY
 ***********************************************************************************/

 unsigned char pocketvik_normal[128] = { 
        0, 108, 105, 51, 0, 0, 57, 46, 0, 50, 103, 53, 52, 0, 106, 47, 
        0, 44, 87, 28, 40, 0, 37, 35, 34, 45, 31, 0, 27, 39, 11, 36, 
        21, 33, 32, 30, 16, 26, 25, 23, 22, 20, 19, 18, 17, 15, 13, 58, 
        10, 9, 7, 6, 4, 3, 41, 14, 24, 12, 8, 5, 0, 122, 2, 38, 
        111, 1, 48, 43, 14, 125, 123, 124, 49, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

 unsigned char pocketvik_function[128] = { 
        0, 109, 102, 0, 0, 0, 0, 0, 0, 0, 104, 0, 0, 0, 107, 0, 
        0, 0, 0, 91, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 89, 0, 0, 
        88, 0, 0, 0, 0, 92, 85, 90, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };

#define POCKETVIK_Fn		0x40
struct {
	unsigned char mask;
	unsigned char key;
} pocketvik_modifiers[] = {
	{ 0x01, KEY_LEFTSHIFT },
	{ 0x02, KEY_RIGHTSHIFT },
	{ 0x04, KEY_LEFTCTRL },
	{ 0x08, KEY_RIGHTCTRL },
	{ 0x10, KEY_LEFTALT },
	{ 0x20, KEY_RIGHTALT }
};

/***********************************************************************************
 * Flexis FX-100
 *
 * 9600 baud, 8N1
 *
 * This keyboard autorepeats
 * It does not generate key up for most keys
 *
 * Set 1:   ShiftL, ShiftR, Ctrl, Alt, Fn
 * Set 2:   All others
 *
 *             Down       	  Up
 *             ====		  ==
 *  Set 1      KEY | 0x80	  KEY
 *  Set 2      KEY | 0x80, KEY    <none>
 *  
 * If you press two keys from Set 1 you get the following:
 *
 * Press Key #1			KEY1_DOWN
 * Press Key #2			KEY1_UP
 * Release Key #1		KEY2_DOWN
 * Release Key #2		KEY2_UP
 ***********************************************************************************/

 unsigned char flexis_fx100_normal[128] = { 
        30, 31, 32, 33, 35, 34, 44, 45, 46, 47, 1, 48, 16, 17, 18, 19, 
        21, 20, 2, 3, 4, 5, 7, 6, 13, 10, 8, 12, 9, 11, 27, 24, 
        22, 26, 23, 25, 28, 38, 36, 40, 37, 39, 43, 51, 53, 49, 50, 52, 
        15, 57, 41, 14, 0, 0, 0, 56, 54, 58, 128, 29, 42, 87, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 111, 105, 106, 108, 103, 0,  };

 unsigned char flexis_fx100_function[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 123, 88, 85, 89, 0, 0, 90, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 91, 0, 0, 0, 92, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 102, 107, 109, 104, 0,  };

/***********************************************************************************
 * HP Slim  keyboard
 *
 * 4800 baud, 8N1
 *
 * Key down sends one byte:  KEY
 * Key up sends one byte:    KEY | 0x80
 * Keys don't need translation (except for arrow keys).
 * If Fn key is pressed, then hpslim_symbol maps is used to translate
 * key codes.  Shift must be added to some symbol key codes.
 * For example Fn-Z is shown on the keyboard as a dollar sign,
 * so we translate it to  shift-4.
 * Haven't yet figured out the euro symbol, or the British pound.
 *
 * HP Slim Keyboard stuff blame to Tom Swiss tms@infamous.net
 * More stuff added by Gordon Maclean (maclean at ucar edu).
 ****************************************************************************/

 /* translations for keys pressed without Fn.
  * Not much translation necessary except for arrow keys
  * (KP_4,6,2,8)=(75,77,80,72)->(146,151,161,144)
  * and IPAQ buttons (113,114,117,116)->(219,220,155,183)
  */
 unsigned char hpslim_normal[128] = { 
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
        44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
        55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71,144, 73, 74,146, 76,151, 78, 79,
       161, 81, 82, 83, 84, 85, 86, 87, 88, 89,
        90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
       100,101,102,103,104,105,106,107,108,109,
       110,111,112,219,220,115,155,183,118,119,
       120,121,122,123,124,125,126,127
};

 /* translations for Fn-key sequences */
 unsigned char hpslim_symbol[128] = { 
/*code   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15 */
/*key                                                            BS     */
/*sym                                                           DEL     */
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,111,  0, 

/*code  16  17  18  19  20  21  22  23  24  25  26  27  28  29 */
/*key    Q   W   E   R   T   Y   U   I   O   P  [   ]   CR  LC */
/*sym    1   2   3   4   5   6   7   8   9   0          TB ESC */
         2,  3,  4,  5,  6,  7,  8,  9, 10, 11,  0,  0, 15,  1,
	
/*code  30  31  32  33  34  35  36  37  38  39  40  41  42  43 */
/*key    A   S   D   F   G   H   J   K   L   ;  AP GRAV LS   \ */
/*sym    *   /   +   -    =  :   '   "   @                     */
        55, 53, 78, 12, 13, 39, 41, 40,  3,  0,  0,  0,  0,  0,
/*shft                       y       y   y                     */

/*code  44  45  46  47  48  49  50  51  52  53  54  */
/*key    Z   X   C   V   B   N   M   ,   .   /  RS  */
/*sym    $   (   )   ?   !   ,   .  NUM             */
         5, 10, 11, 53,  2, 51, 52, 69,  0,  0,  0,
/*shft   y   y   y   y   y                          */

/*code  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69 */
/*key              CAP                                             */
/*sym              NUM                                             */
         0,  0,  0, 69,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

/*code  70  71  72  73  74  75  76  77  78  79 */
/*key          KP8         KP4     KP6         */
/*sym            _           %      \          */
         0,  0, 12,  0,  0,  6,  0, 43,  0,  0,
/*shft           y           y                 */

/*code  80  81  82  83  84  85  86  87  88  89 */
/*key  KP2                                     */
/*sym  euro	                               */
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

/*code  90  91  92  93  94  95  96  97  98  99 */
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

/*code 100 101 102 103 104 105 106 107 108 109 */
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0};

 /* whether we need to add a shft to get a symbol */
 unsigned char hpslim_symshft[128] = { 
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*0-15*/
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	/*16-29*/
         0,  0,  0,  0,  0,  1,  0,  1,  1,  0,  0,  0,  0,  0, /*30-43*/
         1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,		/*44-54*/
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*55-69*/
         0,  0,  1,  0,  0,  1,  0,  0,  0,  0,		/*70-79*/
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,		/*80-89*/
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,		/*90-99*/
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,		/*100-109*/
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,		/*110-119*/
	 0,  0,  0,  0,  0,  0,  0,  0};


#define MKBD_HPS_LOCKKEY 0x3a
#define MKBD_HPS_NUMLOCKSCAN 0x45
#define MKBD_HPS_FNKEY 0x73


/***********************************************************************************
 *
 *  Smart Bluetooth Keyboard
 *
 *  2400 baud, 8N1
 *
 ***********************************************************************************/
unsigned char smartbt_normal[64] = {
  /* 00 */ 0,              KEY_1,      KEY_2,         KEY_3,         KEY_4,          
  /* 05 */ KEY_5,          KEY_6,      KEY_Q,         KEY_W,         KEY_E,
  /* 10 */ KEY_R,          KEY_T,      KEY_A,         KEY_S,         KEY_D,          
  /* 15 */ KEY_F,          KEY_G,      KEY_Z,         KEY_X,         KEY_C,
  /* 20 */ KEY_V,          KEY_B,      KEY_SPACE,     KEY_TAB,       KEY_CAPSLOCK,   
  /* 25 */ KEY_LEFTMETA,   KEY_7,      KEY_8,         KEY_9,         KEY_0,
  /* 30 */ KEY_MINUS,      KEY_EQUAL,  KEY_BACKSPACE, KEY_Y,         KEY_U,          
  /* 35 */ KEY_I,          KEY_O,      KEY_P,         KEY_LEFTBRACE, KEY_RIGHTBRACE,
  /* 40 */ KEY_H,          KEY_J,      KEY_K,         KEY_L,         KEY_SEMICOLON,  
  /* 45 */ KEY_APOSTROPHE, KEY_ENTER,  KEY_N,         KEY_M,         KEY_COMMA,
  /* 50 */ KEY_DOT,        KEY_SLASH,  KEY_UP,        KEY_SPACE,     KEY_F24, 
  /* 55 */ KEY_LEFT,       KEY_DOWN,   KEY_RIGHT,     KEY_LEFTCTRL,  KEY_RIGHTSHIFT,
  /* 60 */ KEY_LEFTSHIFT,  KEY_DELETE, KEY_LEFTALT,   KEY_INTL1
};

/***********************************************************************************
 *	Freedom keyboard
 *
 *	David Lapetina ntic at lapetina dot org
 *      Fixes from Ralf Dragon dragon at tnt dot uni-hannover dot de
 *
 *	Key down sends one byte: (KEY)
 *	Key up sends  one byte:  (KEY+63(!!))
 ***********************************************************************************/
#if 0
unsigned char freedom_kbd[64] = {
/* keys without modificators */
/* 00 */ KEY_RESERVED, 	        KEY_1, 		KEY_2, 		 KEY_3, 		KEY_4, 
/* 05 */ KEY_5, 		KEY_6,  	KEY_Q, 		 KEY_W, 		KEY_E, 
/* 10 */ KEY_R, 		KEY_T, 		KEY_A, 		 KEY_S, 		KEY_D,
/* 15 */ KEY_F, 		KEY_G, 		KEY_Z, 		 KEY_X, 		KEY_C, 
/* 20 */ KEY_V, 		KEY_B, 		KEY_SPACE, 	 KEY_TAB, 	KEY_CAPSLOCK, 
/* 25 */ KEY_LEFTALT/*HOME*/,	KEY_7, 		KEY_8, 		 KEY_9, 		KEY_0, 
/* 30 */ KEY_MINUS,		KEY_EQUAL,	KEY_BACKSPACE,	 KEY_Y, 		KEY_U, 
/* 35 */ KEY_I, 		KEY_O, 		KEY_P, 		 KEY_LEFTBRACE,	KEY_RIGHTBRACE,
/* 40 */ KEY_H, 		KEY_J, 		KEY_K, 		 KEY_L, 		KEY_SEMICOLON, 
/* 45 */ KEY_APOSTROPHE,	KEY_ENTER, 	KEY_N, 		 KEY_M, 	        KEY_COMMA, 
/* 50 */ KEY_DOT, 	        KEY_SLASH, 	KEY_UP, 	 KEY_SPACE, 	KEY_ESC /*key with "circle"*/, 
/* 55 */ KEY_LEFT, 	        KEY_DOWN, 	KEY_RIGHT, 	 KEY_LEFTCTRL, 	KEY_RIGHTSHIFT,
/* 60 */ KEY_LEFTSHIFT, 	KEY_DELETE, 	KEY_INSERT/*Fn*/,KEY_RIGHTALT,
}; 	
#else
unsigned char freedom_kbd[64] = {
/* keys without modificators */
/* 00 */ KEY_RESERVED, 	        KEY_1, 		KEY_2, 		 KEY_3, 		KEY_4, 
/* 05 */ KEY_5, 		KEY_6,  	KEY_Q, 		 KEY_W, 		KEY_E, 
/* 10 */ KEY_R, 		KEY_T, 		KEY_A, 		 KEY_S, 		KEY_D,
/* 15 */ KEY_F, 		KEY_G, 		KEY_Z, 		 KEY_X, 		KEY_C, 
/* 20 */ KEY_V, 		KEY_B, 		KEY_SPACE, 	 KEY_TAB, 	        KEY_CAPSLOCK, 
/* 25 */ KEY_INSERT/*HOME*/,	KEY_7, 		KEY_8, 		 KEY_9, 		KEY_0, 
/* 30 */ KEY_MINUS,		KEY_EQUAL,	KEY_BACKSPACE,	 KEY_Y, 		KEY_U, 
/* 35 */ KEY_I, 		KEY_O, 		KEY_P, 		 KEY_LEFTBRACE,	        KEY_RIGHTBRACE,
/* 40 */ KEY_H, 		KEY_J, 		KEY_K, 		 KEY_L, 		KEY_SEMICOLON, 
/* 45 */ KEY_APOSTROPHE,	KEY_ENTER, 	KEY_N, 		 KEY_M, 	        KEY_COMMA, 
/* 50 */ KEY_DOT, 	        KEY_SLASH, 	KEY_UP, 	 KEY_SPACE, 	        KEY_ESC /*"circle"*/, 
/* 55 */ KEY_LEFT, 	        KEY_DOWN, 	KEY_RIGHT, 	 KEY_LEFTCTRL, 	        KEY_RIGHTSHIFT,
/* 60 */ KEY_LEFTSHIFT, 	KEY_RIGHTALT/*Del*/, 	KEY_LEFTALT/*Fn*/,  KEY_DELETE/*alt gr*/,
}; 	
#endif

/***********************************************************************************
 *	Belkin Wireless PDA IRDA Type keyboard
 *
 *	Alex Lange
 *	chicken@handhelds.org
 *
 *
 ***********************************************************************************/

unsigned char belkin_irda_normal[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 14, 15, 0, 0, 0, 28, 0, 0,
        42, 29, 0, 111, 58, 0, 0, 0, 0, 0, 0, 0, 0, 56, 100, 0,
        57, 57, 0, 0, 0, 106, 103, 105, 108, 0, 0, 0, 40, 12, 52, 53,
        11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 39, 51, 13, 0, 0,
        0, 30, 0, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24,
        25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 11, 19, 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  };


/***********************************************************************************
	*
	*  Benq G250 Gamepad
	*
	*  4800 baud, 8N1
	*  Not strictly a keyboard, but it's serial and works in much the same way
	*
	*  Sends two bytes on press/release, the first byte being OR 0x80 if it is a release
	*
 ***********************************************************************************/

	/* Up, Down, Left, Right, Circle, Cross, Square, Triangle, Stop, Play */
	unsigned char benq_gamepad_map[10][2] = {
		{40, KEY_UP},     /* Up */
		{96, KEY_DOWN},   /* Down */
		{94, KEY_LEFT},   /* Left */
		{47, KEY_RIGHT},  /* Right */
		{90, KEY_W},      /* Circle */
		{28, KEY_A},      /* Cross */
		{50, KEY_S},      /* Square */
		{92, KEY_D},      /* Triangle */
		{33, KEY_ESC},    /* Stop */
		{35, KEY_ENTER}   /* Play */
	};

/***********************************************************************************
 *	Targus Universal Wireless Keyboard (SIR)	
 *
 *	Niki Denev	
 *	niki@totalterror.net
 *
 ***********************************************************************************/

#if 0
unsigned char targus_irda_normal[128] = {};
#endif

/***********************************************************************************
 * Elextex cloth keyboard
 *
 * 9600 baud, 8N1
 *
 * Key down sends one byte:  KEY
 * Key up sends one byte:    KEY | 0x80
 ***********************************************************************************/

unsigned char elektex[128] = {
  /* 00 */ 0,
  /* 01 */ 0,
  /* 02 */ KEY_LEFTSHIFT, /* ambiguous between left and right */
  /* 03 */ KEY_CAPSLOCK,
  /* 04 */ 0,
  /* 05 */ 0,
  /* 06 */ 0,
  /* 07 */ 0,
  /* 08 */ KEY_BACKSPACE,
  /* 09 */ KEY_TAB,
  /* 10 */ 0,
  /* 11 */ 0,
  /* 12 */ 0,
  /* 13 */ KEY_ENTER,
  /* 14 */ 0,
  /* 15 */ 0,
  /* 16 */ 0,
  /* 17 */ 0,
  /* 18 */ 0,
  /* 19 */ 0,
  /* 20 */ KEY_UP,
  /* 21 */ KEY_DOWN,
  /* 22 */ KEY_LEFT,
  /* 23 */ KEY_RIGHT,
  /* 24 */ 0,
  /* 25 */ 0,
  /* 26 */ 0,
  /* 27 */ KEY_ESC,
  /* 28 */ 0,
  /* 29 */ 0,
  /* 30 */ 0,
  /* 31 */ 0,
  /* 32 */ KEY_SPACE,
  /* 33 */ 0,
  /* 34 */ 0,
  /* 35 */ KEY_GRAVE, /* shifted or unmapped: `, unshifted and mapped: # */
  /* 36 */ 0,
  /* 37 */ 0,
  /* 38 */ 0,
  /* 39 */ KEY_APOSTROPHE, /* unshifted or unmapped: ",
                              shifted and mapped: @ */
  /* 40 */ 0,
  /* 41 */ 0,
  /* 42 */ 0,
  /* 43 */ 0,
  /* 44 */ KEY_COMMA,
  /* 45 */ KEY_MINUS,
  /* 46 */ KEY_DOT,
  /* 47 */ KEY_SLASH,
  /* 48 */ KEY_0,
  /* 49 */ KEY_1,
  /* 50 */ KEY_2, /* unshifted or unmapped: @, shifted and mapped: " */
  /* 51 */ KEY_3, /* on keyboard: sterling, unshifted or unmapped: #,
		     shifted and mapped: ` */
  /* 52 */ KEY_4, /* keyboard also has euro */
  /* 53 */ KEY_5,
  /* 54 */ KEY_6,
  /* 55 */ KEY_7,
  /* 56 */ KEY_8,
  /* 57 */ KEY_9,
  /* 58 */ 0,
  /* 59 */ KEY_SEMICOLON,
  /* 60 */ 0,
  /* 61 */ KEY_EQUAL,
  /* 62 */ 0,
  /* 63 */ 0,
  /* 64 */ 0,
  /* 65 */ KEY_A,
  /* 66 */ KEY_B,
  /* 67 */ KEY_C,
  /* 68 */ KEY_D,
  /* 69 */ KEY_E,
  /* 70 */ KEY_F,
  /* 71 */ KEY_G,
  /* 72 */ KEY_H,
  /* 73 */ KEY_I,
  /* 74 */ KEY_J,
  /* 75 */ KEY_K,
  /* 76 */ KEY_L,
  /* 77 */ KEY_M,
  /* 78 */ KEY_N,
  /* 79 */ KEY_O,
  /* 80 */ KEY_P,
  /* 81 */ KEY_Q,
  /* 82 */ KEY_R,
  /* 83 */ KEY_S,
  /* 84 */ KEY_T,
  /* 85 */ KEY_U,
  /* 86 */ KEY_V,
  /* 87 */ KEY_W,
  /* 88 */ KEY_X,
  /* 89 */ KEY_Y,
  /* 90 */ KEY_Z,
  /* 91 */ KEY_LEFTBRACE, /* [ */
  /* 92 */ KEY_BACKSLASH,
  /* 93 */ KEY_RIGHTBRACE, /* ] */
  /* 94 */ 0,
  /* 95 */ 0,
  /* 96 */ 0,
  /* 97 */ 0,
  /* 98 */ 0,
  /* 99 */ 0,
  /*100 */ 0,
  /*101 */ 0,
  /*102 */ 0,
  /*103 */ 0,
  /*104 */ 0,
  /*105 */ 0,
  /*106 */ 0,
  /*107 */ 0,
  /*108 */ 0,
  /*109 */ 0,
  /*110 */ 0,
  /*111 */ 0,
  /*112 */ KEY_LEFTCTRL,
  /*113 */ KEY_LEFTALT,
  /*114 */ KEY_RIGHTALT,
  /*115 */ 0,
  /*116 */ 0,
  /*117 */ 0,
  /*118 */ 0,
  /*119 */ 0,
  /*120 */ 0,
  /*121 */ 0,
  /*122 */ 0,
  /*123 */ 0,
  /*124 */ 0,
  /*125 */ 0,
  /*126 */ 0,
  /*127 */ KEY_DELETE
};

	/***********************************************************************************/
#if 0
 struct microkbd_dev keyboards[] = {
	{ "HP/Compaq Micro Keyboard", "compaq",
	  h3600_compaq_process_char, 4800, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "RipTide SnapNType", "snapntype",
	  h3600_snapntype_process_char, 2400, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "TT Tech SnapNType Bluetooth", "snapntypebt",
	  h3600_snapntype_process_char, 9600, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "iConcepts Portable Keyboard", "iconcepts",
	  h3600_iconcepts_process_char, 9600, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "HP/Compaq Foldable Keyboard", "foldable",
	  h3600_foldable_process_char,  4800, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "Micro Innovations IR Keyboard", "microinnovations",
	  h3600_microinnovations_process_char, 9600, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_2 },
	{ "Eagle-Touch Portable PDA Keyboard", "eagletouch",  
	  h3600_eagletouch_process_char,  9600, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "GrandTec PocketVIK", "pocketvik",  
	  h3600_pocketvik_process_char, 57600, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "Flexis FX100", "flexis",  
	  h3600_flexis_process_char, 9600, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "HP Slim Keyboard", "hpslim",
	  h3600_hpslim_process_char, 4800, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
	{ "Belkin Wireless PDA Keyboard", "belkin-irda",
	h3600_belkin_irda_process_char, 9600, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_2 },
	{ "HP iPAQ Bluetooth Foldable Keyboard", "btfoldable",
	  h3600_btfoldable_process_char,  4800, UTCR0_8BitData | UTCR0_1StpBit, &g_uart_3 },
};

#define NUM_KEYBOARDS ARRAY_SIZE(keyboards)
#endif
