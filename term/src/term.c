#include <stdio.h>
#include <stdbool.h>
#include <mos_api.h>
#include <agon/vdp_vdu.h>
#include <agon/vdp_key.h>
#include "ringb.h"

#define KBDINTERRUPT

#ifdef KBDINTERRUPT
// from <agon/vdp_key.h>
// int vdp_key_init( void );
// void vdp_key_reset_interrupt( void );
// void vdp_update_key_state();
// bool vdp_check_key_press( uint8_t key_code );
// void vdp_set_key_event_handler( KEY_EVENT_HANDLER event_handler );

static char key = -1;
static char vkey = -1;
static int iters = 0;
volatile KEY_EVENT prev_key_event = { 0 };

static RingBuffer rbuf;

void kkehandler( KEY_EVENT key_event )
{
    if (prev_key_event.key_data == key_event.key_data ) return;
    prev_key_event.key_data = key_event.key_data;
	if (key_event.down != 0) {
	  if (key_event.ascii != 0) {
	    key = key_event.ascii;
		vkey = key_event.code;
		append_char(&rbuf, key);
		printf("key %02x vkey %02x %d\n", key, vkey & 0xFF, rbuf.count);
	  }
	} else {
		key = -1;
		vkey = -1;
	};
	return;
};

int main() {
    printf("term: use Ctrl-Q to exit.\n");
    init_buffer(&rbuf);
    vdp_key_init();
    vdp_set_key_event_handler( kkehandler );
    waitvblank();
    waitvblank();

    //vdp_terminal_mode();
    char ch;
    while (true) {
        if (has_next_char(&rbuf)) {
            ch = get_next_char(&rbuf);
            printf("rbuf %02x %d\n", key, rbuf.count);
        }
        if (ch == '\x11') break;
  		vdp_update_key_state();
    }
    //vdp_terminal_mode();
}
#endif

//#define D_INCHAR
#ifdef D_INCHAR
int main() {
    vdp_terminal_mode();
    while (true) {
        char ch = inchar();
        printf(" [%02x]\n", ch);
        if (ch == '\x11') break;
    }
    vdp_terminal_mode();
}
#endif
