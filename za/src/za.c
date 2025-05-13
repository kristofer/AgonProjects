/* za.c,  Agon Emacs, Public Domain, github.com/kristofer 2025, Hugh Barney, 2017, Derived from: Anthony's Editor January 93 */
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <agon/vdp_vdu.h>
#include <agon/vdp_key.h>
#include <mos_api.h>

#define E_NAME          "za"
#define E_VERSION       "v1.1"
#define E_LABEL         "Za:"
#define MSGLINE         (LINES-1)
#define CHUNK           8096L
#define K_BUFFER_LENGTH 256
#define MAX_FNAME       256
#define TEMPBUF         128
#define MIN_GAP_EXPAND  512
#define NOMARK          -1
#define STRBUF_M        64

typedef unsigned char char_t;
typedef long point_t;

typedef struct keymap_t {
	char *key_desc;                 /* name of bound function */
	char *key_bytes;		/* the string of bytes when this key is pressed */
	void (*func)(void);
} keymap_t;

typedef struct buffer_t
{
	point_t b_mark;	     	  /* the mark */
	point_t b_point;          /* the point */
	point_t b_page;           /* start of page */
	point_t b_epage;          /* end of page */
	char_t *b_buf;            /* start of buffer */
	char_t *b_ebuf;           /* end of buffer */
	char_t *b_gap;            /* start of gap */
	char_t *b_egap;           /* end of gap */
	char w_top;	          /* Origin 0 top row of window */
	char w_rows;              /* no. of rows of text in window */
	int b_row;                /* cursor row */
	int b_col;                /* cursor col */
	char b_fname[MAX_FNAME + 1]; /* filename */
	char b_modified;          /* was modified */
} buffer_t;

/*
 * Some compilers define size_t as a unsigned 16 bit number while point_t and
 * off_t might be defined as a signed 32 bit number. malloc(), realloc(),
 * fread(), and fwrite() take size_t parameters, which means there will be some
 * size limits because size_t is too small of a type.
 */
#define MAX_SIZE_T      ((unsigned long) (size_t) ~0)

int done;
char_t *input;
int msgflag;
char msgline[TEMPBUF];
keymap_t *key_return;
keymap_t *key_map;
buffer_t *curbp;
point_t nscrap = 0;
char_t *scrap = NULL;
char searchtext[STRBUF_M];
uint8_t sc, sr;

#define COLS 80
#define LINES 59

#define CLRBLACK 0
#define CLRIBLACK 8
#define CLRWHITE 7
#define CLRIWHITE 15

// service routines needed
void initscr(void);
void clrtoeol(void);
void flushinp(void);
void refresh(void);
// buffer routines
void addstr(const char *str);
void addch(const char c);
const char *unctrl(char_t p);
void move(int c, int r);
void standout(void);
void standend(void);

void clrscr(void);
void pause(char *str);
void cursor(char onoff);

// init the screen
void initscr(void) {
    clrscr();
}
void flushinp(void) {}
void refresh(void) {
    cursor(1);
}

// output str
void addstr(const char *str) {
    printf("%s",str);
}
void addch(const char c) {
    putchar(c);
}
// change a control-char into a string ctrl-c -> ^C
const char *unctrl(char_t p) {}
// move cursor (scr) to c,r x,y
void move(int c, int r) {
    sc=c; sr=r;
    vdp_cursor_tab(c,r);
}
// make things standout
void standout(void) {}
void standend(void) {}


void delay(int tenthseconds) {
    //sleep(seconds);
    for (int i = 0; i < tenthseconds; i++) {
        for (int j = 0; j < 5; j++) waitvblank(); // 20ms(?)
    }
}
void pause(char *str) {
    printf("%s", str); delay(10);
}

void cursor(char onoff) {
    // Turning off flashing cursor (1 on, 0 off)
    //putch(23); putch(1); putch(onoff);
}
// Clear screen
void clrscr() {
    //vdp_mode(3); - Doesn't work, send the characters instead!
    char mode[2] = {22,0};
    mos_puts(mode,2,0);
    vdp_clear_screen();
    //cursor(0);
}
// Get character from keyboard
char cgetc() {
    return inchar();
}
// Position cursor
void gotoxy(uint8_t *x, uint8_t *y) {
    vdp_cursor_tab(x,y);
}
// get Position cursor
void getxycursor(uint8_t *x, uint8_t *y) {
    vdp_return_text_cursor_position(x,y);
}


