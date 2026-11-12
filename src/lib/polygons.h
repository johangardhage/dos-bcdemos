//
// Retro programming in Borland C++ 3.1
//
#ifndef	__POLYGONS_H__
#define	__POLYGONS_H__

#include "lib/types.h"
#include <stddef.h>

screentype	*leftarray[3], *rightarray[3];
int		leftsection, rightsection;
int		leftsectionheight, rightsectionheight;
int		dudx, dudy, leftu, deltaleftu, leftv, deltaleftv;
int		dadx, dady, lefta, deltalefta, leftb, deltaleftb;
int		dgdx, leftg, deltaleftg;
int		leftx, rightx;
word		deltaleftx, deltarightx;

int	getrightsection ( void ) {
	int	height;

	screentype *v1 = rightarray[rightsection];
	screentype *v2 = rightarray[rightsection-1];

	height = v2->y - v1->y;
	if ( height == 0 ) return 0;

	deltarightx = ((v2->x - v1->x) << 6) / height;
	rightx = v1->x << 6;
	rightsectionheight = height;

	return height;
}

int	getleftsection ( void ) {
	int height;

	screentype *v1 = leftarray[leftsection];
	screentype *v2 = leftarray[leftsection-1];

	height = v2->y - v1->y;
	if ( height == 0 ) return 0;

	deltaleftx = ((v2->x - v1->x) << 6) / height;
	leftx = v1->x << 6;
	leftsectionheight = height;

	// texture coordinates
	deltalefta = (((v2->a - v1->a) << 6) / height) << 2;
	lefta = (word)v1->a << 8;
	deltaleftb = (((v2->b - v1->b) << 6) / height) << 2;
	leftb = (word)v1->b << 8;

	// phong coordinates
	deltaleftu = (((v2->u - v1->u) << 6) / height) << 2;
	leftu = (word)v1->u << 8;
	deltaleftv = (((v2->v - v1->v) << 6) / height) << 2;
	leftv = (word)v1->v << 8;

	// gouraud coordinates
	deltaleftg = (((v2->color - v1->color) << 6) / height) << 2;
	leftg = (word)v1->color << 8;

	return height;
}

enum { TRIANGLE_A = offsetof(screentype, a), TRIANGLE_B = offsetof(screentype, b),
       TRIANGLE_U = offsetof(screentype, u), TRIANGLE_V = offsetof(screentype, v),
       TRIANGLE_COLOR = offsetof(screentype, color) };

// Compute triangle gradients without repeated calls and argument copies.
// si/di/bx hold v1/v2/v3. Subtractions still wrap at 16 bits before widening,
// and signed division truncates toward zero.
// output = ( ( (long)( v1->field - v2->field ) << 14 ) + (long)( v3->field - v1->field ) * ratio ) / longest;
#define TRIANGLE_GRADIENT(field, output, ratio, longest) \
    asm mov ax, word ptr [si+field]; \
    asm sub ax, word ptr [di+field]; \
    asm movsx eax, ax; \
    asm shl eax, 14; \
    asm mov cx, word ptr [bx+field]; \
    asm sub cx, word ptr [si+field]; \
    asm movsx ecx, cx; \
    asm imul ecx, dword ptr [ratio]; \
    asm add eax, ecx; \
    asm cdq; \
    asm movsx ecx, word ptr [longest]; \
    asm idiv ecx; \
    asm mov word ptr [output], ax;

// Variant when ecx already holds the sign-extended longest span.
// output = ( ( (long)( v1->field - v2->field ) << 14 ) + (long)( v3->field - v1->field ) * ratio ) / ecx;
#define TRIANGLE_GRADIENT_ECX(field, output, ratio) \
    asm mov ax, word ptr [si+field]; \
    asm sub ax, word ptr [di+field]; \
    asm movsx eax, ax; \
    asm shl eax, 14; \
    asm mov dx, word ptr [bx+field]; \
    asm sub dx, word ptr [si+field]; \
    asm movsx edx, dx; \
    asm imul edx, dword ptr [ratio]; \
    asm add eax, edx; \
    asm cdq; \
    asm idiv ecx; \
    asm mov word ptr [output], ax;

