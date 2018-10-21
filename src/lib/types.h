//
// Retro programming in Borland C++ 3.1
//
#ifndef	__TYPES_H__
#define	__TYPES_H__

#pragma	inline
asm	.486p

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>

#define	MAX_VERTEX	980
#define	MAX_FACET	1620
#define	MAX_ANGLE	512
#define	MAX_DELTA	5

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

typedef	unsigned int	boolean;
typedef	unsigned char	byte;
typedef	unsigned int	word;
typedef	unsigned long	dword;
typedef	float		real;
enum			boolean_values { false = 0, true = 1 };

extern		word far	*bin_ptr;
extern		word		facetorder;
extern		int		bincount[80];

typedef struct {
	int	a, b;
}	texturetype;

typedef struct {
	int	x, y, z;
}	vectortype;

typedef struct {
	word	a, b, c;
}	facettype;

typedef struct {
	int	x, y, z;
	int	a, b;
	int	u, v, uvz;
	int	color;
}	screentype;

typedef struct {
	byte	r, g, b;
}	colortype;

typedef struct {
	boolean		rotation;
	int		xrotation, yrotation, zrotation;
	int		xinc, yinc, zinc;
	word		totalvertices, totalfacets;
	int		zdepth;
	vectortype	objectvertex[MAX_VERTEX];
	vectortype      objectnormal[MAX_VERTEX];
	screentype	screenvertex[MAX_VERTEX];
	facettype	objectfacet[MAX_FACET];
}	objecttype;

typedef struct {
	int 	x, y, z;
	int	u, v, uvz;
	int	xrot, yrot;
	int	xinc, yinc;
}	lighttype;

#endif	//	__TYPES_H__