// Put a character at screen coord
void cputcxy(uint8_t *x, uint8_t *y, char c) {
    sc = *x; sr = *y;
    vdp_cursor_tab(x,y);
    putch(c); putch(8);
}

void clrtoeol(void) {
    int cc = sc;
    int cr = sr;
    for (int i = cc; i < 80; i++) cputcxy((uint8_t *)i, (uint8_t *)cr, ' ');
}

void set_colours(int fg, int bg) {
    vdp_set_text_colour(fg);
    vdp_set_text_colour(bg+128);
}


// main zep routines.

buffer_t* new_buffer()
{
	buffer_t *bp = (buffer_t *)malloc(sizeof(buffer_t));
	assert(bp != NULL);
	bp->b_point = 0;
	bp->b_mark = NOMARK;
	bp->b_page = 0;
	bp->b_epage = 0;
	bp->b_modified = 0;
	bp->b_buf = NULL;
	bp->b_ebuf = NULL;
	bp->b_gap = NULL;
	bp->b_egap = NULL;
	bp->b_fname[0] = '\0';
	bp->w_top = 0;
	bp->w_rows = LINES - 2;
	return bp;
}

void fatal(char *msg)
{
	// noraw();
	// endwin();
	printf("\n" E_NAME " " E_VERSION ": %s\n", msg);
	exit(1);
}

int msg(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	(void)vsprintf(msgline, msg, args);
	va_end(args);
	msgflag = true;
	return false;
}

/* Given a buffer offset, convert it to a pointer into the buffer */
char_t * ptr(buffer_t *bp, register point_t offset)
{
	if (offset < 0) return (bp->b_buf);
	return (bp->b_buf+offset + (bp->b_buf + offset < bp->b_gap ? 0 : bp->b_egap-bp->b_gap));
}

/* Given a pointer into the buffer, convert it to a buffer offset */
point_t pos(buffer_t *bp, register char_t *cp)
{
	assert(bp->b_buf <= cp && cp <= bp->b_ebuf);
	return (cp - bp->b_buf - (cp < bp->b_egap ? 0 : bp->b_egap - bp->b_gap));
}

/* Enlarge gap by n chars, position of gap cannot change */
int growgap(buffer_t *bp, point_t n)
{
	char_t *new;
	point_t buflen, newlen, xgap, xegap;
	assert(bp->b_buf <= bp->b_gap);
	assert(bp->b_gap <= bp->b_egap);
	assert(bp->b_egap <= bp->b_ebuf);
	xgap = bp->b_gap - bp->b_buf;
	xegap = bp->b_egap - bp->b_buf;
	buflen = bp->b_ebuf - bp->b_buf;

	/* reduce number of reallocs by growing by a minimum amount */
	n = (n < MIN_GAP_EXPAND ? MIN_GAP_EXPAND : n);
	newlen = buflen + n * sizeof (char_t);

	if (buflen == 0) {
		if (newlen < 0 || MAX_SIZE_T < newlen) fatal("Failed to allocate required memory");
		new = (char_t*) malloc((size_t) newlen);
		if (new == NULL) fatal("Failed to allocate required memory");
	} else {
		if (newlen < 0 || MAX_SIZE_T < newlen) return msg("Failed to allocate required memory");
		new = (char_t*) realloc(bp->b_buf, (size_t) newlen);
		if (new == NULL) return msg("Failed to allocate required memory");
	}

	/* Relocate pointers in new buffer and append the new extension to the end of the gap */
	bp->b_buf = new;
	bp->b_gap = bp->b_buf + xgap;
	bp->b_ebuf = bp->b_buf + buflen;
	bp->b_egap = bp->b_buf + newlen;
	while (xegap < buflen--)
		*--bp->b_egap = *--bp->b_ebuf;
	bp->b_ebuf = bp->b_buf + newlen;

	assert(bp->b_buf < bp->b_ebuf);          /* Buffer must exist. */
	assert(bp->b_buf <= bp->b_gap);
	assert(bp->b_gap < bp->b_egap);          /* Gap must grow only. */
	assert(bp->b_egap <= bp->b_ebuf);
	return (true);
}

