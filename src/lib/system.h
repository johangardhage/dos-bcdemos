//
// Retro programming in Borland C++ 3.1
//
#ifndef	__SYSTEM_H__
#define	__SYSTEM_H__

#pragma	inline

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>

typedef	unsigned int	boolean;
typedef	unsigned char	byte;
typedef	unsigned int	word;
typedef	unsigned long	dword;
typedef	float		real;
enum			boolean_values { false = 0, true = 1 };

typedef struct {
	byte	r, g, b;
}	colortype;

void	setgfxmode ( word mode ) {
	// Lets TASM accept 486 instructions from here to the end of the file, so
	// system.h must be the first include.  It sits in a function because
	// BC++ 3.1 counts a file-scope asm statement as initialized data, which
	// shifts the offsets it uses for local initializers (d@w+N).
	asm	.486p
	asm	mov	ax, [mode]
	asm	int	0x10
}

void	fadepalette ( colortype *pal, byte fade ) {
	int color;

	asm	mov	dx, 0x3c8
	asm	xor	al, al
	asm	out	dx, al
	for ( int i = 0; i < 256; i ++ ) {
		color = pal[i].r * fade / 64;
		asm	mov	al, byte ptr [color]
		asm	mov	dx, 0x3c9
		asm	out	dx, al
		color = pal[i].g * fade / 64;
		asm	mov	al, byte ptr [color]
		asm	mov	dx, 0x3c9
		asm	out	dx, al
		color = pal[i].b * fade / 64;
		asm	mov	al, byte ptr [color]
		asm	mov	dx, 0x3c9
		asm	out	dx, al
	}
}

void	setpalette ( colortype *pal ) {
	asm	mov	dx, 0x3c8
	asm	xor	al, al
	asm	out	dx, al
	asm	mov	dx, 0x3c9
	asm	mov	cx, 768
	asm	mov	si, [pal]
	asm	rep	outsb
}

void	vretrace ( void ) {
	asm	mov	dx, 0x3da
	retrace_end:
	asm	in	al, dx
	asm	test	al, 0x8
	asm	jnz	retrace_end
	retrace_start:
	asm	in	al, dx
	asm	test	al, 0x8
	asm	jz	retrace_start
}

void	copyscreen ( word source, word dest ) {
	asm	push	ds
	asm	mov	ds, [source]
	asm	mov	es, [dest]
	asm	xor	di, di
	asm	xor	si, si
	asm	mov	cx, 64000/4
	asm	rep	movsd
	asm	pop	ds
}

