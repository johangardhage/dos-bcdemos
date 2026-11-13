//
// Retro programming in Borland C++ 3.1
//
#ifndef	__ENGINE_H__
#define	__ENGINE_H__

#include "lib/types.h"
#include "lib/graphics.h"
#include "lib/misc.h"

int		matrix[3][3];
long		m00, m01, m02, m10, m11, m12, m20, m21, m22;	// the matrix as longs, for rotatevector
long		rx, ry, rz;	// the result of rotatevector
long		xcos, xsin, ycos, ysin, zcos, zsin;
word		costable, sintable;
int		asinlookup[257];	// environment map coordinate for a normal component from -256 to +256 (see enviroindex)
word far	*bin_ptr;
word		facetorder;
int		bincount[80];

objecttype	*initobject (char *filename) {
	FILE		*f;
	word		vertices, normals, facet, texturecoords;
	texturetype	objecttexture;
	int		loop;
	objecttype	*objectptr;

	if (!(objectptr = new objecttype)) fatalerror("*** error engine_initobject #1 ***");

	if ( ( f = fopen (filename, "rb" ) ) == NULL ) fatalerror ( "*** error engine_initobject #2 ***" );

	// load vertices
	if ( fread ( &vertices, 2, 1, f ) != 1 ) fatalerror ( "*** error engine_initobject #3 ***" );
	if ( vertices >= MAX_VERTEX ) fatalerror ( "*** error engine_initobject #4 ***" );
	if ( fread ( objectptr->objectvertex, sizeof(vectortype), vertices, f ) != vertices ) fatalerror ( "*** error engine_initobject #5 ***" );

	objectptr->totalvertices = vertices;

	// load normals
	if ( fread ( &normals, 2, 1, f ) != 1 ) fatalerror ( "*** error engine_initobject #6 ***" );
	if ( normals >= MAX_VERTEX ) fatalerror ( "*** error engine_initobject #7 ***" );
	if ( fread ( objectptr->objectnormal, sizeof(vectortype), normals, f ) != normals ) fatalerror ( "*** error engine_initobject #8 ***" );

	// check to be sure data is valid
	if ( vertices != normals ) fatalerror ( "*** error engine_initobject #9 ***" );

	// load facets
	if ( fread ( &objectptr->totalfacets, 2, 1, f ) != 1 ) fatalerror ( "*** error engine_initobject #10 ***" );
	if ( objectptr->totalfacets >= MAX_FACET ) fatalerror ( "*** error engine_initobject #11 ***" );
	if ( fread ( objectptr->objectfacet, sizeof(facettype), objectptr->totalfacets, f ) != objectptr->totalfacets ) fatalerror ( "*** error engine_initobject #12 ***" );

	// change facets from array indices to offsets
	for ( facet = 0; facet < objectptr->totalfacets; facet ++ ) {
		if ( objectptr->objectfacet[facet].a >= vertices ||
		     objectptr->objectfacet[facet].b >= vertices ||
		     objectptr->objectfacet[facet].c >= vertices ) fatalerror ( "*** error engine_initobject #13 ***" );
		objectptr->objectfacet[facet].a = (word)&objectptr->screenvertex[objectptr->objectfacet[facet].a];
		objectptr->objectfacet[facet].b = (word)&objectptr->screenvertex[objectptr->objectfacet[facet].b];
		objectptr->objectfacet[facet].c = (word)&objectptr->screenvertex[objectptr->objectfacet[facet].c];
	}

	// load texture coords
	if (fread ( &texturecoords, 2, 1, f ) == 1) {
		if ( texturecoords > vertices ) fatalerror ( "*** error engine_initobject #14 ***" );
		for (loop = 0; loop < texturecoords; loop++) {
			if ( fread ( &objecttexture, sizeof(texturetype), 1, f ) != 1 ) fatalerror ( "*** error engine_initobject #15 ***" );
			objectptr->screenvertex[loop].a = objecttexture.a;
			objectptr->screenvertex[loop].b = objecttexture.b;
		}
	}
	fclose ( f );
	return objectptr;
}