point_t movegap(buffer_t *bp, point_t offset)
{
	char_t *p = ptr(bp, offset);
	while (p < bp->b_gap)
		*--bp->b_egap = *--bp->b_gap;
	while (bp->b_egap < p)
		*bp->b_gap++ = *bp->b_egap++;
	assert(bp->b_gap <= bp->b_egap);
	assert(bp->b_buf <= bp->b_gap);
	assert(bp->b_egap <= bp->b_ebuf);
	return (pos(bp, bp->b_egap));
}

void save()
{
	FILE *fp;
	point_t length;
	fp = fopen(curbp->b_fname, "w");
	if (fp == NULL) msg("Failed to open file \"%s\".", curbp->b_fname);
	(void) movegap(curbp, (point_t) 0);
	length = (point_t) (curbp->b_ebuf - curbp->b_egap);
	if (fwrite(curbp->b_egap, sizeof (char), (size_t) length, fp) != length)
		msg("Failed to write file \"%s\".", curbp->b_fname);
	fclose(fp);
	curbp->b_modified = 0;
	msg("File \"%s\" %ld bytes saved.", curbp->b_fname, pos(curbp, curbp->b_ebuf));
}

/* reads file into buffer at point */
/**
 * Custom getline implementation using standard C library.
 *
 * @param lineptr Pointer to the buffer where the line will be stored.
 *               If *lineptr is NULL, a buffer will be allocated.
 *               If *lineptr is not NULL, the buffer will be reused.
 * @param n Pointer to the size of the buffer. Will be updated if buffer is reallocated.
 * @param stream File stream to read from (typically stdin)
 * @return Number of characters read including the newline, or -1 on error/EOF
 */
int k_getline(char **lineptr, int *n, FILE *stream) {
    char *buffer = *lineptr;
    size_t size = *n;
    int c;
    size_t pos = 0;

    // Check if we need to allocate a buffer
    if (buffer == NULL || size == 0) {
        size = 128; // Start with a reasonable buffer size
        buffer = (char *)malloc(size);
        if (!buffer) return -1;
    }

    while ((c = fgetc(stream)) != EOF) {
        // Ensure space for character and null terminator
        if (pos + 1 >= size) {
            size_t new_size = size * 2;
            char *new_buffer = (char *)realloc(buffer, new_size);
            if (!new_buffer) {
                return -1;
            }
            buffer = new_buffer;
            size = new_size;
        }

        // Store the character
        buffer[pos++] = c;

        // If we hit a newline, we're done
        if (c == '\n') break;
    }

    // Handle EOF or error cases
    if (c == EOF && pos == 0) {
        return -1;
    }

    // Null terminate the string
    buffer[pos] = '\0';

    // Update caller's pointers
    *lineptr = buffer;
    *n = size;

    return pos;
}

int insert_file(char *fn)
{
    FIL fp;             // FatFS file object
    FILINFO fno;        // FatFS file information
    FRESULT fr;         // FatFS result code
    uint24_t bytes_read;    // Bytes actually read

    // Get file information instead of using stat()
    fr = ffs_stat(&fno, fn);
    if (fr != FR_OK)
        return msg("Failed to find file \"%s\". Error: %d", fn, fr);

    // Check if file is too big
    if (MAX_SIZE_T < fno.fsize)
        return msg("File \"%s\" is too big to load.", fn);

    // Ensure gap is large enough
    if (curbp->b_egap - curbp->b_gap < fno.fsize * sizeof(char_t) &&
        !growgap(curbp, fno.fsize))
        return (false);

    // Open file using FatFS
    fr = ffs_fopen(&fp, fn, FA_READ);
    if (fr != FR_OK)
        return msg("Failed to open file \"%s\". Error: %d", fn, fr);

    // Position gap at current point
    curbp->b_point = movegap(curbp, curbp->b_point);

    // Read file content into gap
    bytes_read = ffs_fread(&fp, curbp->b_gap, fno.fsize);
    if (fr != FR_OK) {
        ffs_fclose(&fp);
        return msg("Failed to read file \"%s\". Error: %d", fn, fr);
    }

    // Update gap pointer
    curbp->b_gap += bytes_read;

    // Close file
    fr = ffs_fclose(&fp);
    if (fr != FR_OK)
        return msg("Failed to close file \"%s\". Error: %d", fn, fr);

    // Report success
    msg("File \"%s\" %lu bytes read.", fn, (unsigned long)bytes_read);
    return (true);
}