void	triangle ( screentype *v1, screentype *v2, screentype *v3, byte color, word destscreen ) {
	long	yratio;
	int	temp, xlongest;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio = ((v2->y - v1->y) << 6) / temp;
	xlongest = ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 ) return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 ) return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 ) return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 ) return;
		}
	}

	word	destptr = v1->y * 320;

	while ( 1 ) {
		int	width = (rightx >> 6) - (leftx >> 6);

		if ( width > 0 ) {
			// setup es:[di]  -->  destination
			asm	mov	es, word ptr [destscreen]
			asm	mov	ax, word ptr [leftx]
			asm	shr	ax, 6
			asm	add	ax, word ptr [destptr]
			asm	mov	di, ax

			asm	mov	al, [color]
			asm	mov	cx, word ptr [width]
		pixel_loop:
			asm	mov	es:[di], al
			asm	inc	di
			asm	dec	cx
			asm	jnz	pixel_loop
		}

		destptr += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 ) return;
			if ( getleftsection () <= 0 ) return;
		}
		else {
			leftx += deltaleftx;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 ) return;
			if ( getrightsection () <= 0 ) return;
		}
		else
			rightx += deltarightx;
	}
}

void	gouraudtriangle ( screentype *v1, screentype *v2, screentype *v3, word destscreen ) {
	long	yratio;
	int	temp, xlongest;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio = ((v2->y - v1->y) << 6) / temp;
	xlongest = ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 ) return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 ) return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 ) return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 ) return;
		}
	}

	yratio <<= 8;
	word	dgdx;
	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	TRIANGLE_GRADIENT ( TRIANGLE_COLOR, dgdx, yratio, xlongest )
	word	destptr = v1->y * 320;

	while ( 1 ) {
		int	width = (rightx >> 6) - (leftx >> 6);

		if ( width > 0 ) {
			// setup es:[di]  -->  destination
			asm	mov	es, word ptr [destscreen]
			asm	mov	ax, word ptr [leftx]
			asm	shr	ax, 6
			asm	add	ax, word ptr [destptr]
			asm	mov	di, ax

			asm	mov	ax, word ptr [leftg]
			asm	mov	bx, word ptr [dgdx]
			asm	mov	cx, word ptr [width]
		pixel_loop:
			asm	mov	es:[di], ah
			asm	add	ax, bx
			asm	inc	di
			asm	dec	cx
			asm	jnz	pixel_loop
		}

		destptr += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 ) return;
			if ( getleftsection () <= 0 ) return;
		}
		else {
			leftx += deltaleftx;
			leftg += deltaleftg;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 ) return;
			if ( getrightsection () <= 0 ) return;
		}
		else
			rightx += deltarightx;
	}
}

void	envirotriangle ( screentype *v1, screentype *v2, screentype *v3, word enviromap, word destscreen ) {
	long	yratio;
	int	temp, xlongest;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio = ((v2->y - v1->y) << 6) / temp;
	xlongest = ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 ) return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 ) return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 ) return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 ) return;
		}
	}

	yratio <<= 8;
	word	dudx, dvdx;
	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	asm movsx ecx, word ptr [xlongest]
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_U, dudx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_V, dvdx, yratio )
	word	destptr = v1->y * 320;

	while ( 1 ) {
		int	width = (rightx >> 6) - (leftx >> 6);

		if ( width > 0 ) {
			asm	push	ds

			// setup es:[di]  -->  destination
			asm	mov	es, word ptr [destscreen]
			asm	mov	ax, word ptr [leftx]
			asm	shr	ax, 6
			asm	add	ax, word ptr [destptr]
			asm	mov	di, ax

			asm	mov	si, word ptr [leftu]
			asm	mov	dx, word ptr [leftv]
			asm	mov	ds, word ptr [enviromap]

			asm	mov	cx, word ptr [width]
		pixel_loop:
			asm	mov	bx, si
			asm	mov	bl, dh
			asm	mov	al, ds:[bx]
			asm	mov	es:[di], al
			asm	add	si, word ptr [dudx]
			asm	add	dx, word ptr [dvdx]
			asm	inc	di
			asm	dec	cx
			asm	jnz	pixel_loop

			asm	pop	ds
		}

		destptr += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 ) return;
			if ( getleftsection () <= 0 ) return;
		}
		else {
			leftx += deltaleftx;
			leftu += deltaleftu;
			leftv += deltaleftv;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 ) return;
			if ( getrightsection () <= 0 ) return;
		}
		else
			rightx += deltarightx;
	}
}