void	motionblur (word alpha, word backdrop, word vscreen, byte level) {
	asm	mov	di, 63996	// last 4 pixels of the screen
	asm	mov	es, [alpha]
	asm	mov	fs, [backdrop]
	asm	mov	ax, 0xa000
	asm	mov	gs, ax
	asm	mov	dl, [level]
	asm	push	ds
	asm	mov	ds, [vscreen]
	asm	xor	cx, cx

	composite:
	// where neither the object nor its trail is, 4 pixels at a time: the
	// trail stays 0 and the screen is the backdrop
	asm	mov	eax, [di]
	asm	or	eax, es:[di]
	asm	jnz	blend
	asm	mov	eax, fs:[di]
	asm	mov	gs:[di], eax
	asm	sub	di, 4
	asm	jnc	composite
	asm	jmp	done

	// otherwise blend each pair of pixels
	blend:
	asm	mov	ax, [di+2]	// get 2 pixels from vbuffer
	asm	mov	bx, es:[di+2]	// get 2 pixels from alpha
	asm	and	al, al		// is this pixel drawn?
	asm	jz	skipper0_hi
	asm	add	bl, al		// composite with pixel in alpha
	asm	rcr	bl, 1		// average it
	asm	mov	[di+2], cl	// clear vbuffer
	skipper0_hi:
	asm	sub	bl, dl		// reduce intensity of expired samples
	asm	jnc	not_negative0_hi
	asm	xor	bl, bl
	not_negative0_hi:
	asm	and	ah, ah
	asm	jz	skipper1_hi
	asm	add	bh, ah
	asm	rcr	bh, 1
	asm	mov	[di+2+1], cl
	skipper1_hi:
	asm	sub	bh, dl
	asm	jnc	not_negative1_hi
	asm	xor	bh, bh
	not_negative1_hi:
	asm	mov	es:[di+2], bx	// store back to alpha
	asm	add	bx, fs:[di+2]	// now composite with texture
	asm	mov	gs:[di+2], bx	// store to vscreen

	asm	mov	ax, [di]	// get 2 pixels from vbuffer
	asm	mov	bx, es:[di]	// get 2 pixels from alpha
	asm	and	al, al		// is this pixel drawn?
	asm	jz	skipper0_lo
	asm	add	bl, al		// composite with pixel in alpha
	asm	rcr	bl, 1		// average it
	asm	mov	[di], cl	// clear vbuffer
	skipper0_lo:
	asm	sub	bl, dl		// reduce intensity of expired samples
	asm	jnc	not_negative0_lo
	asm	xor	bl, bl
	not_negative0_lo:
	asm	and	ah, ah
	asm	jz	skipper1_lo
	asm	add	bh, ah
	asm	rcr	bh, 1
	asm	mov	[di+1], cl
	skipper1_lo:
	asm	sub	bh, dl
	asm	jnc	not_negative1_lo
	asm	xor	bh, bh
	not_negative1_lo:
	asm	mov	es:[di], bx	// store back to alpha
	asm	add	bx, fs:[di]	// now composite with texture
	asm	mov	gs:[di], bx	// store to vscreen

	// too far for a short conditional jump back
	asm	sub	di, 4
	asm	jc	done
	asm	jmp	composite
	done:
	asm	pop	ds
}

void	cls ( word dest ) {
	asm	mov	es, [dest]
	asm	xor	di, di
	asm	xor	eax, eax
	asm	mov	cx, 64000/4
	asm	rep	stosd
}

void	clearbuffer ( word dest, word size, byte color ) {
	asm	mov	es, [dest]
	asm	xor	di, di
	asm	mov	al, color
	asm	mov	cx, size
	asm	rep	stosb
}

void	putpixel ( word x, word y, byte color, word destscreen ) {
	asm	mov	al, [color]
	asm	mov	es, [destscreen]
	asm	mov	bx, [y]
	asm	shl	bx, 8
	asm	mov	di, [x]
	asm	add	di, bx
	asm	shr	bx, 2
	asm	add	di, bx
	asm	mov	es:[di], al
}

dword	gettickcount ( void ) {
	dword	tickcount;

	// Returns the number of times the timer interrupt handler has been
	// called.  Therefore divide this timer by 18.2 to get seconds.

	asm	mov	ax, 0x0040
	asm	mov	es, ax
	asm	mov	di, 0x006C
	asm	mov	eax, dword ptr es:[di]
	asm	mov	dword ptr [tickcount], eax

	return tickcount;
}

// Returns the number of timer ticks from start to end.  The BIOS resets
// the tick count to zero at midnight, after 0x1800B0 ticks.
dword	elapsedticks ( dword start, dword end ) {
	if ( end >= start ) return end - start;
	return end + 0x1800B0L - start;
}

void	printfps ( dword frames, dword ticks ) {
	if ( ticks == 0 )
		printf ( "    Average Frames Per Second - n/a (ran too briefly)\n\n" );
	else
		printf ( "    Average Frames Per Second - %f\n\n", ( frames * 18.2 ) / ticks );
}

// The keyboard is read through the BIOS.  DOS checks for Ctrl+C and
// Ctrl+Break when it reads the keyboard and ends the program on the spot,
// leaving the screen in graphics mode; the BIOS returns them as keys.

#define	KEY_BREAK	3	// returned for Ctrl+C and Ctrl+Break

// Returns true if a key is waiting.
boolean	keypressed ( void ) {
	asm	mov	ah, 0x01
	asm	int	0x16
	asm	jnz	key_ready
	return false;
	key_ready:
	return true;
}