char_t *get_key(keymap_t *keys, keymap_t **key_return)
{
	keymap_t *k;
	int submatch;
	static char_t buffer[K_BUFFER_LENGTH];
	static char_t *record = buffer;
	*key_return = NULL;

	/* if recorded bytes remain, return next recorded byte. */
	if (*record != '\0') {
		*key_return = NULL;
		return record++;
	}

	record = buffer; /* reset record buffer. */
	do {
		assert(K_BUFFER_LENGTH > record - buffer);
		//*record++ = (unsigned)getch(); /* read and record one byte. */
		*record++ = (unsigned)cgetc(); /* read and record one byte. */
		*record = '\0';

		/* if recorded bytes match any multi-byte sequence... */
		for (k = keys, submatch = 0; k->key_bytes != NULL; ++k) {
			char_t *p, *q;
			for (p = buffer, q = (char_t *)k->key_bytes; *p == *q; ++p, ++q) {
			        /* an exact match */
				if (*q == '\0' && *p == '\0') {
	    				record = buffer;
					*record = '\0';
					*key_return = k;
					return record; /* empty string */
				}
			}
			/* record bytes match part of a command sequence */
			if (*p == '\0' && *q != '\0') submatch = 1;
		}
	} while (submatch);
	/* nothing matched, return recorded bytes. */
	record = buffer;
	return (record++);
}

/* Reverse scan for start of logical line containing offset */
point_t lnstart(buffer_t *bp, register point_t off)
{
	register char_t *p;
	do
		p = ptr(bp, --off);
	while (bp->b_buf < p && *p != '\n');
	return (bp->b_buf < p ? ++off : 0);
}

/* Forward scan for start of logical line segment containing 'finish' */
point_t segstart(buffer_t *bp, point_t start, point_t finish)
{
	char_t *p;
	int c = 0;
	point_t scan = start;

	while (scan < finish) {
		p = ptr(bp, scan);
		if (*p == '\n') {
			c = 0;
			start = scan + 1;
		} else if (COLS <= c) {
			c = 0;
			start = scan;
		}
		++scan;
		c += *p == '\t' ? 8 - (c & 7) : 1;
	}
	return (c < COLS ? start : finish);
}

/* Forward scan for start of logical line segment following 'finish' */
point_t segnext(buffer_t *bp, point_t start, point_t finish)
{
	char_t *p;
	int c = 0;

	point_t scan = segstart(bp, start, finish);
	for (;;) {
		p = ptr(bp, scan);
		if (bp->b_ebuf <= p || COLS <= c) break;
		++scan;
		if (*p == '\n') break;
		c += *p == '\t' ? 8 - (c & 7) : 1;
	}
	return (p < bp->b_ebuf ? scan : pos(bp, bp->b_ebuf));
}

/* Move up one screen line */
point_t upup(buffer_t *bp, point_t off)
{
	point_t curr = lnstart(bp, off);
	point_t seg = segstart(bp, curr, off);
	if (curr < seg)
		off = segstart(bp, curr, seg-1);
	else
		off = segstart(bp, lnstart(bp,curr-1), curr-1);
	return (off);
}

/* Move down one screen line */
point_t dndn(buffer_t *bp, point_t off) { return (segnext(bp, lnstart(bp,off), off)); }

/* Return the offset of a column on the specified line */
point_t lncolumn(buffer_t *bp, point_t offset, int column)
{
	int c = 0;
	char_t *p;
	while ((p = ptr(bp, offset)) < bp->b_ebuf && *p != '\n' && c < column) {
		c += *p == '\t' ? 8 - (c & 7) : 1;
		++offset;
	}
	return (offset);
}

void modeline(buffer_t *bp)
{
	char temp[TEMPBUF];
	char mch;

	standout();//
	move(0, bp->w_top + bp->w_rows);
	mch = bp->b_modified ? '*' : '=';
	sprintf(temp, "=%c " E_LABEL " == %s ", mch, bp->b_fname);
	addstr(temp);
	for (int i = strlen(temp) + 1; i <= COLS-2; i++)
		addch('=');
	standend();//
}

void dispmsg()
{
	move(0, MSGLINE);
	if (msgflag) {
		addstr(msgline);
		msgflag = false;
	}
	for (int i = strlen(msgline); i <= COLS-2; i++)
		addch(' ');}

