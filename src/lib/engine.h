//
// Retro programming in Borland C++ 3.1
//
#ifndef	__ENGINE_H__
#define	__ENGINE_H__

#include "lib/types.h"
#include "lib/graphics.h"
#include "lib/misc.h"

int		matrix[3][3];
long		xcos, xsin, ycos, ysin, zcos, zsin;
word		costable, sintable;
int		asinlookup[256];
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
	fread ( &vertices, 2, 1, f );
	if ( vertices >= MAX_VERTEX ) fatalerror ( "*** error engine_initobject #3 ***" );
	fread ( objectptr->objectvertex, sizeof(vectortype), vertices, f );

	objectptr->totalvertices = vertices;

	// load normals
	fread ( &normals, 2, 1, f );
	if ( normals >= MAX_VERTEX ) fatalerror ( "*** error engine_initobject #4 ***" );
	fread ( objectptr->objectnormal, sizeof(vectortype), normals, f );

	// check to be sure data is valid
	if ( vertices != normals ) fatalerror ( "*** error engine_initobject #5 ***" );

	// load facets
	fread ( &objectptr->totalfacets, 2, 1, f );
	if ( objectptr->totalfacets >= MAX_FACET ) fatalerror ( "*** error engine_initobject #6 ***" );
	fread ( objectptr->objectfacet, sizeof(facettype), objectptr->totalfacets, f );

	// change facets from array indices to offsets
	for ( facet = 0; facet < objectptr->totalfacets; facet ++ ) {
		objectptr->objectfacet[facet].a = (word)&objectptr->screenvertex[objectptr->objectfacet[facet].a];
		objectptr->objectfacet[facet].b = (word)&objectptr->screenvertex[objectptr->objectfacet[facet].b];
		objectptr->objectfacet[facet].c = (word)&objectptr->screenvertex[objectptr->objectfacet[facet].c];
	}

	// load texture coords
	if (fread ( &texturecoords, 2, 1, f ) != NULL) {
		if ( texturecoords >= MAX_VERTEX ) fatalerror ( "*** error engine_initobject #7 ***" );
		for (loop = 0; loop < texturecoords; loop++) {
			fread ( &objecttexture, sizeof(texturetype), 1, f );
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

	if (!(lightptr = new lighttype)) fatalerror("*** error engine_initlight ***");

	length = sqrt ( x * x + y * y + z * z);
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

void	initsincos ( void ) {
	int	loop, temp1;
	long	temp2;

	if ( allocmem ( MAX_ANGLE * 4 / 16, &costable ) != -1 ) fatalerror ( "*** error misc_initsincos #1 ***" );
	if ( allocmem ( MAX_ANGLE * 4 / 16, &sintable ) != -1 ) fatalerror ( "*** error misc_initsincos #2 ***" );

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

	// create arc cosine correction lookup
	for ( loop = -128; loop < 128; loop ++ )
		asinlookup[loop+128] = 255.0 * asin ( loop / 128.0 ) / M_PI + 127;
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
}

void	rotatevertices (objecttype *objectptr) {
	int	loop, xint, yint, zint;
	long	xtemp, ytemp, ztemp, x, y, z;

	for ( loop = 0; loop < objectptr->totalvertices; loop ++ ) {
		xtemp = objectptr->objectvertex[loop].x;
		ytemp = objectptr->objectvertex[loop].y;
		ztemp = objectptr->objectvertex[loop].z;

		x = (xtemp * matrix[0][0] + ytemp * matrix[0][1] + ztemp * matrix[0][2]) >> 8;
		y = (xtemp * matrix[1][0] + ytemp * matrix[1][1] + ztemp * matrix[1][2]) >> 8;
		z = (xtemp * matrix[2][0] + ytemp * matrix[2][1] + ztemp * matrix[2][2]) >> 8;

		zint = (z >> 8) + objectptr->zdepth;
		if (zint == 0) zint = 1;
		xint = x / zint + 160;
		yint = y / zint + 100;

		objectptr->screenvertex[loop].x = xint;
		objectptr->screenvertex[loop].y = yint;
		objectptr->screenvertex[loop].z = z;
	}
}

void	rotatenormals (objecttype *objectptr) {
	int	loop;
	long	xtemp, ytemp, ztemp, x, y, z;

	for ( loop = 0; loop < objectptr->totalvertices; loop ++ ) {
		xtemp = objectptr->objectnormal[loop].x;
		ytemp = objectptr->objectnormal[loop].y;
		ztemp = objectptr->objectnormal[loop].z;

		x = (xtemp * matrix[0][0] + ytemp * matrix[0][1] + ztemp * matrix[0][2]) >> 8;
		y = (xtemp * matrix[1][0] + ytemp * matrix[1][1] + ztemp * matrix[1][2]) >> 8;
		z = (xtemp * matrix[2][0] + ytemp * matrix[2][1] + ztemp * matrix[2][2]) >> 8;

		objectptr->screenvertex[loop].u = x;
		objectptr->screenvertex[loop].v = y;
		objectptr->screenvertex[loop].uvz = z;
	}
}

void	rotatenormalsenviro (objecttype *objectptr) {
	int	loop;
	long	xtemp, ytemp, ztemp, x, y, z;

	for ( loop = 0; loop < objectptr->totalvertices; loop ++ ) {
		xtemp = objectptr->objectnormal[loop].x;
		ytemp = objectptr->objectnormal[loop].y;
		ztemp = objectptr->objectnormal[loop].z;

		x = (xtemp * matrix[0][0] + ytemp * matrix[0][1] + ztemp * matrix[0][2]) >> 8;
		y = (xtemp * matrix[1][0] + ytemp * matrix[1][1] + ztemp * matrix[1][2]) >> 8;

		objectptr->screenvertex[loop].u = asinlookup[(x+256) >> 1];
		objectptr->screenvertex[loop].v = asinlookup[(y+256) >> 1];
	}
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
	int		facet, xnormal, depthbin;
	screentype	*v1, *v2, *v3;

	for ( facet = 0; facet < objectptr->totalfacets; facet++ ) {
		// grab pointers to the facets 3 vertices
		v1 = (screentype*)objectptr->objectfacet[facet].a;
		v2 = (screentype*)objectptr->objectfacet[facet].b;
		v3 = (screentype*)objectptr->objectfacet[facet].c;

		xnormal = (v2->x - v1->x)*(v1->y - v3->y) - (v2->y - v1->y)*(v1->x - v3->x);
		if ( xnormal < 0 ) continue;

		// find the z midpoint of the facet
		depthbin = ( v1->z >> 11) + (v2->z >> 11) + (v3->z >> 11) + 40;

		// is the facet depth inside the bin sort range?
		if (( depthbin < 80 )&&( depthbin >= 0 )) {

			bin_full:

			// is the selected bin full?
			if ( bincount[depthbin] < 128 ) {

				// add facet to bin
				bin_ptr = (word far*)MK_FP ( facetorder, ( depthbin << 8 ) + ( bincount[depthbin] << 1 ) );
				*bin_ptr = facet;
				bincount[depthbin]++;	// add one to the number of facets in the bin
			}
			else {
				// the bin is full, try the next one.
				depthbin++;
				goto	bin_full;
			}
		}
		else {
			asm	mov	ax,	0x03
			asm	int	0x10
			printf ( "facet: %i depthbin: %i", facet, depthbin );
			fatalerror("*** error engine_sortfacets ***");
		}
	}
}

#endif	//	__ENGINE_H__