// Waits for a key and returns its character.  Extended keys (arrows,
// function keys, Alt combinations) have no character and return 0.
int	getkey ( void ) {
	word	key;

	asm	xor	ah, ah
	asm	int	0x16
	asm	mov	[key], ax

	if ( key == 0 ) return KEY_BREAK;		// Ctrl+Break
	key &= 0xff;
	if ( key == 0 || key == 0xe0 ) return 0;	// extended key
	return key;
}

// Ctrl+Break also makes DOS end the program at its next call, which would
// skip the exit sequence.  Returning nonzero tells DOS to carry on.
int	ignorebreak ( void ) {
	return 1;
}

void	fatalerror ( char *msg ) {
	printf ( "\n\n%s", msg );
	printf ( "\n\npress a key to quit..." );
	getkey ();
	exit ( 1 );
}

// Draws the first frame with drawframe, then fades it in to palette.
void	introsequence ( colortype *palette, void ( *drawframe ) ( void ) ) {
	int	fade;

	// the demo quits on Ctrl+C and Ctrl+Break itself
	ctrlbrk ( ignorebreak );

	fadepalette ( palette, 0 );
	drawframe ();

	// a slow fade in, ending at the full palette (fadepalette scales by fade/64)
	for ( fade = 0; fade <= 64; fade ++ ) {
		vretrace ();
		vretrace ();
		fadepalette ( palette, fade );
	}
}

void	exitsequence ( colortype *palette ) {
	int	fade;

	// a quick fade out, from the full palette to black
	for ( fade = 0; fade <= 64; fade ++ ) {
		vretrace ();
		fadepalette ( palette, 64 - fade );
	}
}

word	initpicture (char *picfile) {
	int	handle;
	long	filesize, offset;
	word	picbuffer, chunk, bytesread;

	if ((handle = open(picfile, O_RDONLY | O_BINARY)) == -1) fatalerror("*** error system_initpicture #1 ***");

	// the picture must fit in one 64K segment
	filesize = filelength(handle);
	if ( filesize < 0 || filesize > 65536L ) fatalerror ( "*** error system_initpicture #2 ***" );
	if ( allocmem ( (word)((filesize + 15) / 16), &picbuffer ) != -1 ) fatalerror ( "*** error system_initpicture #3 ***" );

	// one DOS read is at most 0xFFFF bytes, one byte short of a full 64K
	// picture, so read it in 32K chunks
	for ( offset = 0; offset < filesize; offset += chunk ) {
		chunk = ( filesize - offset > 0x8000L ) ? 0x8000 : (word)(filesize - offset);
		if ( _dos_read ( handle, MK_FP ( picbuffer, (word)offset ), chunk, &bytesread ) != 0 || bytesread != chunk )
			fatalerror ( "*** error system_initpicture #4 ***" );
	}

	close (handle);
	return picbuffer;
}

void	initpalette (char *palettefile, colortype *palette) {
	FILE	*f;

	if ( ( f = fopen ( palettefile, "rb" ) ) == NULL ) fatalerror ( "*** error system_initpalette #1 ***" );
	if ( fread ( palette, sizeof ( colortype ), 256, f ) != 256 ) fatalerror ( "*** error system_initpalette #2 ***" );
	fclose ( f );
}

word	initvirtual (long size) {
	word	buffer;

	if ( size <= 0 || size > 65536L ) fatalerror ( "*** error system_initvirtual #1 ***" );
	if ( allocmem ( (word)((size + 15) / 16), &buffer ) != -1 ) fatalerror ( "*** error system_initvirtual #2 ***" );

	// clearbuffer's word count can't hold 65536, so clear all but the
	// last byte, then the last byte on its own
	clearbuffer (buffer, (word)(size-1), 0);
	*(byte far *)MK_FP ( buffer, (word)(size-1) ) = 0;
	return buffer;
}

#endif	//	__SYSTEM_H__