void display()
{
	char_t *p;
	int i, j, k;
	buffer_t *bp = curbp;

	/* find start of screen, handle scroll up off page or top of file  */
	/* point is always within b_page and b_epage */
	if (bp->b_point < bp->b_page)
		bp->b_page = segstart(bp, lnstart(bp,bp->b_point), bp->b_point);

	/* reframe when scrolled off bottom */
	if (bp->b_epage <= bp->b_point) {
		/* Find end of screen plus one. */
		bp->b_page = dndn(bp, bp->b_point);
		/* if we scoll to EOF we show 1 blank line at bottom of screen */
		if (pos(bp, bp->b_ebuf) <= bp->b_page) {
			bp->b_page = pos(bp, bp->b_ebuf);
			i = bp->w_rows - 1;
		} else {
			i = bp->w_rows - 0;
		}
		/* Scan backwards the required number of lines. */
		while (0 < i--)
			bp->b_page = upup(bp, bp->b_page);
	}

	move(0, bp->w_top); /* start from top of window */
	i = bp->w_top;
	j = 0;
	bp->b_epage = bp->b_page;

	/* paint screen from top of page until we hit maxline */
	cursor(0);
	vdp_clear_screen();

	while (1) {
		/* reached point - store the cursor position */
		if (bp->b_point == bp->b_epage) {
			bp->b_row = i;
			bp->b_col = j;
		}
		p = ptr(bp, bp->b_epage);
		if (bp->w_top + bp->w_rows <= i || bp->b_ebuf <= p) /* maxline */
			break;
		if (*p != '\r') {
			if (isprint(*p) || *p == '\t' || *p == '\n') {
				j += *p == '\t' ? 8-(j&7) : 1;
				addch(*p);
			} else {
				const char *ctrl = unctrl(*p);
				j += (int) strlen(ctrl);
				addstr(ctrl);
			}
		}
		if (*p == '\n' || COLS <= j) {
			j -= COLS;
			if (j < 0) j = 0;
			++i;
		}
		++bp->b_epage;
	}

	/* replacement for clrtobot() to bottom of window */
	// for (k=i; k < bp->w_top + bp->w_rows; k++) {
	// 	move(j,k); /* clear from very last char not start of line */
	// 	clrtoeol();
	// 	j = 0; /* thereafter start of line */
	// }

	modeline(bp);
	dispmsg();
	cursor(1);
	move(bp->b_col, bp->b_row); /* set cursor */
	refresh();
}

void top() { curbp->b_point = 0; }
void bottom() {	curbp->b_epage = curbp->b_point = pos(curbp, curbp->b_ebuf); }
void left() { if (0 < curbp->b_point) --curbp->b_point; }
void right() { if (curbp->b_point < pos(curbp, curbp->b_ebuf)) ++curbp->b_point; }
void up() { curbp->b_point = lncolumn(curbp, upup(curbp, curbp->b_point),curbp->b_col); }
void down() { curbp->b_point = lncolumn(curbp, dndn(curbp, curbp->b_point),curbp->b_col); }
void lnbegin() { curbp->b_point = segstart(curbp, lnstart(curbp,curbp->b_point), curbp->b_point); }
void quit() { done = 1; }

void lnend()
{
	curbp->b_point = dndn(curbp, curbp->b_point);
	left();
}

void pgdown()
{
	curbp->b_page = curbp->b_point = upup(curbp, curbp->b_epage);
	while (0 < curbp->b_row--)
		down();
	curbp->b_epage = pos(curbp, curbp->b_ebuf);
}

void pgup()
{
	int i = curbp->w_rows;
	while (0 < --i) {
		curbp->b_page = upup(curbp, curbp->b_page);
		up();
	}
}

void insert()
{
	assert(curbp->b_gap <= curbp->b_egap);
	if (curbp->b_gap == curbp->b_egap && !growgap(curbp, CHUNK)) return;
	curbp->b_point = movegap(curbp, curbp->b_point);
	*curbp->b_gap++ = *input == '\r' ? '\n' : *input;
	curbp->b_point = pos(curbp, curbp->b_egap);
	curbp->b_modified = 1;
}

void backsp()
{
	curbp->b_point = movegap(curbp, curbp->b_point);
	if (curbp->b_buf < curbp->b_gap) {
		--curbp->b_gap;
		curbp->b_modified = 1;
	}
	curbp->b_point = pos(curbp, curbp->b_egap);
}

