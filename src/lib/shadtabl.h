//
// Retro programming in Borland C++ 3.1
//
#ifndef	__SHADTABL_H__
#define	__SHADTABL_H__

#include "lib/system.h"

#define	NUM_BASECOLORS	4096

#define	COLOR_RED	0
#define	COLOR_GREEN	1
#define	COLOR_BLUE	2

#define	PAL_SIZE	255
#define	PAL_OFFSET	1

#define	K_AMBIENT	0.2
#define	K_DIFFUSE       0.9
#define	K_SPECULAR      0.7
#define	K_ATTENUATION   1.0
#define	K_FALLOFF	150

#define	FACE_R		0.86
#define	FACE_G		0.23
#define	FACE_B		0.59

#define	AMBIENT_R	0.0
#define	AMBIENT_G	0.0
#define	AMBIENT_B	0.0

#define	LIGHT_R		0.83
#define	LIGHT_G		0.83
#define	LIGHT_B		0.83

void	initshadetable (char *palettefile, byte *shadetable, colortype *palette) {
	FILE	*f;

	if ( ( f = fopen ( palettefile, "rb" ) ) == NULL ) fatalerror ( "*** error shadtabl_initshadetable #1 ***" );
	if ( fread ( palette, sizeof ( colortype ), 256, f ) != 256 ||
	     fread ( shadetable, sizeof ( byte ), NUM_BASECOLORS, f ) != NUM_BASECOLORS ) fatalerror ( "*** error shadtabl_initshadetable #2 ***" );
	fclose ( f );
}

// specularity is the strength of the highlight, falloff its sharpness
real	phongillumination ( real diffusecolor, real specularcolor, real lightcolor,  real ambientcolor, real theta, double specularity, double falloff ) {
	real	phongcolor;

	phongcolor =  ( K_DIFFUSE * diffusecolor * cos( theta ));
	if ( theta < M_PI / 4 )
		phongcolor += ( specularity * specularcolor * pow( cos( theta * 2 ), falloff ));
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
		phongpal[baseindex].r = 63 * phongillumination ( FACE_R, 1.0, LIGHT_R, AMBIENT_R, incedent, K_SPECULAR, K_FALLOFF );
		phongpal[baseindex].g = 63 * phongillumination ( FACE_G, 1.0, LIGHT_G, AMBIENT_G, incedent, K_SPECULAR, K_FALLOFF );
		phongpal[baseindex].b = 63 * phongillumination ( FACE_B, 1.0, LIGHT_B, AMBIENT_B, incedent, K_SPECULAR, K_FALLOFF );
	}
}

word	initphongmap ( int phongsize ) {
	int	x, y;
	real	xcomp, ycomp, zcomp, incedent, temp;
	byte	paletteindex;
	word	enviromap;

	if ( allocmem ( 4096, &enviromap ) != -1 ) fatalerror ( "*** error shadtabl_initphongmap ***" );

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

#endif	//	__SHADTABL_H__