lighttype	*initlight (int x, int y, int z, int xinc, int yinc) {
	lighttype	*lightptr;
	float		xlen, ylen, zlen, length;

	if (!(lightptr = new lighttype)) fatalerror("*** error engine_initlight #1 ***");

	length = sqrt ( (float)x * x + (float)y * y + (float)z * z );
	if ( length == 0 ) fatalerror ( "*** error engine_initlight #2 ***" );
	xlen = x / length;
	ylen = y / length;
	zlen = z / length;

	lightptr->x = (xlen * 256);
	lightptr->y = (ylen * 256);
	lightptr->z = (zlen * 256);

	lightptr->xinc = xinc;
	lightptr->yinc = yinc;

	lightptr->xrot = 0;
	lightptr->yrot = 0;

	return lightptr;
}

// Calculates each facet's normal from its vertices, scaled to a length of 256
// like the vertex normals, for lighting a facet as one flat surface.
void	initfacetnormals ( objecttype *objectptr, vectortype *facetnormal ) {
	int		facet;
	vectortype	*v1, *v2, *v3;
	float		ux, uy, uz, wx, wy, wz, nx, ny, nz, length;

	for ( facet = 0; facet < objectptr->totalfacets; facet ++ ) {
		// the facets hold offsets to the screen vertices, which are numbered
		// like the object vertices
		v1 = &objectptr->objectvertex[(screentype*)objectptr->objectfacet[facet].a - objectptr->screenvertex];
		v2 = &objectptr->objectvertex[(screentype*)objectptr->objectfacet[facet].b - objectptr->screenvertex];
		v3 = &objectptr->objectvertex[(screentype*)objectptr->objectfacet[facet].c - objectptr->screenvertex];

		ux = v2->x - v1->x;
		uy = v2->y - v1->y;
		uz = v2->z - v1->z;
		wx = v3->x - v1->x;
		wy = v3->y - v1->y;
		wz = v3->z - v1->z;

		nx = uy * wz - uz * wy;
		ny = uz * wx - ux * wz;
		nz = ux * wy - uy * wx;
		length = sqrt ( nx * nx + ny * ny + nz * nz );
		if ( length == 0 ) length = 1;

		facetnormal[facet].x = nx * 256 / length;
		facetnormal[facet].y = ny * 256 / length;
		facetnormal[facet].z = nz * 256 / length;
	}
}

void	initsincos ( void ) {
	int	loop, temp1;
	long	temp2;

	if ( allocmem ( MAX_ANGLE * 4 / 16, &costable ) != -1 ) fatalerror ( "*** error engine_initsincos #1 ***" );
	if ( allocmem ( MAX_ANGLE * 4 / 16, &sintable ) != -1 ) fatalerror ( "*** error engine_initsincos #2 ***" );

	for ( loop = 0; loop < MAX_ANGLE; loop ++ ) {
		temp1 = loop << 2;
		temp2 = cos ( (float)loop / MAX_ANGLE * M_PI * 2 ) * 256;
		asm	mov	es, [costable]
		asm	mov	di, [temp1]
		asm	mov	eax, dword ptr [temp2]
		asm	mov	dword ptr [es:di], eax
		temp2 = sin ( (float)loop / MAX_ANGLE * M_PI * 2 ) * 256;
		asm	mov	es, [sintable]
		asm	mov	di, [temp1]
		asm	mov	eax, dword ptr [temp2]
		asm	mov	dword ptr [es:di], eax
	}

	// create arc sine correction lookup: a normal component from -1 to +1
	// maps to 0..255, centred on 127.5 (rounded to the nearest texel).
	// the +1 pole lands on 255.5, so saturate to stay inside the map
	for ( loop = -128; loop <= 128; loop ++ ) {
		temp1 = 255.0 * asin ( loop / 128.0 ) / M_PI + 128.5;
		if ( temp1 > 255 ) temp1 = 255;
		asinlookup[loop+128] = temp1;
	}
}