void delete()
{
	curbp->b_point = movegap(curbp, curbp->b_point);
	if (curbp->b_egap < curbp->b_ebuf) {
		curbp->b_point = pos(curbp, ++curbp->b_egap);
		curbp->b_modified = 1;
	}
}

void set_mark()
{
	curbp->b_mark = (curbp->b_mark == curbp->b_point ? NOMARK : curbp->b_point);
	msg("Mark set");
}

void copy_cut(int cut)
{
	char_t *p;
	/* if no mark or point == marker, nothing doing */
	if (curbp->b_mark == NOMARK || curbp->b_point == curbp->b_mark) return;
	if (scrap != NULL) {
		free(scrap);
		scrap = NULL;
	}
	if (curbp->b_point < curbp->b_mark) {
		/* point above marker: move gap under point, region = marker - point */
		(void)movegap(curbp, curbp->b_point);
		p = ptr(curbp, curbp->b_point);
		nscrap = curbp->b_mark - curbp->b_point;
	} else {
		/* if point below marker: move gap under marker, region = point - marker */
		(void)movegap(curbp, curbp->b_mark);
		p = ptr(curbp, curbp->b_mark);
		nscrap = curbp->b_point - curbp->b_mark;
	}
	if ((scrap = (char_t*) malloc(nscrap)) == NULL) {
		msg("No more memory available.");
	} else {
		(void)memcpy(scrap, p, nscrap * sizeof (char_t));
		if (cut) {
			curbp->b_egap += nscrap; /* if cut expand gap down */
			curbp->b_point = pos(curbp, curbp->b_egap); /* set point to after region */
			curbp->b_modified = 1;
			msg("%ld bytes cut.", nscrap);
		} else {
			msg("%ld bytes copied.", nscrap);
		}
		curbp->b_mark = NOMARK;  /* unmark */
	}
}

void paste()
{
	if (nscrap <= 0) {
		msg("Nothing to paste.");
	} else if (nscrap < curbp->b_egap - curbp->b_gap || growgap(curbp, nscrap)) {
		curbp->b_point = movegap(curbp, curbp->b_point);
		memcpy(curbp->b_gap, scrap, nscrap * sizeof (char_t));
		curbp->b_gap += nscrap;
		curbp->b_point = pos(curbp, curbp->b_egap);
		curbp->b_modified = 1;
	}
}

void copy() { copy_cut(false); }
void cut() { copy_cut(true); }

void killtoeol()
{
	/* point = start of empty line or last char in file */
	if (*(ptr(curbp, curbp->b_point)) == 0xa || (curbp->b_point + 1 == ((curbp->b_ebuf - curbp->b_buf) - (curbp->b_egap - curbp->b_gap))) ) {
		delete();
	} else {
		curbp->b_mark = curbp->b_point;
		lnend();
		copy_cut(true);
	}
}

point_t search_forward(buffer_t *bp, point_t start_p, char *stext)
{
	point_t end_p = pos(bp, bp->b_ebuf);
	point_t p,pp;
	char* s;

	if (0 == strlen(stext)) return start_p;

	for (p=start_p; p < end_p; p++) {
		for (s=stext, pp=p; *s == *(ptr(bp, pp)) && *s !='\0' && pp < end_p; s++, pp++)
			;
		if (*s == '\0') return pp;
	}
	return -1;
}

void search()
{
	int cpos = 0;
	int c;
	point_t o_point = curbp->b_point;
	point_t found;
	searchtext[0] = '\0';
	msg("Search: %s", searchtext);
	dispmsg();
	cpos = strlen(searchtext);

	for (;;) {
		refresh();
		c = cgetc();
		/* ignore control keys other than C-g, backspace, CR,  C-s, C-R, ESC */
		if (c < 32 && c != 07 && c != 0x08 && c != 0x13 && c != 0x12 && c != 0x1b)
			continue;

		switch(c) {
		case 0x1b: /* esc */
			searchtext[cpos] = '\0';
			flushinp(); /* discard any escape sequence without writing in buffer */
			return;
		case 0x07: /* ctrl-g */
			curbp->b_point = o_point;
			return;
		case 0x13: /* ctrl-s, do the search */
			found = search_forward(curbp, curbp->b_point, searchtext);
			if (found != -1 ) {
				curbp->b_point = found;
				msg("Search: %s", searchtext);
				display();
			} else {
				msg("Failing Search: %s", searchtext);
				dispmsg();
				curbp->b_point = 0;
			}
			break;
		case 0x7f: /* del, erase */
		case 0x08: /* backspace */
			if (cpos == 0)
				continue;
			searchtext[--cpos] = '\0';
			msg("Search: %s", searchtext);
			dispmsg();
			break;
		default:
			if (cpos < STRBUF_M - 1) {
				searchtext[cpos++] = c;
				searchtext[cpos] = '\0';
				msg("Search: %s", searchtext);
				dispmsg();
			}
			break;
		}
	}
}

