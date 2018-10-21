//
// Retro programming in Borland C++ 3.1
//
#ifndef	__MAKESHAD_H__
#define	__MAKESHAD_H__

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>

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

typedef struct {
	byte	r, g, b;
}	colortype;

void	createbasecolors ( void );
void	createshadetable ( void );
void	shrinkcolorcube ( colortype *min, colortype *max );
void	determinedivision ( colortype min, colortype max, int longest, colortype *minsplit, colortype *maxsplit );
void	processcolorcube ( colortype min, colortype max, int level );
int	colormatch ( colortype basecolor );

byte		shadetable[NUM_BASECOLORS];
colortype	basecolor[NUM_BASECOLORS];
colortype	optimalpal[256];
colortype	texturepal[32];
int		numbasecolors;
int		colorcount[64];
int		palcount;
//int		match[256];
char		texturepalfile[]	= "mask.pal";
char		destshadefile[]		= "mask.sht";
FILE		*f;

void	main (void) {
	if ( ( f = fopen ( texturepalfile, "rb" ) ) == NULL ) printf ( "*** error main #1 ***" );
	fread ( texturepal, sizeof ( colortype ), 32, f );
	fclose ( f );

	createbasecolors ();
	createshadetable ();

	if ( ( f = fopen ( destshadefile, "wb" ) ) == NULL ) printf ( "*** error main #2 ***" );
	fwrite ( optimalpal, sizeof ( colortype ), 256, f );
	fwrite ( shadetable, sizeof ( byte ), NUM_BASECOLORS, f );
	fclose ( f );
}

real	phongillumination ( real diffusecolor, real specularcolor,
					real lightcolor,  real ambientcolor, real theta )
{
	real	phongcolor;

	phongcolor =  ( K_DIFFUSE * diffusecolor * cos( theta ));
	if ( theta < M_PI / 4 )
	{
		phongcolor += ( K_SPECULAR * specularcolor * pow( cos( theta * 2 ), K_FALLOFF ));
	}
	phongcolor *= ( K_ATTENUATION * lightcolor );
	phongcolor += ( ambientcolor * K_AMBIENT * diffusecolor );

	// clip phong color to between 0.0 and 1.0
	if ( phongcolor > 1.0 )
		phongcolor = 1.0;
	else if ( phongcolor < 0.0 )
		phongcolor = 0.0;

	return phongcolor;
}

void	createbasecolors ( void )
{
	int	palcolor, shade;
	real	incedent;
	colortype	tclr, temp;

	numbasecolors = 0;

	for ( palcolor = 0; palcolor < 32; palcolor ++ ) {
		tclr = texturepal[palcolor];

		if ( tclr.r > 63 ) tclr.r = 63;
		if ( tclr.g > 63 ) tclr.g = 63;
		if ( tclr.b > 63 ) tclr.b = 63;

		for ( shade = 0; shade < 128; shade ++ ) {

			incedent = ( (real)(128 - ( shade + 1 )) / 128 ) * ( M_PI / 2 );

			temp.r = 63 * phongillumination ( tclr.r/63.0, 1.0, LIGHT_R, AMBIENT_R, incedent );
			temp.g = 63 * phongillumination ( tclr.g/63.0, 1.0, LIGHT_G, AMBIENT_G, incedent );
			temp.b = 63 * phongillumination ( tclr.b/63.0, 1.0, LIGHT_B, AMBIENT_B, incedent );

			basecolor[numbasecolors] = temp;
			numbasecolors++;
			}
		}
}

void	createshadetable ( void )
{
	colortype min = { 0, 0, 0 }, max = { 64, 64, 64 };
	int	index;

	palcount = 0;

	processcolorcube ( min, max, 8 );

//	for ( index = 0; index < 256; index ++ )
//		match[index] = 0;

	for ( index = 0; index < numbasecolors; index ++ ) {
		shadetable[index] = colormatch ( basecolor[index] );
//		match[shadetable[index]]++;
		}

//	for ( index = 0; index < 256; index ++ )
//		printf ( "palette color %i used %i times\n", index, match[index] );

//	exit ( 0 );
}


int	colormatch ( colortype basecolor )
{
	int			index, match, distance, paldistance;
	int			deltar, deltag, deltab;
	colortype	palcolor;

	match = 0;
	distance = 12000;

	for ( index = 0; index < 256; index ++ ) {
		palcolor = optimalpal[index];

		// what is the distance to this color
		deltar = ((int)palcolor.r - (int)basecolor.r);
		deltag = ((int)palcolor.g - (int)basecolor.g);
		deltab = ((int)palcolor.b - (int)basecolor.b);

		paldistance = deltar * deltar + deltag * deltag + deltab * deltab;

		// is this distance shorter than the current match
		if ( paldistance < distance ) {
			distance = paldistance;
			match = index;
			}
		}

	palcolor = optimalpal[match];
//	printf ( "base color (%i %i %i) - match (%i %i %i) - distance (%i)\n",
  //							basecolor.r, basecolor.g, basecolor.b,
	 //						palcolor.r, palcolor.g, palcolor.b, distance );

	return match;
}