void	texturetriangle ( screentype *v1, screentype *v2, screentype *v3, word texturemap, word destscreen ) {
	long	yratio;
	int	temp, xlongest;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio = ((v2->y - v1->y) << 6) / temp;
	xlongest = ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 )
			return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 )
				return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 )
			return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 )
				return;
		}
	}

	yratio <<= 8;
	word	dadx, dbdx;
	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	asm movsx ecx, word ptr [xlongest]
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dadx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdx, yratio )
	word destptr = v1->y * 320;

	while ( 1 ) {
		int width = (rightx >> 6) - (leftx >> 6);

		if ( width > 0 ) {
			asm	push	ds

			// setup es:[di]  -->  destination
			asm	mov	es, word ptr [destscreen]
			asm	mov	ax, word ptr [leftx]
			asm	shr	ax, 6
			asm	add	ax, word ptr [destptr]
			asm	mov	di, ax

			asm	mov	ax, word ptr [lefta]
			asm	mov	dx, word ptr [leftb]
			asm	mov	ds, word ptr [texturemap]

			asm	mov	cx, word ptr [width]
			pixel_loop:
				asm	mov	bh, ah
				asm	mov	bl, dh
				asm	mov	bl, ds:[bx]
				asm	mov	es:[di], bl
				asm	add	ax, word ptr [dadx]
				asm	add	dx, word ptr [dbdx]
				asm	inc	di
				asm	dec	cx
				asm	jnz	pixel_loop

			asm	pop	ds
		}

		destptr += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 )
				return;
			if ( getleftsection () <= 0 )
				return;
		}
		else {
			leftx += deltaleftx;
			lefta += deltalefta;
			leftb += deltaleftb;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 )
				return;
			if ( getrightsection () <= 0 )
				return;
		}
		else
			rightx += deltarightx;
	}
}

void	flattexturetriangle ( screentype *v1, screentype *v2, screentype *v3, word texturemap, byte *shadetable, word shade, word destscreen ) {
	long	yratio;
	int	temp, xlongest;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio = ((v2->y - v1->y) << 6) / temp;
	xlongest = ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 )
			return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 )
				return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 )
			return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 )
				return;
		}
	}

	yratio <<= 8;
	word	dadx, dbdx;
	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	asm movsx ecx, word ptr [xlongest]
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dadx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdx, yratio )
	word destptr = v1->y * 320;

	while ( 1 ) {
		int	width = (rightx >> 6) - (leftx >> 6);

		if ( width > 0 ) {
			word	a, b, poffset;

			a = lefta;
			b = leftb;
			poffset = destptr + (leftx >> 6);

			// es:di destination, fs texture, cx and dx the texture
			// coordinates, bx the shade's row in the shade table
			asm	mov	es, word ptr [destscreen]
			asm	mov	fs, word ptr [texturemap]
			asm	mov	di, word ptr [poffset]
			asm	mov	cx, word ptr [a]
			asm	mov	dx, word ptr [b]
			asm	mov	bx, word ptr [shadetable]
			asm	add	bx, word ptr [shade]
		flat_pixel:
			// texel = texture[a_hi * 256 + b_hi]
			asm	mov	ax, cx
			asm	mov	al, dh
			asm	mov	si, ax
			asm	mov	al, fs:[si]
			// pixel = shadetable[texel * 128 + shade]
			asm	xor	ah, ah
			asm	shl	ax, 7
			asm	add	ax, bx
			asm	mov	si, ax
			asm	mov	al, [si]
			asm	stosb
			// a += dadx; b += dbdx;
			asm	add	cx, word ptr [dadx]
			asm	add	dx, word ptr [dbdx]
			asm	dec	word ptr [width]
			asm	jnz	flat_pixel
			}
		destptr += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 )
				return;
			if ( getleftsection () <= 0 )
				return;
		}
		else {
			leftx += deltaleftx;
			lefta += deltalefta;
			leftb += deltaleftb;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 )
				return;
			if ( getrightsection () <= 0 )
				return;
		}
		else
			rightx += deltarightx;
	}
}

