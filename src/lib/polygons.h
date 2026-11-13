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
int		dudx, dvdx, leftu, leftv, deltaleftu, deltaleftv;
int		dadx, dady, dbdx, dbdy, lefta, leftb, deltalefta, deltaleftb;
int		dgdx, leftg, deltaleftg;
int		leftx, rightx;
word		deltaleftx, deltarightx;
word		dady2, dbdy2, dadx2, dbdx2, step1a, step1b, step2a, step2b;
dword		bump_uv, bump_leftuv;	// u in the low word, v in the high word
word		bump_lineoffset, bump_shadetable, bump_texturemap;

int	getrightsection ( void ) {
	// screentype *v1 = rightarray[rightsection];
	// screentype *v2 = rightarray[rightsection - 1];
	asm	mov	bx, word ptr [rightsection]
	asm	shl	bx, 1
	asm	mov	si, word ptr [rightarray + bx]
	asm	mov	di, word ptr [rightarray + bx - 2]

	// int height = v2->y - v1->y;
	// if ( height == 0 ) return 0;
	// rightsectionheight = height;
	asm	mov	ax, word ptr [di + 2]
	asm	sub	ax, word ptr [si + 2]
	asm	jz	right_done
	asm	mov	bx, ax
	asm	mov	word ptr [rightsectionheight], bx

	// deltarightx = ( ( v2->x - v1->x ) << 6 ) / height;
	// rightx = v1->x << 6;
	asm	mov	ax, word ptr [di]
	asm	sub	ax, word ptr [si]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	mov	word ptr [deltarightx], ax
	asm	mov	ax, word ptr [si]
	asm	shl	ax, 6
	asm	mov	word ptr [rightx], ax

	asm	mov	ax, bx
	right_done:
	return _AX;
}

int	getleftsection ( void ) {
	// screentype *v1 = leftarray[leftsection];
	// screentype *v2 = leftarray[leftsection - 1];
	asm	mov	bx, word ptr [leftsection]
	asm	shl	bx, 1
	asm	mov	si, word ptr [leftarray + bx]
	asm	mov	di, word ptr [leftarray + bx - 2]

	// int height = v2->y - v1->y;
	// if ( height == 0 ) return 0;
	// leftsectionheight = height;
	asm	mov	ax, word ptr [di + 2]
	asm	sub	ax, word ptr [si + 2]
	asm	jnz	left_calc
	return 0;
	left_calc:
	asm	mov	bx, ax
	asm	mov	word ptr [leftsectionheight], bx

	// deltaleftx = ( ( v2->x - v1->x ) << 6 ) / height;
	// leftx = v1->x << 6;
	asm	mov	ax, word ptr [di]
	asm	sub	ax, word ptr [si]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	mov	word ptr [deltaleftx], ax
	asm	mov	ax, word ptr [si]
	asm	shl	ax, 6
	asm	mov	word ptr [leftx], ax

	// texture coordinates
	// deltalefta = ( ( ( v2->a - v1->a ) << 6 ) / height ) << 2;
	// lefta = v1->a << 8;
	asm	mov	ax, word ptr [di + 6]
	asm	sub	ax, word ptr [si + 6]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltalefta], ax
	asm	mov	ax, word ptr [si + 6]
	asm	shl	ax, 8
	asm	mov	word ptr [lefta], ax

	// deltaleftb = ( ( ( v2->b - v1->b ) << 6 ) / height ) << 2;
	// leftb = v1->b << 8;
	asm	mov	ax, word ptr [di + 8]
	asm	sub	ax, word ptr [si + 8]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltaleftb], ax
	asm	mov	ax, word ptr [si + 8]
	asm	shl	ax, 8
	asm	mov	word ptr [leftb], ax

	// phong coordinates
	// deltaleftu = ( ( ( v2->u - v1->u ) << 6 ) / height ) << 2;
	// leftu = v1->u << 8;
	asm	mov	ax, word ptr [di + 10]
	asm	sub	ax, word ptr [si + 10]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltaleftu], ax
	asm	mov	ax, word ptr [si + 10]
	asm	shl	ax, 8
	asm	mov	word ptr [leftu], ax

	// deltaleftv = ( ( ( v2->v - v1->v ) << 6 ) / height ) << 2;
	// leftv = v1->v << 8;
	asm	mov	ax, word ptr [di + 12]
	asm	sub	ax, word ptr [si + 12]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltaleftv], ax
	asm	mov	ax, word ptr [si + 12]
	asm	shl	ax, 8
	asm	mov	word ptr [leftv], ax

	// gouraud coordinates
	// deltaleftg = ( ( ( v2->color - v1->color ) << 6 ) / height ) << 2;
	// leftg = v1->color << 8;
	asm	mov	ax, word ptr [di + 16]
	asm	sub	ax, word ptr [si + 16]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltaleftg], ax
	asm	mov	ax, word ptr [si + 16]
	asm	shl	ax, 8
	asm	mov	word ptr [leftg], ax

	asm	mov	ax, bx
	return _AX;
}