void	shrinkcolorcube ( colortype *min, colortype *max )
{
	colortype	color, newmin, newmax;
	int			numcolors, index;

	newmin = *max;
	newmax = *min;
	numcolors = 0;

//	printf ( "Shrink " );

	for ( index = 0; index < numbasecolors; index ++ )
	{
		color = basecolor[index];

		// is it inside the color cube?
		if (	color.r >= min->r && color.r < max->r &&
				color.g >= min->g && color.g < max->g &&
				color.b >= min->b && color.b < max->b ) {

			// i'm counting the colors in the cube
			numcolors++;

			// does this expand out boundaries?
			if ( color.r < newmin.r ) newmin.r = color.r;
			if ( color.g < newmin.g ) newmin.g = color.g;
			if ( color.b < newmin.b ) newmin.b = color.b;

			// does this expand out boundaries?
			if ( color.r >= newmax.r ) newmax.r = color.r + 1;
			if ( color.g >= newmax.g ) newmax.g = color.g + 1;
			if ( color.b >= newmax.b ) newmax.b = color.b + 1;
			}
		}

//	printf ( "change (%i %i %i) - (%i %i %i) numcolors (%i)\n",
/*								newmin.r - min->r, newmin.g - min->g,
								newmin.b - min->b, max->r - newmax.r,
								max->g - newmax.g, max->b - newmax.b,
								numcolors );
  */
	*min = newmin;
	*max = newmax;
}

void	determinedivision ( colortype min, colortype max, int longest,
					 colortype *minsplit, colortype *maxsplit )
{
	colortype	color;
	int			numcolors, index;
	int			mediancolor, mediancomp;

	numcolors = 0;

//	printf ( "Median " );

	// clear count array...
	for ( index = 0; index < 64; index ++ )
		colorcount[index] = 0;

	for ( index = 0; index < numbasecolors; index ++ )
	{
		color = basecolor[index];

		// is it inside the color cube?
		if (	color.r >= min.r && color.r < max.r &&
				color.g >= min.g && color.g < max.g &&
				color.b >= min.b && color.b < max.b ) {

			// i'm counting the colors in the cube
			numcolors++;

			switch ( longest ) {
				case COLOR_RED:
					colorcount[color.r]++;
					break;
				case COLOR_GREEN:
					colorcount[color.g]++;
					break;
				case COLOR_BLUE:
					colorcount[color.b]++;
					break;
				}
			}
		}

//	printf ( "colors (%i) ", numcolors );

	// how many colors until the median
	mediancolor = numcolors / 2;

//	printf ( "mcount (%i) ", mediancolor );

	// determine at what color is the median reached
	for ( index = 0; index < 64; index ++ ) {
		mediancolor -= colorcount[index];

		if ( mediancolor <= 0 ) {
			mediancomp = index;
			break;
			}
		}
	mediancomp += 1;
//	printf ( "median (%i)\n", mediancomp );

	*minsplit = max;
	*maxsplit = min;

	switch ( longest ) {
		case COLOR_RED:
			minsplit->r = mediancomp;
			maxsplit->r = mediancomp;
			break;
		case COLOR_GREEN:
			minsplit->g = mediancomp;
			maxsplit->g = mediancomp;
			break;
		case COLOR_BLUE:
			minsplit->b = mediancomp;
			maxsplit->b = mediancomp;
			break;
		}
}

void	processcolorcube ( colortype min, colortype max, int level )
{
	int	deltar, deltag, deltab, longest;
	colortype	color, minsplit, maxsplit;

//	printf ( "incoming (%i) (%i %i %i) - (%i %i %i)\n",
  //				level, min.r, min.g, min.b, max.r, max.g, max.b );

	// shrink the color cube to contain just the used colors
	shrinkcolorcube ( &min, &max );

//	printf ( "skrunk cube (%i %i %i) - (%i %i %i)\n",
  //				min.r, min.g, min.b, max.r, max.g, max.b );

	// find the length of the sides of the color cube
	deltar = max.r - min.r;
	deltag = max.g - min.g;
	deltab = max.b - min.b;

//	printf ( "deltas (%i %i %i)", deltar, deltag, deltab );

	// if this is the last level then stop
	if ( level == 0 ) {
		color.r = min.r + ( deltar / 2 );
		color.g = min.g + ( deltag / 2 );
		color.b = min.b + ( deltab / 2 );

		if ( color.r > 63 ) color.r = 63;
		if ( color.g > 63 ) color.g = 63;
		if ( color.b > 63 ) color.b = 63;

		optimalpal[palcount] = color;
		palcount++;

	 //	printf ( "color selected (%i) (%i %i %i)\n", palcount, color.r, color.g, color.b );
		}
	else {
		// or subdivide the cube...

		// determine which size is longest
		if ( deltab >= deltar && deltab >= deltag )
			longest = COLOR_BLUE;
		else if ( deltar >= deltag && deltar >= deltab )
			longest = COLOR_RED;
		else if ( deltag >= deltar && deltag >= deltab )
			longest = COLOR_GREEN;

		// split the color cube at the median point on the longest side
		determinedivision ( min, max, longest, &minsplit, &maxsplit );

//		printf ( " dividing (%i)\n", level );

//		printf ( "upper cube (%i %i %i) - (%i %i %i)\n",
  //				min.r, min.g, min.b, minsplit.r, minsplit.g, minsplit.b );
	 //	printf ( "lower cube (%i %i %i) - (%i %i %i)\n",
		//		maxsplit.r, maxsplit.g, maxsplit.b, max.r, max.g, max.b );

		// reduce the level counter
		level--;

		// continue the process
		processcolorcube ( min, minsplit, level );
		processcolorcube ( maxsplit, max, level );
		}
}

#endif	//	__MAKESHAD_H__