void	gouraudtexturetriangle ( screentype *v1, screentype *v2, screentype *v3, word texturemap, byte *shadetable, word destscreen ) {
	long	yratio;
	int	temp, xlongest;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio = ((v2->y - v1->y) << 6) / temp;
	xlongest = ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 )
			return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 )
				return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 )
			return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 )
				return;
		}
	}

	yratio <<= 8;
	word	dgdx, dadx, dbdx;
	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	asm movsx ecx, word ptr [xlongest]
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_COLOR, dgdx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dadx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdx, yratio )
	word destptr = v1->y * 320;

	while ( 1 ) {
		int	width = (rightx >> 6) - (leftx >> 6);

		if ( width > 0 ) {
			word	a, b, g, poffset;

			a = lefta;
			b = leftb;
			g = leftg;
			poffset = destptr + (leftx >> 6);

			// es:di destination, fs texture, cx and dx the texture
			// coordinates, bx the shade
			asm	mov	es, word ptr [destscreen]
			asm	mov	fs, word ptr [texturemap]
			asm	mov	di, word ptr [poffset]
			asm	mov	cx, word ptr [a]
			asm	mov	dx, word ptr [b]
			asm	mov	bx, word ptr [g]
		gouraud_pixel:
			// texel = texture[a_hi * 256 + b_hi]
			asm	mov	ax, cx
			asm	mov	al, dh
			asm	mov	si, ax
			asm	mov	al, fs:[si]
			// pixel = shadetable[texel * 128 + g_hi]
			asm	xor	ah, ah
			asm	shl	ax, 7
			asm	add	al, bh
			asm	adc	ah, 0
			asm	add	ax, word ptr [shadetable]
			asm	mov	si, ax
			asm	mov	al, [si]
			asm	stosb
			// a += dadx; b += dbdx; g += dgdx;
			asm	add	cx, word ptr [dadx]
			asm	add	dx, word ptr [dbdx]
			asm	add	bx, word ptr [dgdx]
			asm	dec	word ptr [width]
			asm	jnz	gouraud_pixel
			}
		destptr += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 )
				return;
			if ( getleftsection () <= 0 )
				return;
		}
		else {
			leftx += deltaleftx;
			lefta += deltalefta;
			leftb += deltaleftb;
			leftg += deltaleftg;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 )
				return;
			if ( getrightsection () <= 0 )
				return;
		}
		else
			rightx += deltarightx;
	}
}