void	calcangles (objecttype *objectptr) {
	word	temp1, temp2, temp3;

	temp1 = objectptr->xrotation << 2;
	temp2 = objectptr->yrotation << 2;
	temp3 = objectptr->zrotation << 2;

	// get the cos and sin values for the selected rotations
	asm	mov	es, [costable]
	asm	mov	fs, [sintable]
	asm	mov	di, [temp1]
	asm	mov	eax, dword ptr [es:di]
	asm	mov	dword ptr [xcos], eax
	asm	mov	eax, dword ptr [fs:di]
	asm	mov	dword ptr [xsin], eax
	asm	mov	di, [temp2]
	asm	mov	eax, dword ptr [es:di]
	asm	mov	dword ptr [ycos], eax
	asm	mov	eax, dword ptr [fs:di]
	asm	mov	dword ptr [ysin], eax
	asm	mov	di, [temp3]
	asm	mov	eax, dword ptr [es:di]
	asm	mov	dword ptr [zcos], eax
	asm	mov	eax, dword ptr [fs:di]
	asm	mov	dword ptr [zsin], eax

	// update the x rotation while keeping it between 0 and MAX_ANGLE
	objectptr->xrotation = objectptr->xrotation + objectptr->xinc;
	if ( objectptr->xrotation >= MAX_ANGLE ) objectptr->xrotation -= MAX_ANGLE;
	if ( objectptr->xrotation < 0 ) objectptr->xrotation += MAX_ANGLE;

	// update the y rotation while keeping it between 0 and MAX_ANGLE
	objectptr->yrotation = objectptr->yrotation + objectptr->yinc;
	if ( objectptr->yrotation >= MAX_ANGLE ) objectptr->yrotation -= MAX_ANGLE;
	if ( objectptr->yrotation < 0 ) objectptr->yrotation += MAX_ANGLE;

	// update the z rotation while keeping it between 0 and MAX_ANGLE
	objectptr->zrotation = objectptr->zrotation + objectptr->zinc;
	if ( objectptr->zrotation >= MAX_ANGLE ) objectptr->zrotation -= MAX_ANGLE;
	if ( objectptr->zrotation < 0 ) objectptr->zrotation += MAX_ANGLE;
}

void	initmatrix () {
	long	temp1, temp2;

	temp1 = (xcos * ysin) >> 8;
	temp2 = (xsin * ysin) >> 8;
	matrix[0][0]=(ycos * zcos) >> 8;
	matrix[0][1]=(-ycos * zsin) >> 8;
	matrix[0][2]=(-ysin);
	matrix[1][0]=((xcos * zsin) >> 8) - ((temp2 * zcos) >> 8);
	matrix[1][1]=((temp2 * zsin) >> 8) + ((xcos * zcos) >> 8);
	matrix[1][2]=(-xsin * ycos) >> 8;
	matrix[2][0]=((temp1 * zcos) >> 8) + ((zsin * xsin) >> 8);
	matrix[2][1]=((zcos * xsin) >> 8) - ((temp1 * zsin) >> 8);
	matrix[2][2]=(xcos * ycos) >> 8;

	m00 = matrix[0][0];	m01 = matrix[0][1];	m02 = matrix[0][2];
	m10 = matrix[1][0];	m11 = matrix[1][1];	m12 = matrix[1][2];
	m20 = matrix[2][0];	m21 = matrix[2][1];	m22 = matrix[2][2];
}

// Rotates a vector by the matrix into rx, ry and rz.
void	rotatevector ( int x, int y, int z ) {
	// rx = ( x * m00 + y * m01 + z * m02 ) >> 8;
	asm	movsx	eax, word ptr [x]
	asm	imul	eax, dword ptr [m00]
	asm	movsx	ecx, word ptr [y]
	asm	imul	ecx, dword ptr [m01]
	asm	add	eax, ecx
	asm	movsx	ecx, word ptr [z]
	asm	imul	ecx, dword ptr [m02]
	asm	add	eax, ecx
	asm	sar	eax, 8
	asm	mov	dword ptr [rx], eax

	// ry = ( x * m10 + y * m11 + z * m12 ) >> 8;
	asm	movsx	eax, word ptr [x]
	asm	imul	eax, dword ptr [m10]
	asm	movsx	ecx, word ptr [y]
	asm	imul	ecx, dword ptr [m11]
	asm	add	eax, ecx
	asm	movsx	ecx, word ptr [z]
	asm	imul	ecx, dword ptr [m12]
	asm	add	eax, ecx
	asm	sar	eax, 8
	asm	mov	dword ptr [ry], eax

	// rz = ( x * m20 + y * m21 + z * m22 ) >> 8;
	asm	movsx	eax, word ptr [x]
	asm	imul	eax, dword ptr [m20]
	asm	movsx	ecx, word ptr [y]
	asm	imul	ecx, dword ptr [m21]
	asm	add	eax, ecx
	asm	movsx	ecx, word ptr [z]
	asm	imul	ecx, dword ptr [m22]
	asm	add	eax, ecx
	asm	sar	eax, 8
	asm	mov	dword ptr [rz], eax
}