/* the key bindings:  desc, keys, func */
keymap_t keymap[] = {
    {"left arrow               ", "\x08", left },
    {"right arrow              ", "\x15", right },
    {"up arrow                 ", "\x0B", up },
    {"down arrow               ", "\x0A", down },
	{"C-a beginning-of-line    ", "\x01", lnbegin },
	{"C-b                      ", "\x02", left },
	{"C-d forward-delete-char  ", "\x04", delete },
	{"C-e end-of-line          ", "\x05", lnend },
	{"C-f                      ", "\x06", right },
	{"C-n                      ", "\x0E", down },
	{"C-p                      ", "\x10", up },
	{"C-h backspace            ", "\x08", backsp },
	{"C-k kill-to-eol          ", "\x0B", killtoeol },
	{"C-l redraw               ", "\x0C", display },
	{"C-s search               ", "\x13", search },
	{"C-v                      ", "\x16", pgdown },
	{"C-w kill-region          ", "\x17", cut},
	{"C-y yank                 ", "\x19", paste},
	{"C-space set-mark         ", "\x00", set_mark },
	{"esc @ set-mark           ", "\x1B\x40", set_mark },
	{"esc k kill-region        ", "\x1B\x6B", cut },
	{"esc v                    ", "\x1B\x76", pgup },
	{"esc w copy-region        ", "\x1B\x77", copy},
	{"esc < beg-of-buf         ", "\x1B\x3C", top },
	{"esc > end-of-buf         ", "\x1B\x3E", bottom },
	{"up previous-line         ", "\x1B\x5B\x41", up },
	{"down next-line           ", "\x1B\x5B\x42", down },
	{"left backward-character  ", "\x1B\x5B\x44", left },
	{"right forward-character  ", "\x1B\x5B\x43", right },
	{"home beginning-of-line   ", "\x1B\x4F\x48", lnbegin },
	{"end end-of-line          ", "\x1B\x4F\x46", lnend },
	{"DEL forward-delete-char  ", "\x1B\x5B\x33\x7E", delete },
	{"backspace delete-left    ", "\x7f", backsp },
	{"PgUp                     ", "\x1B\x5B\x35\x7E",pgup },
	{"PgDn                     ", "\x1B\x5B\x36\x7E", pgdown },
	{"C-x C-s save-buffer      ", "\x18\x13", save },
	{"C-x C-c exit             ", "\x18\x03", quit },
	{"K_ERROR                  ", NULL, NULL }
};

void cleanup() {
    vdp_clear_screen();
    set_colours(CLRIWHITE, CLRBLACK);
    cursor(1);
    exit(0);
}

int main(int argc, char **argv)
{
	if (argc != 2) fatal("usage: " E_NAME " filename\n");
	initscr();
	curbp = new_buffer();

	(void)insert_file(argv[1]);
	strncpy(curbp->b_fname, argv[1], MAX_FNAME);  /* save filename regardless */
	curbp->b_fname[MAX_FNAME] = '\0'; /* force truncation */
	if (!growgap(curbp, CHUNK)) fatal("Failed to allocate required memory.\n");

	key_map = keymap;

	while (!done) {
		display();
		input = get_key(key_map, &key_return);
		if (key_return != NULL) {
			(key_return->func)();
		} else {
			if (*input > 31 || *input == 10 || *input == 13 || *input == 9) /* allow TAB, NEWLINE and other control char is Not Bound */
				insert();
            else
				msg("Not bound");
		}
	}

	if (scrap != NULL) free(scrap);
	if (curbp != NULL) free(curbp);
	cleanup();
	return 0;
}