void	envirotexturetriangle ( screentype *v1, screentype *v2, screentype *v3, word enviromap, word texturemap, byte *shadetable, word destscreen ) {
	long	yratio;
	int	temp, xlongest;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio = ((v2->y - v1->y) << 6) / temp;
	xlongest = ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 ) return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 ) return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 ) return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 ) return;
		}
	}

	yratio <<= 8;
	word	dadx, dbdx, dudx, dvdx;
	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	asm movsx ecx, word ptr [xlongest]
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dadx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_U, dudx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_V, dvdx, yratio )
	word destptr = v1->y * 320;

	while ( 1 ) {
		int	width = (rightx >> 6) - (leftx >> 6);

		if ( width > 0 ) {
			word u, v, a, b, poffset;
			byte	shade;

			a = lefta;
			b = leftb;
			u = leftu;
			v = leftv;
			poffset = destptr + (leftx >> 6);

			// es:di destination, fs texture, gs environment map, cx and dx
			// the texture coordinates, bx and v the environment map's
			asm	mov	es, word ptr [destscreen]
			asm	mov	fs, word ptr [texturemap]
			asm	mov	gs, word ptr [enviromap]
			asm	mov	di, word ptr [poffset]
			asm	mov	cx, word ptr [a]
			asm	mov	dx, word ptr [b]
			asm	mov	bx, word ptr [u]
		envirotexture_pixel:
			// shade = enviromap[u_hi * 256 + v_hi]
			asm	mov	ax, bx
			asm	mov	al, byte ptr [v+1]
			asm	mov	si, ax
			asm	mov	al, gs:[si]
			asm	mov	byte ptr [shade], al
			// texel = texture[a_hi * 256 + b_hi]
			asm	mov	ax, cx
			asm	mov	al, dh
			asm	mov	si, ax
			asm	mov	al, fs:[si]
			// pixel = shadetable[texel * 128 + shade]
			asm	xor	ah, ah
			asm	shl	ax, 7
			asm	add	al, byte ptr [shade]
			asm	adc	ah, 0
			asm	add	ax, word ptr [shadetable]
			asm	mov	si, ax
			asm	mov	al, [si]
			asm	stosb
			// a += dadx; b += dbdx; u += dudx; v += dvdx;
			asm	add	cx, word ptr [dadx]
			asm	add	dx, word ptr [dbdx]
			asm	add	bx, word ptr [dudx]
			asm	mov	ax, word ptr [dvdx]
			asm	add	word ptr [v], ax
			asm	dec	word ptr [width]
			asm	jnz	envirotexture_pixel
			}
		destptr += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 ) return;
			if ( getleftsection () <= 0 ) return;
		}
		else {
			leftx += deltaleftx;
			leftu += deltaleftu;
			leftv += deltaleftv;
			lefta += deltalefta;
			leftb += deltaleftb;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 ) return;
			if ( getrightsection () <= 0 ) return;
		}
		else {
			rightx += deltarightx;
		}
	}
}

