# AgonProjects
a couple of hacks for Agon Light 2

Both are tied to [`agondev` C compiler](https://github.com/AgonPlatform/agondev).
Don't think they work if compiled with [AgDev](https://github.com/pcawte/AgDev).

_Many many many repos have informed me and this work, the Agon community is terrific._

## kl3 - Kris' Lisp 3

A simple symbolic lisp with integers.
It **does** actually work to run a program inside of a lisp meta-interpreter running on `kl3`.
See [tests.lisp](kl3/tests.lisp) - that last block of lisp code.

### Issues

- should do a simple 16-bit fixed point data type

## za - Very Small emacs

based on hugh barney's Zep, hacked to work on Agon.

Most editors need to handle _everything_ from the keyboard.
On UNIX this means 1960's hard core `RAW` mode on the various
terminal structs attached to the `tty || pty` devices.
Emacs needs to handle tings like Ctrl-C and -Z and ESC etc.
And they rely on input routines to not ECHO anything (like the char) to stdout before it handles it.

Primary difficulties are/was that there is no _RAW_ mode on Agon for the console (uart0).
There is some `cooked` aspects in MOS, but no way to just turn it off. (_I think_)
Best you can do is reach deep into the C stdlib of agondev and find `inchar()` (see `agondev/src/lib/crt/inchar.src`).
The `rst.lil	008h` there is a MOS call to get a key from keyboard and blocks, without echoing to the VDU/VDP. (_I think_)
Anyway, it works(-ish).

Speed is an issue, the Agon is bit slow for this type of work, where I have done no real optimization of the code
for a "smart output device" like the Agon's VDP.
Currently, it blanks the screen and redraws on each key stroke. (Not ideal, obviously)

### Issues

- need to change `display()` to only re-draw after a screen change. Too much flashing from cursor movement.
Can probably just use Agon VDP cursor movement for everything that doesn't cause a buffer to window shift.
- uses only `inchar()`, but NOT the FabGL key stuff.
- without handling FabGL keys, am unable to handle things on KBD like Home, PageUp, etc.
- a keyboard interrupt version, using ideas like `agon/vdp_key.h` might be much better
- probaby need to unify all the various pointer types to be the ez80 24-bit ones.

#### Huh? Keyboard?

To get the keyboard `get_key()` stuff to use this instead of an un-buffered inchar() (see ).
CANNOT use things like `fgetc()` as they tend to ECHO the char to stdout.
[<agon/vdp_key.h>](https://github.com/AgonPlatform/agondev/release/include/agon/vdp_key.h)
```C
#ifndef _VDP_KEY_H
#define _VDP_KEY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
	uint32_t key_data;
	struct {
		uint8_t ascii;
		uint8_t mods;
		uint8_t code;
		uint8_t down;
	};
} KEY_EVENT;


extern uint8_t vdp_key_bits[32];

typedef void (*KEY_EVENT_HANDLER)(KEY_EVENT key_event);

int vdp_key_init( void );
void vdp_key_reset_interrupt( void );

void vdp_update_key_state();
bool vdp_check_key_press( uint8_t key_code );
void vdp_set_key_event_handler( KEY_EVENT_HANDLER event_handler );

#ifdef __cplusplus
}
#endif

#endif
```
