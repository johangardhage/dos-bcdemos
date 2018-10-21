//
// Retro programming in Borland C++ 3.1
//
#ifndef	__MISC_H__
#define	__MISC_H__

#include "lib/types.h"
#include "lib/graphics.h"

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

void	fatalerror ( char *msg ) {
	printf ( "\n\n%s", msg );
	printf ( "\n\npress a key to quit..." );
	getch ();
	exit ( 0 );
}

void	introsequence ( void ) {
	int	fade;
	extern	colortype	palette[256];
	extern	void		drawframe ( void );

	fadepalette ( palette, 0 );
	drawframe ();

	// a slow fade in
	for ( fade = 0; fade < 64; fade ++ ) {
		vretrace ();
		vretrace ();
		fadepalette ( palette, fade );
	}
}

void	exitsequence ( void ) {
	int	fade;
	extern	colortype	palette[256];

	// a quick fade out
	for ( fade = 0; fade < 64; fade ++ ) {
		vretrace ();
		fadepalette ( palette, 63 - fade );
	}
}

word	initpicture (char *picfile) {
	int	handle;
	long	filesize;
	word	picbuffer;

	if ((handle = open(picfile, O_RDONLY | O_BINARY)) == -1) fatalerror("*** error misc_initpicture #1 ***");

	filesize = filelength(handle);
	if ( allocmem ( ceil(filesize / 16), &picbuffer ) != -1 ) fatalerror ( "*** error misc_initpicture #2 ***" );

	asm	mov	bx, handle
	asm	mov	ah, 0x3f
	asm	mov	cx, 0xffff
	asm	xor	dx, dx
	asm	push	ds
	asm	mov	ds, [picbuffer]
	asm	int	0x21
	asm	pop	ds

	close (handle);
	return picbuffer;
}

void	initpalette (char *palettefile, colortype *palette) {
	FILE	*f;

	if ( ( f = fopen ( palettefile, "rb" ) ) == NULL ) fatalerror ( "*** error misc_initpalette ***" );
	fread ( palette, sizeof ( colortype ), 256, f );
	fclose ( f );
}

void	initshadetable (char *palettefile, char *shadetable, colortype *palette) {
	FILE	*f;

	if ( ( f = fopen ( palettefile, "rb" ) ) == NULL ) fatalerror ( "*** error misc_initshadetable ***" );
	fread ( palette, sizeof ( colortype ), 256, f );
	fread ( shadetable, sizeof ( byte ), NUM_BASECOLORS, f );
	fclose ( f );
}

real	phongillumination ( real diffusecolor, real specularcolor, real lightcolor,  real ambientcolor, real theta ) {
	real	phongcolor;

	phongcolor =  ( K_DIFFUSE * diffusecolor * cos( theta ));
	if ( theta < M_PI / 4 )
		phongcolor += ( K_SPECULAR * specularcolor * pow( cos( theta * 2 ), K_FALLOFF ));
	phongcolor *= ( K_ATTENUATION * lightcolor );
	phongcolor += ( ambientcolor * K_AMBIENT * diffusecolor );

	// clip phong color to between 0.0 and 1.0
	if ( phongcolor > 1.0 )
		phongcolor = 1.0;
	else if ( phongcolor < 0.0 )
		phongcolor = 0.0;

	return phongcolor;
}

void	initphongpal ( colortype *phongpal ) {
	int	loop, baseindex;
	real	incedent;

	// create phong palette
	phongpal[0].r = 0;
	phongpal[0].g = 0;
	phongpal[0].b = 0;

	for ( loop = 0; loop < PAL_SIZE; loop ++ ) {

		// what is the incedent angle that we are calculating a color for?
		// Scale loop to 0 to 1, flip then scale it up to 0 to 90 degrees.
		// I say degrees but remember that the computer works in radians.
		incedent = ( (real)(PAL_SIZE - ( loop + 1 )) / PAL_SIZE ) * ( M_PI / 2 );

		baseindex = ( loop + PAL_OFFSET );

		// determine the rgb components
		phongpal[baseindex].r = 63 * phongillumination ( FACE_R, 1.0, LIGHT_R, AMBIENT_R, incedent );
		phongpal[baseindex].g = 63 * phongillumination ( FACE_G, 1.0, LIGHT_G, AMBIENT_G, incedent );
		phongpal[baseindex].b = 63 * phongillumination ( FACE_B, 1.0, LIGHT_B, AMBIENT_B, incedent );
	}
}

word	initphongmap ( int phongsize ) {
	int	x, y;
	real	xcomp, ycomp, zcomp, incedent, temp;
	byte	paletteindex;
	word	enviromap;

	if ( allocmem ( 4096, &enviromap ) != -1 ) fatalerror ( "*** error misc_initphongmap ***" );

	// clear the map
	asm	mov	es, [enviromap]
	asm	xor	di, di
	asm	mov	cx, 0x4000
	asm	xor	eax, eax
	asm	rep	stosd

	// calculate the top left quadrant of the phong enviroment map
	for ( x = 0; x < 128; x ++ )
		for ( y = 0; y < 128; y ++ ) {

			// determine the angle that will have this pixel mapped to
			// it in radian.
			xcomp = (127.5 - x) / 127.5 * (M_PI / 2);
			ycomp = (127.5 - y) / 127.5 * (M_PI / 2);

			// lets find "1 - (x^2 + y^2)" from above.
			temp = 1.0 - ( pow ( sin ( xcomp ), 2 ) + pow ( sin ( ycomp ), 2 ) );

			// we can only that the sqrt if temp is positive.  Also if temp is negative
			// the pixel in question will never be mapped onto the object so we don't
			// care about it.
			if ( temp >= 0.0 ) {
				// we now get the z component of the normal vector
				zcomp = sqrt ( temp );

				// find the angle
				incedent = asin ( zcomp );

				// now that we have the angle lets scale down to 0 to 1 since it is
				// an angle between 0 and 90 except it's in radians.
				// Then scale it up the be a palette index
				paletteindex = (byte) ( incedent / ( M_PI / 2 ) * phongsize );

				// put the index into the phongmap
				asm	mov	bh, byte ptr [x]
				asm	mov	bl, byte ptr [y]
				asm	mov	es, word ptr [enviromap]
				asm	mov	di, bx
				asm	mov	al, byte ptr [paletteindex]
				asm	mov	es:[di], al
			}
		}

	// copy the quadrant we just calculated to the other three (we flip)
	for ( x = 0; x < 128; x ++ )
		for ( y = 0; y < 128; y ++ ) {
			asm	mov	es, word ptr [enviromap]
			asm	mov	ch, byte ptr [x]
			asm	mov	cl, byte ptr [y]
			asm	mov	dx, cx
			asm	mov	si, cx
			asm	mov	bx, 0xffff
			asm	sub	bx, cx
			asm	mov	al, es:[si]
			asm	mov	di, bx
			asm	mov	es:[di], al
			asm	mov	cl, bl
			asm	mov	di, cx
			asm	mov	es:[di], al
			asm	mov	dh, bh
			asm	mov	di, dx
			asm	mov	es:[di], al
		}
	return enviromap;
}

word	initvirtual (long size) {
	word	buffer;

	if ( allocmem ( ceil(size / 16), &buffer ) != -1 ) fatalerror ( "*** error misc_initvirtual ***" );
	clearbuffer (buffer, size-1, 0);
	return buffer;
}

#endif	//	__MISC_H__