void	envirotexturebumptriangle ( screentype *v1, screentype *v2, screentype *v3, word enviromap, word texturemap, word bumpmap, byte *shadetable, word destscreen ) {
	long	xratio, yratio;
	int	xlongest, ylongest, temp;
	word	dudx, dudy, dvdx, dvdy, dadx, dady, dbdx, dbdy;
	word	lineoffset, pixeloffset;
	int	linewidth;

	// sort the triangle so that v1 is the top and v3 is the bottom
	if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the length of the longest horizontal scanline
	temp = (v3->y - v1->y);
	if ( temp == 0 ) return;
	yratio	= ((v2->y - v1->y) << 6) / temp;
	xlongest	= ((v1->x - v2->x) << 6) + (v3->x - v1->x) * (int)yratio;
	if ( xlongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( xlongest < 0 ) {
		if ( xlongest > -64 ) xlongest = -64;
	}
	else {
		if ( xlongest <  64 ) xlongest =  64;
	}

	if ( xlongest < 0 ) {
		rightarray[0] = v3;
		rightarray[1] = v2;
		rightarray[2] = v1;
		rightsection = 2;
		leftarray[0] = v3;
		leftarray[1] = v1;
		leftsection = 1;

		if ( getleftsection () <= 0 ) return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 ) return;
		}
	}
	else {
		leftarray[0] = v3;
		leftarray[1] = v2;
		leftarray[2] = v1;
		leftsection = 2;
		rightarray[0] = v3;
		rightarray[1] = v1;
		rightsection = 1;

		if ( getrightsection () <= 0 ) return;
		if ( getleftsection () <= 0 ) {
			leftsection--;
			if ( getleftsection () <= 0 ) return;
		}
	}

	yratio <<= 8;
	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	asm movsx ecx, word ptr [xlongest]
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_U, dudx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_V, dvdx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dadx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdx, yratio )

	lineoffset = v1->y * 320;

	// sort the triangle so that v1 is the left and v3 is the right
	if ( v1->x > v2->x ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	if ( v1->x > v3->x ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	if ( v2->x > v3->x ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }

	// determine the lenght of the longest vertical scanline
	temp = (v3->x - v1->x);
	if ( temp == 0 ) return;
	xratio = ((v2->x - v1->x) << 6) / temp;
	ylongest = ((v1->y - v2->y) << 6) + (v3->y - v1->y) * (int)xratio;
	if ( ylongest == 0 ) return;

	// increase the magnitude to prevent overflow
	if ( ylongest < 0 ) {
		if ( ylongest > -64 ) ylongest = -64;
	}
	else {
		if ( ylongest <  64 ) ylongest =  64;
	}

	xratio <<= 8;

	asm mov si, word ptr [v1]
	asm mov di, word ptr [v2]
	asm mov bx, word ptr [v3]
	asm movsx ecx, word ptr [ylongest]
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dady, xratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdy, xratio )

	while ( 1 ) {
		byte	index, texel, shade;
		word	a1, a2, b1, b2;
		int	ah1,ah2,bh1,bh2;
		word	u, v, a, b;
		int bmpu, bmpv;

		linewidth = (rightx >> 6) - (leftx >> 6);

		if ( linewidth > 0 ) {

			u = leftu;	v = leftv;
			a = lefta;	b = leftb;

			pixeloffset = lineoffset + (leftx >> 6);

			do {
				//printf ( "texel -> (%i %i) ", a>>8, b>>8 );
				a1 = ((a + dadx) & 0xff00) + ((b + dbdx) >> 8);
				a2 = ((a - dadx) & 0xff00) + ((b - dbdx) >> 8);
				b1 = ((a + dady) & 0xff00) + ((b + dbdy) >> 8);
				b2 = ((a - dady) & 0xff00) + ((b - dbdy) >> 8);

				asm	xor	ax, ax
				asm	mov	es, [bumpmap]
				// the horizontal bumpmapping
				asm	mov	di, [a1]
				asm	mov	al, es:[di]
				asm	mov   [ah1], ax
				asm	mov	di, [a2]
				asm	mov	al, es:[di]
				asm	mov   [ah2], ax
				// the vertical bumpmapping
				asm	mov	di, [b1]
				asm	mov	al, es:[di]
				asm	mov   [bh1], ax
				asm	mov	di, [b2]
				asm	mov	al, es:[di]
				asm	mov   [bh2], ax

				bmpu = ah1 - ah2 + (u >> 8);
				bmpv = bh1 - bh2 + (v >> 8);

				if ( bmpu >= 0 && bmpu < 256 && bmpv >= 0 && bmpv < 256 ) {
					asm	mov	es, word ptr [texturemap]
					asm	mov	ah, byte ptr [a+1]
					asm	mov	al, byte ptr [b+1]
					asm	mov	di, ax
					asm	mov	al, es:[di]
					asm	mov	[texel], al

					asm	mov	es, word ptr [enviromap]
					asm	mov	ah, byte ptr [bmpu]
					asm	mov	al, byte ptr [bmpv]
					asm	mov	di, ax
					asm	mov	al, es:[di]
					asm	mov	[shade], al

					index = shadetable[texel*128 + shade];

					asm	mov	es, [destscreen]
					asm	mov	di, [pixeloffset]
					asm	mov	al, [index]
					asm	mov	es:[di], al
				}
				else {
					asm	mov	es, [destscreen]
					asm	mov	di, [pixeloffset]
					asm	xor	al, al
					asm	mov	es:[di], al
				}
				pixeloffset ++;
				a += dadx;
				b += dbdx;
				u += dudx;
				v += dvdx;
				linewidth --;
			} while ( linewidth > 0 );
		}

		lineoffset += 320;

		if ( -- leftsectionheight <= 0 ) {
			if ( -- leftsection <= 0 ) return;
			if ( getleftsection () <= 0 ) return;
		}
		else {
			leftx += deltaleftx;
			leftu += deltaleftu;
			leftv += deltaleftv;
			lefta += deltalefta;
			leftb += deltaleftb;
		}

		if ( -- rightsectionheight <= 0 ) {
			if ( -- rightsection <= 0 ) return;
			if ( getrightsection () <= 0 ) return;
		}
		else {
			rightx += deltarightx;
		}
	}
}

#endif	//	__POLYGONS_H__