int	getleftsectionuvab ( void ) {
	// screentype *v1 = leftarray[leftsection];
	// screentype *v2 = leftarray[leftsection - 1];
	asm	mov	bx, word ptr [leftsection]
	asm	shl	bx, 1
	asm	mov	si, word ptr [leftarray + bx]
	asm	mov	di, word ptr [leftarray + bx - 2]

	// int height = v2->y - v1->y;
	// if ( height == 0 ) return 0;
	// leftsectionheight = height;
	asm	mov	ax, word ptr [di + 2]
	asm	sub	ax, word ptr [si + 2]
	asm	jnz	left_uvab_calc
	return 0;
	left_uvab_calc:
	asm	mov	bx, ax
	asm	mov	word ptr [leftsectionheight], bx

	// deltaleftx = ( ( v2->x - v1->x ) << 6 ) / height;
	// leftx = v1->x << 6;
	asm	mov	ax, word ptr [di]
	asm	sub	ax, word ptr [si]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	mov	word ptr [deltaleftx], ax
	asm	mov	ax, word ptr [si]
	asm	shl	ax, 6
	asm	mov	word ptr [leftx], ax

	// texture coordinates
	// deltalefta = ( ( ( v2->a - v1->a ) << 6 ) / height ) << 2;
	// lefta = v1->a << 8;
	asm	mov	ax, word ptr [di + 6]
	asm	sub	ax, word ptr [si + 6]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltalefta], ax
	asm	mov	ax, word ptr [si + 6]
	asm	shl	ax, 8
	asm	mov	word ptr [lefta], ax

	// deltaleftb = ( ( ( v2->b - v1->b ) << 6 ) / height ) << 2;
	// leftb = v1->b << 8;
	asm	mov	ax, word ptr [di + 8]
	asm	sub	ax, word ptr [si + 8]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltaleftb], ax
	asm	mov	ax, word ptr [si + 8]
	asm	shl	ax, 8
	asm	mov	word ptr [leftb], ax

	// phong coordinates
	// deltaleftu = ( ( ( v2->u - v1->u ) << 6 ) / height ) << 2;
	// bump_leftuv.u = v1->u << 8;
	asm	mov	ax, word ptr [di + 10]
	asm	sub	ax, word ptr [si + 10]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltaleftu], ax
	asm	mov	ax, word ptr [si + 10]
	asm	shl	ax, 8
	asm	mov	word ptr [bump_leftuv], ax

	// deltaleftv = ( ( ( v2->v - v1->v ) << 6 ) / height ) << 2;
	// bump_leftuv.v = v1->v << 8;
	asm	mov	ax, word ptr [di + 12]
	asm	sub	ax, word ptr [si + 12]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	bx
	asm	shl	ax, 2
	asm	mov	word ptr [deltaleftv], ax
	asm	mov	ax, word ptr [si + 12]
	asm	shl	ax, 8
	asm	mov	word ptr [bump_leftuv+2], ax

	asm	mov	ax, bx
	return _AX;
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
	int	xlongest;

	// sort the triangle so that v1 (si) is the top and v3 (bx) is the bottom
	// if ( v1->y > v2->y ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	// if ( v1->y > v3->y ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	// if ( v2->y > v3->y ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }
	asm	mov	si, word ptr [v1]
	asm	mov	di, word ptr [v2]
	asm	mov	bx, word ptr [v3]
	asm	mov	ax, word ptr [si + 2]
	asm	cmp	ax, word ptr [di + 2]
	asm	jle	sort_y1
	asm	xchg	si, di
	sort_y1:
	asm	mov	ax, word ptr [si + 2]
	asm	cmp	ax, word ptr [bx + 2]
	asm	jle	sort_y2
	asm	xchg	si, bx
	sort_y2:
	asm	mov	ax, word ptr [di + 2]
	asm	cmp	ax, word ptr [bx + 2]
	asm	jle	sort_y3
	asm	xchg	di, bx
	sort_y3:

	// determine the length of the longest horizontal scanline
	// temp = v3->y - v1->y;
	// if ( temp == 0 ) return;
	// yratio = ( ( v2->y - v1->y ) << 6 ) / temp;
	// xlongest = ( ( v1->x - v2->x ) << 6 ) + ( v3->x - v1->x ) * (int)yratio;
	// if ( xlongest == 0 ) return;
	asm	mov	cx, word ptr [bx + 2]
	asm	sub	cx, word ptr [si + 2]
	asm	jz	early_return
	asm	mov	ax, word ptr [di + 2]
	asm	sub	ax, word ptr [si + 2]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	cx
	asm	mov	cx, ax
	asm	mov	ax, word ptr [bx]
	asm	sub	ax, word ptr [si]
	asm	imul	cx
	asm	mov	dx, word ptr [si]
	asm	sub	dx, word ptr [di]
	asm	shl	dx, 6
	asm	add	ax, dx
	asm	jnz	xlong_nonzero
	early_return:
		return;
	xlong_nonzero:
	// increase the magnitude to prevent overflow
	// if ( xlongest < 0 && xlongest > -64 ) xlongest = -64;
	// if ( xlongest >= 0 && xlongest < 64 ) xlongest = 64;
	asm	jns	xlong_pos
	asm	cmp	ax, -64
	asm	jle	xlong_ok
	asm	mov	ax, -64
	asm	jmp	xlong_ok
	xlong_pos:
	asm	cmp	ax, 64
	asm	jge	xlong_ok
	asm	mov	ax, 64
	xlong_ok:
	asm	mov	word ptr [xlongest], ax

	// yratio <<= 8;
	asm	movsx	edx, cx
	asm	shl	edx, 8
	asm	mov	dword ptr [yratio], edx
	asm	movsx	ecx, ax

	TRIANGLE_GRADIENT_ECX ( TRIANGLE_U, dudx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_V, dvdx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dadx, yratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdx, yratio )

	// bump_lineoffset = v1->y * 320;
	asm	mov	ax, word ptr [si + 2]
	asm	mov	dx, ax
	asm	shl	ax, 8
	asm	shl	dx, 6
	asm	add	ax, dx
	asm	mov	word ptr [bump_lineoffset], ax

	// if ( xlongest < 0 ) {
	//     rightarray[0] = v3; rightarray[1] = v2; rightarray[2] = v1; rightsection = 2;
	//     leftarray[0]  = v3; leftarray[1]  = v1; leftsection  = 1;
	// } else {
	//     leftarray[0]  = v3; leftarray[1]  = v2; leftarray[2]  = v1; leftsection  = 2;
	//     rightarray[0] = v3; rightarray[1] = v1; rightsection = 1;
	// }
	asm	cmp	word ptr [xlongest], 0
	asm	jge	setup_pos_xlong
	asm	mov	word ptr [rightarray], bx
	asm	mov	word ptr [rightarray + 2], di
	asm	mov	word ptr [rightarray + 4], si
	asm	mov	word ptr [rightsection], 2
	asm	mov	word ptr [leftarray], bx
	asm	mov	word ptr [leftarray + 2], si
	asm	mov	word ptr [leftsection], 1
	asm	jmp	setup_arrays_done
	setup_pos_xlong:
	asm	mov	word ptr [leftarray], bx
	asm	mov	word ptr [leftarray + 2], di
	asm	mov	word ptr [leftarray + 4], si
	asm	mov	word ptr [leftsection], 2
	asm	mov	word ptr [rightarray], bx
	asm	mov	word ptr [rightarray + 2], si
	asm	mov	word ptr [rightsection], 1
	setup_arrays_done:

	// sort the triangle so that v1 (si) is the left and v3 (bx) is the right
	// if ( v1->x > v2->x ) { screentype *temp_ptr = v1; v1 = v2; v2 = temp_ptr; }
	// if ( v1->x > v3->x ) { screentype *temp_ptr = v1; v1 = v3; v3 = temp_ptr; }
	// if ( v2->x > v3->x ) { screentype *temp_ptr = v2; v2 = v3; v3 = temp_ptr; }
	asm	mov	ax, word ptr [si]
	asm	cmp	ax, word ptr [di]
	asm	jle	sort_x1
	asm	xchg	si, di
	sort_x1:
	asm	mov	ax, word ptr [si]
	asm	cmp	ax, word ptr [bx]
	asm	jle	sort_x2
	asm	xchg	si, bx
	sort_x2:
	asm	mov	ax, word ptr [di]
	asm	cmp	ax, word ptr [bx]
	asm	jle	sort_x3
	asm	xchg	di, bx
	sort_x3:

	// determine the length of the longest vertical scanline
	// temp = v3->x - v1->x;
	// if ( temp == 0 ) return;
	// xratio = ( ( v2->x - v1->x ) << 6 ) / temp;
	// ylongest = ( ( v1->y - v2->y ) << 6 ) + ( v3->y - v1->y ) * (int)xratio;
	// if ( ylongest == 0 ) return;
	asm	mov	cx, word ptr [bx]
	asm	sub	cx, word ptr [si]
	asm	jz	early_return2
	asm	mov	ax, word ptr [di]
	asm	sub	ax, word ptr [si]
	asm	shl	ax, 6
	asm	cwd
	asm	idiv	cx
	asm	mov	cx, ax
	asm	mov	ax, word ptr [bx + 2]
	asm	sub	ax, word ptr [si + 2]
	asm	imul	cx
	asm	mov	dx, word ptr [si + 2]
	asm	sub	dx, word ptr [di + 2]
	asm	shl	dx, 6
	asm	add	ax, dx
	asm	jnz	ylong_nonzero
	early_return2:
		return;
	ylong_nonzero:
	// increase the magnitude to prevent overflow
	// if ( ylongest < 0 && ylongest > -64 ) ylongest = -64;
	// if ( ylongest >= 0 && ylongest < 64 ) ylongest = 64;
	asm	jns	ylong_pos
	asm	cmp	ax, -64
	asm	jle	ylong_ok
	asm	mov	ax, -64
	asm	jmp	ylong_ok
	ylong_pos:
	asm	cmp	ax, 64
	asm	jge	ylong_ok
	asm	mov	ax, 64
	ylong_ok:

	// xratio <<= 8;
	asm	movsx	edx, cx
	asm	shl	edx, 8
	asm	mov	dword ptr [xratio], edx
	asm	movsx	ecx, ax

	TRIANGLE_GRADIENT_ECX ( TRIANGLE_A, dady, xratio )
	TRIANGLE_GRADIENT_ECX ( TRIANGLE_B, dbdy, xratio )

	// Precompute steps for walking (a, b) through (a, b) -> (a + dady, b + dbdy)
	// -> (a - dady, b - dbdy) -> (a - dadx, b - dbdx) -> (a + dadx, b + dbdx):
	// dbdy2 = dbdy * 2; dady2 = dady * 2; dadx2 = dadx * 2; dbdx2 = dbdx * 2;
	// step1a = dady - dadx; step2a = dady + dadx;
	// step1b = dbdy - dbdx; step2b = dbdy + dbdx;
	asm	mov	ax, word ptr [dbdy]
	asm	mov	si, ax
	asm	shl	ax, 1
	asm	mov	word ptr [dbdy2], ax
	asm	mov	cx, word ptr [dady]
	asm	mov	ax, cx
	asm	shl	ax, 1
	asm	mov	word ptr [dady2], ax
	asm	mov	dx, word ptr [dadx]
	asm	mov	ax, dx
	asm	shl	ax, 1
	asm	mov	word ptr [dadx2], ax
	asm	mov	di, word ptr [dbdx]
	asm	mov	ax, di
	asm	shl	ax, 1
	asm	mov	word ptr [dbdx2], ax
	asm	mov	ax, cx
	asm	sub	ax, dx
	asm	mov	word ptr [step1a], ax
	asm	add	cx, dx
	asm	mov	word ptr [step2a], cx
	asm	mov	ax, si
	asm	sub	ax, di
	asm	mov	word ptr [step1b], ax
	asm	add	si, di
	asm	mov	word ptr [step2b], si

	if ( xlongest < 0 ) {
		if ( getleftsectionuvab () <= 0 ) return;
		if ( getrightsection () <= 0 ) {
			rightsection--;
			if ( getrightsection () <= 0 ) return;
		}
	}
	else {
		if ( getrightsection () <= 0 ) return;
		if ( getleftsectionuvab () <= 0 ) {
			leftsection--;
			if ( getleftsectionuvab () <= 0 ) return;
		}
	}

	// bump_shadetable = (word)shadetable;
	// bump_texturemap = texturemap;
	asm	mov	ax, word ptr [shadetable]
	asm	mov	word ptr [bump_shadetable], ax
	asm	mov	ax, word ptr [texturemap]
	asm	mov	word ptr [bump_texturemap], ax

	// Keep all four segment registers loaded across the triangle, and keep
	// packed (dudx, dvdx) step in ebp throughout the scanline and pixel loops:
	// ebp = (long)dudx + ( (long)dvdx << 16 );
	asm	push	ds
	asm	push	bp
	asm	mov	es, word ptr [destscreen]
	asm	mov	fs, word ptr [bumpmap]
	asm	mov	gs, word ptr [enviromap]
	asm	mov	ds, word ptr [bump_texturemap]
	asm	movsx	ebp, word ptr ss:[dudx]
	asm	movzx	eax, word ptr ss:[dvdx]
	asm	shl	eax, 16
	asm	add	ebp, eax
	asm	mov	di, word ptr ss:[leftx]
	asm	mov	cx, word ptr ss:[rightx]
	asm	jmp	scan_loop

	triangle_done:
		asm	pop	bp
		asm	pop	ds
		return;

	// if ( --leftsection <= 0 ) return;
	// if ( getleftsectionuvab () <= 0 ) return;
	left_next_section:
		asm	dec	word ptr ss:[leftsection]
		asm	jle	triangle_done
		asm	mov	ax, ss
		asm	mov	ds, ax
		if ( getleftsectionuvab () <= 0 ) goto triangle_done;
		asm	mov	ds, word ptr [bump_texturemap]
		asm	mov	di, word ptr ss:[leftx]
		asm	jmp	left_advanced

	// if ( --rightsection <= 0 ) return;
	// if ( getrightsection () <= 0 ) return;
	right_next_section:
		asm	dec	word ptr ss:[rightsection]
		asm	jle	triangle_done
		asm	mov	ax, ss
		asm	mov	ds, ax
		if ( getrightsection () <= 0 ) goto triangle_done;
		asm	mov	ds, word ptr [bump_texturemap]
		asm	mov	di, word ptr ss:[leftx]
		asm	mov	cx, word ptr ss:[rightx]
		asm	jmp	scan_loop

	// bump_lineoffset += 320;
	// if ( --leftsectionheight <= 0 ) goto left_next_section;
	// bump_leftuv.u += deltaleftu; bump_leftuv.v += deltaleftv;
	// lefta += deltalefta; leftb += deltaleftb;
	// leftx += deltaleftx;
	scan_advance:
		asm	add	word ptr ss:[bump_lineoffset], 320
		asm	dec	word ptr ss:[leftsectionheight]
		asm	jle	left_next_section
		asm	mov	ax, word ptr ss:[deltaleftu]
		asm	add	word ptr ss:[bump_leftuv], ax
		asm	mov	ax, word ptr ss:[deltaleftv]
		asm	add	word ptr ss:[bump_leftuv+2], ax
		asm	mov	ax, word ptr ss:[deltalefta]
		asm	add	word ptr ss:[lefta], ax
		asm	mov	ax, word ptr ss:[deltaleftb]
		asm	add	word ptr ss:[leftb], ax
		asm	mov	di, word ptr ss:[leftx]
		asm	add	di, word ptr ss:[deltaleftx]
		asm	mov	word ptr ss:[leftx], di
	// if ( --rightsectionheight <= 0 ) goto right_next_section;
	// rightx += deltarightx;
	left_advanced:
		asm	dec	word ptr ss:[rightsectionheight]
		asm	jle	right_next_section
		asm	mov	cx, word ptr ss:[rightx]
		asm	add	cx, word ptr ss:[deltarightx]
		asm	mov	word ptr ss:[rightx], cx

	scan_loop:
		// linewidth = ( rightx >> 6 ) - ( leftx >> 6 );
		// if ( linewidth <= 0 ) goto scan_advance;
		asm	sar	di, 6
		asm	sar	cx, 6
		asm	sub	cx, di
		asm	jle	scan_advance

		// The pixel loop counts in cl, since ch is scratch, so a span is
		// drawn ( linewidth & 0xff ) pixels wide, with 0 meaning 256.

		// bump_uv = bump_leftuv;
		// pixeloffset = bump_lineoffset + ( leftx >> 6 );
		// a = lefta; b = leftb;
		asm	mov	eax, dword ptr ss:[bump_leftuv]
		asm	mov	dword ptr ss:[bump_uv], eax
		asm	add	di, word ptr ss:[bump_lineoffset]
		asm	mov	si, word ptr ss:[lefta]
		asm	mov	dx, word ptr ss:[leftb]
	bump_pixel:
		// texel = texture[( a & 0xff00 ) + ( ( b >> 8 ) & 0xff )];
		asm	mov	bx, si
		asm	mov	bl, dh
		asm	mov	ah, [bx]
		// bumpv = bump[( ( a + dady ) & 0xff00 ) + ( ( ( b + dbdy ) >> 8 ) & 0xff )]
		//       - bump[( ( a - dady ) & 0xff00 ) + ( ( ( b - dbdy ) >> 8 ) & 0xff )]
		//       + ( v >> 8 );
		// if ( bumpv < 0 || bumpv > 255 ) goto bump_v_black;
		asm	add	si, word ptr ss:[dady]
		asm	add	dx, word ptr ss:[dbdy]
		asm	mov	bx, si
		asm	mov	bl, dh
		asm	mov	al, byte ptr ss:[bump_uv+3]
		asm	add	al, fs:[bx]
		asm	sbb	ch, ch
		asm	sub	si, word ptr ss:[dady2]
		asm	sub	dx, word ptr ss:[dbdy2]
		asm	mov	bx, si
		asm	mov	bl, dh
		asm	sub	al, fs:[bx]
		asm	adc	ch, 0
		asm	jnz	bump_v_black
		// bumpu = bump[( ( a + dadx ) & 0xff00 ) + ( ( ( b + dbdx ) >> 8 ) & 0xff )]
		//       - bump[( ( a - dadx ) & 0xff00 ) + ( ( ( b - dbdx ) >> 8 ) & 0xff )]
		//       + ( u >> 8 );
		// if ( bumpu < 0 || bumpu > 255 ) goto bump_black;
		asm	add	si, word ptr ss:[step1a]
		asm	add	dx, word ptr ss:[step1b]
		asm	mov	bx, si
		asm	mov	bl, dh
		asm	mov	ch, fs:[bx]
		asm	add	si, word ptr ss:[dadx2]
		asm	add	dx, word ptr ss:[dbdx2]
		asm	mov	bx, si
		asm	mov	bl, dh
		asm	mov	bh, fs:[bx]
		asm	sub	bh, ch
		asm	sbb	bl, bl
		asm	add	bh, byte ptr ss:[bump_uv+1]
		asm	adc	bl, 0
		asm	jnz	bump_black
		// shade = enviromap[( bumpu << 8 ) + bumpv];
		// pixel = shadetable[( texel << 7 ) + ( shade & 0x7f )];
		// Shades must be below 128, as initphongmap (128) makes them.
		asm	mov	bl, al
		asm	mov	al, gs:[bx]
		asm	shl	al, 1
		asm	shr	ax, 1
		asm	xchg	ax, bx
		asm	add	bx, word ptr ss:[bump_shadetable]
		asm	mov	al, ss:[bx]
	bump_store:
		// screen[pixeloffset++] = pixel;
		// bump_uv += ebp;
		// A carry out of u adds 1 to v (1/256 of a texel) when u wraps.
		asm	stosb
		asm	add	dword ptr ss:[bump_uv], ebp
		// if ( --cl != 0 ) goto bump_pixel;
		asm	dec	cl
		asm	jnz	bump_pixel
		asm	jmp	scan_advance
	// Advance (a, b) from (a - dady, b - dbdy) to (a + dadx, b + dbdx)
	// and write pixel = 0 when bumpv or bumpu is out of range [0..255].
	bump_v_black:
		asm	add	si, word ptr ss:[step2a]
		asm	add	dx, word ptr ss:[step2b]
	bump_black:
		asm	xor	al, al
		asm	jmp	bump_store
}

#endif	//	__POLYGONS_H__
