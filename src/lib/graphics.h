//
// Retro programming in Borland C++ 3.1
//
#ifndef	__GRAPHICS_H__
#define	__GRAPHICS_H__

#include "lib/types.h"

void	setgfxmode ( word mode ) {
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
	asm	mov	di, 64000
	asm	mov	es, [alpha]
	asm	mov	fs, [backdrop]
	asm	mov	ax, 0xa000
	asm	mov	gs, ax
	asm	push	ds
	asm	mov	ds, [vscreen]
	asm	xor	cx, cx

	composite:
	asm	mov	ax, [di]	// get 2 pixels from vbuffer
	asm	mov	bx, es:[di]	// get 2 pixels from alpha

	//PIXEL #1
	asm	and	al, al		// is this pixel drawn?
	asm	jz	skipper0
	asm	add	bl, al		// composite with pixel in alpha
	asm	rcr	bl, 1		// average it
	asm	mov	[di], cl	// clear vbuffer

	// motion blur, reduce intensity of expired samples
	skipper0:
	asm	sub	bl, level
	asm	jnc	not_negative0	// less than zero?
	asm	xor	bl, bl		// overflow, clear pixel
	not_negative0:

	//PIXEL #2
	asm	and	ah, ah
	asm	jz	skipper1
	asm	add	bh, ah
	asm	rcr	bh, 1
	asm	mov	[di+1], cl

	skipper1:
	asm	sub	bh, level
	asm	jnc	not_negative1
	asm	xor	bh, bh
	not_negative1:

	asm	mov	es:[di], bx	// store back to alpha
	asm	add	bx, fs:[di]     // now composite with texture
	asm 	mov	gs:[di], bx     // store to vscreen

	asm	sub	di, 2
	asm	jnc	composite
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

#endif	//	__GRAPHICS_H__