void	rotatevertices (objecttype *objectptr) {
	vectortype	*vptr = objectptr->objectvertex;
	screentype	*sptr = objectptr->screenvertex;
	word		count = objectptr->totalvertices;
	int		zdepth = objectptr->zdepth;

	if ( count == 0 ) return;

	// for ( loop = 0; loop < objectptr->totalvertices; loop++, vptr++, sptr++ )
	asm	mov	si, word ptr [vptr]
	asm	mov	di, word ptr [sptr]
	rot_vert_loop:
		// rx = ( vptr->x * m00 + vptr->y * m01 + vptr->z * m02 ) >> 8;
		asm	movsx	ebx, word ptr [si]
		asm	movsx	ecx, word ptr [si + 2]

		asm	mov	eax, ebx
		asm	imul	eax, dword ptr [m00]
		asm	mov	edx, ecx
		asm	imul	edx, dword ptr [m01]
		asm	add	eax, edx
		asm	movsx	edx, word ptr [si + 4]
		asm	imul	edx, dword ptr [m02]
		asm	add	eax, edx
		asm	sar	eax, 8
		asm	mov	dword ptr [rx], eax

		// ry = ( vptr->x * m10 + vptr->y * m11 + vptr->z * m12 ) >> 8;
		asm	mov	eax, ebx
		asm	imul	eax, dword ptr [m10]
		asm	mov	edx, ecx
		asm	imul	edx, dword ptr [m11]
		asm	add	eax, edx
		asm	movsx	edx, word ptr [si + 4]
		asm	imul	edx, dword ptr [m12]
		asm	add	eax, edx
		asm	sar	eax, 8
		asm	mov	dword ptr [ry], eax

		// rz = ( vptr->x * m20 + vptr->y * m21 + vptr->z * m22 ) >> 8;
		// sptr->z = rz;
		asm	mov	eax, ebx
		asm	imul	eax, dword ptr [m20]
		asm	mov	edx, ecx
		asm	imul	edx, dword ptr [m21]
		asm	add	eax, edx
		asm	movsx	edx, word ptr [si + 4]
		asm	imul	edx, dword ptr [m22]
		asm	add	eax, edx
		asm	sar	eax, 8
		asm	mov	word ptr [di + 4], ax

		// zint = ( rz >> 8 ) + zdepth;
		// if ( zint == 0 ) zint = 1;
		// sptr->x = rx / zint + 160;
		// sptr->y = ry / zint + 100;
		asm	sar	eax, 8
		asm	add	ax, word ptr [zdepth]
		asm	jnz	zint_ok
		asm	mov	ax, 1
	zint_ok:
		asm	movsx	ecx, ax
		asm	mov	eax, dword ptr [rx]
		asm	cdq
		asm	idiv	ecx
		asm	add	ax, 160
		asm	mov	word ptr [di], ax
		asm	mov	eax, dword ptr [ry]
		asm	cdq
		asm	idiv	ecx
		asm	add	ax, 100
		asm	mov	word ptr [di + 2], ax

		// vptr++; sptr++;
		asm	add	si, 6
		asm	add	di, 18
		asm	dec	word ptr [count]
		asm	jz	rot_vert_done
		asm	jmp	rot_vert_loop
	rot_vert_done:;
}

void	rotatenormals (objecttype *objectptr) {
	int	loop;

	for ( loop = 0; loop < objectptr->totalvertices; loop ++ ) {
		rotatevector ( objectptr->objectnormal[loop].x, objectptr->objectnormal[loop].y, objectptr->objectnormal[loop].z );

		objectptr->screenvertex[loop].u = rx;
		objectptr->screenvertex[loop].v = ry;
		objectptr->screenvertex[loop].uvz = rz;
	}
}

// Returns the asinlookup index for a rotated normal component.  The
// fixed-point rotation can push a component slightly past -256..256.
int	enviroindex ( int c ) {
	c = ( c + 256 ) >> 1;
	if ( c < 0 ) return 0;
	if ( c > 256 ) return 256;
	return c;
}

void	rotatenormalsenviro (objecttype *objectptr) {
	vectortype	*nptr = objectptr->objectnormal;
	screentype	*sptr = objectptr->screenvertex;
	word		count = objectptr->totalvertices;

	if ( count == 0 ) return;

	// for ( loop = 0; loop < objectptr->totalvertices; loop++, nptr++, sptr++ )
	asm	mov	si, word ptr [nptr]
	asm	mov	di, word ptr [sptr]
	rot_norm_loop:
		// rx = ( nptr->x * m00 + nptr->y * m01 + nptr->z * m02 ) >> 8;
		asm	movsx	ebx, word ptr [si]
		asm	movsx	ecx, word ptr [si + 2]

		asm	mov	eax, ebx
		asm	imul	eax, dword ptr [m00]
		asm	mov	edx, ecx
		asm	imul	edx, dword ptr [m01]
		asm	add	eax, edx
		asm	movsx	edx, word ptr [si + 4]
		asm	imul	edx, dword ptr [m02]
		asm	add	eax, edx
		asm	sar	eax, 8
		asm	xchg	ebx, eax

		// ry = ( nptr->x * m10 + nptr->y * m11 + nptr->z * m12 ) >> 8;
		asm	imul	eax, dword ptr [m10]
		asm	mov	edx, ecx
		asm	imul	edx, dword ptr [m11]
		asm	add	eax, edx
		asm	movsx	edx, word ptr [si + 4]
		asm	imul	edx, dword ptr [m12]
		asm	add	eax, edx
		asm	sar	eax, 8

		// sptr->u = asinlookup[enviroindex ( rx )];
		asm	add	bx, 256
		asm	sar	bx, 1
		asm	jns	u_nonneg
		asm	xor	bx, bx
	u_nonneg:
		asm	cmp	bx, 256
		asm	jle	u_clamped
		asm	mov	bx, 256
	u_clamped:
		asm	shl	bx, 1
		asm	mov	dx, word ptr [asinlookup + bx]
		asm	mov	word ptr [di + 10], dx

		// sptr->v = asinlookup[enviroindex ( ry )];
		asm	mov	bx, ax
		asm	add	bx, 256
		asm	sar	bx, 1
		asm	jns	v_nonneg
		asm	xor	bx, bx
	v_nonneg:
		asm	cmp	bx, 256
		asm	jle	v_clamped
		asm	mov	bx, 256
	v_clamped:
		asm	shl	bx, 1
		asm	mov	dx, word ptr [asinlookup + bx]
		asm	mov	word ptr [di + 12], dx

		// nptr++; sptr++;
		asm	add	si, 6
		asm	add	di, 18
		asm	dec	word ptr [count]
		asm	jz	rot_norm_done
		asm	jmp	rot_norm_loop
	rot_norm_done:;
}

void	rotatelight (lighttype *lightptr) {
	int	loop;
	long	xtemp, ytemp, ztemp, x, y, z;
	long	xcos, xsin, ycos, ysin;
	word	temp1, temp2;

	temp1 = lightptr->xrot << 2;
	temp2 = lightptr->yrot << 2;
	// get the cos and sin values for the selected rotations
	asm	mov	es, [costable]
	asm	mov	fs, [sintable]
	asm	mov	di, [temp1]
	asm	mov	eax, dword ptr [es:di]
	asm	mov	dword ptr [xcos], eax
	asm	mov	eax, dword ptr [fs:di]
	asm	mov	dword ptr [xsin], eax
	asm	mov	di, [temp2]
	asm	mov	eax, dword ptr [es:di]
	asm	mov	dword ptr [ycos], eax
	asm	mov	eax, dword ptr [fs:di]
	asm	mov	dword ptr [ysin], eax

	// update the x rotation while keeping it between 0 and MAX_ANGLE
	lightptr->xrot = lightptr->xrot + lightptr->xinc;
	if ( lightptr->xrot >= MAX_ANGLE ) lightptr->xrot -= MAX_ANGLE;
	if ( lightptr->xrot < 0 ) lightptr->xrot += MAX_ANGLE;

	// update the y rotation while keeping it between 0 and MAX_ANGLE
	lightptr->yrot = lightptr->yrot + lightptr->yinc;
	if ( lightptr->yrot >= MAX_ANGLE ) lightptr->yrot -= MAX_ANGLE;
	if ( lightptr->yrot < 0 ) lightptr->yrot += MAX_ANGLE;

	xtemp = lightptr->x;
	ytemp = lightptr->y;
	ztemp = lightptr->z;

	// rotate around the x axis
	y = ( ytemp * xcos - ztemp * xsin ) >> 8;
	z = ( ytemp * xsin + ztemp * xcos ) >> 8;

	// rotate around the y axis
	x = ( xtemp * ycos - z * ysin ) >> 8;
	z = ( xtemp * ysin + z * ycos ) >> 8;

	lightptr->u = x;
	lightptr->v = y;
	lightptr->uvz = z;
}

void	sortfacets (objecttype *objectptr) {
	int		facet, depthbin;
	facettype	*fptr = objectptr->objectfacet;
	int		totalfacets = objectptr->totalfacets;

	// es = facetorder, for the bin stores below
	asm	mov	es, word ptr [facetorder]
	for ( facet = 0; facet < totalfacets; facet++, fptr++ ) {
		// v1 = (screentype*)fptr->a;
		// v2 = (screentype*)fptr->b;
		// v3 = (screentype*)fptr->c;
		asm	mov	si, word ptr [fptr]
		asm	mov	bx, word ptr [si]
		asm	mov	di, word ptr [si + 2]
		asm	mov	si, word ptr [si + 4]

		// twice the facet's screen area, in 32 bits: it overflows int for
		// large facets
		// if ( (long)( v2->x - v1->x ) * ( v1->y - v3->y )
		//    - (long)( v2->y - v1->y ) * ( v1->x - v3->x ) >= 0 )
		asm	mov	ax, word ptr [di]
		asm	sub	ax, word ptr [bx]
		asm	movsx	eax, ax
		asm	mov	dx, word ptr [bx + 2]
		asm	sub	dx, word ptr [si + 2]
		asm	movsx	edx, dx
		asm	imul	eax, edx

		asm	mov	cx, word ptr [di + 2]
		asm	sub	cx, word ptr [bx + 2]
		asm	movsx	ecx, cx
		asm	mov	dx, word ptr [bx]
		asm	sub	dx, word ptr [si]
		asm	movsx	edx, dx
		asm	imul	ecx, edx

		asm	sub	eax, ecx
		asm	jl	facet_next

		// find the z midpoint of the facet
		// depthbin = ( v1->z >> 11 ) + ( v2->z >> 11 ) + ( v3->z >> 11 ) + 40;
		asm	mov	ax, word ptr [bx + 4]
		asm	sar	ax, 11
		asm	mov	dx, word ptr [di + 4]
		asm	sar	dx, 11
		asm	add	ax, dx
		asm	mov	dx, word ptr [si + 4]
		asm	sar	dx, 11
		asm	add	ax, dx
		asm	add	ax, 40

		// if the selected bin is full, try the next one
		// while ( (unsigned)depthbin < 80 && bincount[depthbin] >= 128 ) depthbin++;
	bin_check:
		asm	cmp	ax, 80
		asm	jae	bin_error
		asm	mov	bx, ax
		asm	shl	bx, 1
		asm	mov	dx, word ptr [bincount + bx]
		asm	cmp	dx, 128
		asm	jl	bin_store
		asm	inc	ax
		asm	jmp	bin_check

	bin_store:
		// add facet to bin
		// bin_ptr = (word far*)MK_FP ( facetorder, ( depthbin << 8 ) + ( bincount[depthbin] << 1 ) );
		// *bin_ptr = facet;
		// bincount[depthbin]++;
		asm	inc	word ptr [bincount + bx]
		asm	mov	bh, al
		asm	mov	bl, dl
		asm	shl	bl, 1
		asm	mov	ax, word ptr [facet]
		asm	mov	es:[bx], ax
		asm	jmp	facet_next

	bin_error:
		// depthbin = _AX;
		// setgfxmode ( 0x03 );
		asm	mov	word ptr [depthbin], ax
		asm	mov	ax,	0x03
		asm	int	0x10
		printf ( "facet: %i depthbin: %i", facet, depthbin );
		fatalerror("*** error engine_sortfacets ***");
		asm	mov	es, word ptr [facetorder]

	facet_next:;
	}
}

#endif	//	__ENGINE_H__
