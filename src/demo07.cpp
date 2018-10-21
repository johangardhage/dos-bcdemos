//
// Retro programming in Borland C++ 3.1
//
// Flat shaded texture mapping demonstration
//
#include "lib/types.h"
#include "lib/engine.h"
#include "lib/graphics.h"
#include "lib/misc.h"
#include "lib/polygons.h"

objecttype	*objectptr;
lighttype	*lightptr;
colortype       palette[256];
char		filename[]	= "assets/mask.3d_";
char		texturefile[]	= "assets/mask.raw";
char		shadefile[]	= "assets/mask.sht";
word		vscreen, texturemap;
byte		shadetable[NUM_BASECOLORS];

void	drawfacets ( void ) {
	screentype	*v1, *v2, *v3;
	int		bin, bintotal, binfacet, facet;
	// this is for calcing colors
	float		x1, y1, z1;
	float		costheta;
	int		color;

	for ( bin = 79; bin >= 0; bin -- ) {

		bintotal = bincount[bin];

		for ( binfacet = 0; binfacet < bintotal; binfacet ++ ) {
			// get the facet to be drawn
			bin_ptr = (word far*)MK_FP ( facetorder, ( bin << 8 ) + ( binfacet << 1 ) );
			facet = *bin_ptr;

			// get the offsets to the vertices of this facet
			v1 = (screentype*)objectptr->objectfacet[facet].a;
			v2 = (screentype*)objectptr->objectfacet[facet].b;
			v3 = (screentype*)objectptr->objectfacet[facet].c;

			// calculate facet normal
			x1=(v1->u + v2->u + v3->u);
			y1=(v1->v + v2->v + v3->v);
			z1=(v1->uvz + v2->uvz + v3->uvz);

			// calculate facet color (196608 = 256*256*3)
			costheta = (x1 * lightptr->u + y1 * lightptr->v + z1 * lightptr->uvz) / 196608;
			if (costheta < 0) color = abs(costheta * 128);
			else color = 1;
			if (color > 127) color = 127;
			if (color < 1) color = 1;

			flattexturetriangle ( v1, v2, v3, texturemap, shadetable, color, vscreen );
		}
		bincount[bin] = 0;
	}
}

void	drawframe ( void ) {
	cls (vscreen);

	calcangles (objectptr);
	initmatrix ();
	rotatevertices (objectptr);
	rotatenormals (objectptr);
	sortfacets (objectptr);
	rotatelight (lightptr);
	drawfacets ();

	copyscreen ( vscreen, 0xa000 );
}

void	main ( void ) {
	dword	startcount, endcount, frames;

	clrscr ();
	printf ( "\n" );
	printf ( "Flat shaded texture mapping demonstration\n\n" );
	printf ( "  Runtime controls:\n" );
	printf ( "    '+' and '-' to change object distance\n" );
	printf ( "    '8' and '2' to change object x rotation\n" );
	printf ( "    '4' and '6' to change object y rotation\n" );
	printf ( "    '7' and '9' to change object z rotation\n" );
	printf ( "    '5' to center object\n\n" );
	printf ( "    'u' and 'j' to change light x rotation\n" );
	printf ( "    'k' and 'l' to change light y rotation\n" );
	printf ( "    'o' to center lightsource\n\n" );
	printf ( "    'spacebar' to toggle rotations\n" );
	printf ( "    'q' or 'esc' to quit\n\n" );
	printf ( "  Press a key to start demonstration" );
	getch ();

	objectptr = initobject (filename);
	texturemap = initpicture (texturefile);
	initshadetable (shadefile, shadetable, palette);
	initsincos ();
	facetorder = initvirtual (65536);
	lightptr = initlight (0, 0, 10, 0, 0);

	objectptr->rotation = true;
	objectptr->xrotation = 128;
	objectptr->yrotation = 0;
	objectptr->zrotation = 0;
	objectptr->zdepth = 200;
	objectptr->xinc = 1;
	objectptr->yinc = 1;
	objectptr->zinc = 1;

	vscreen = initvirtual (65536);
	setgfxmode(0x13);
	introsequence ();

	frames = 0;
	startcount = gettickcount ();

	boolean leave = false;
	do {
		if ( objectptr->rotation ) {
			drawframe ();
			frames ++;
		}

		if ( kbhit () )
			switch ( getch () ) {
				// do you want to quit?
				case 'q':
				case 27 :
					leave = true;
					break;
				// toggle rotation
				case ' ':
					objectptr->rotation = ( objectptr->rotation == false );
					break;

				// increase x rotation
				case '2' :
					objectptr->xinc ++;
					if ( objectptr->xinc > MAX_DELTA ) objectptr->xinc = MAX_DELTA;
					break;
				// decrease x rotation
				case '8' :
					objectptr->xinc --;
					if ( objectptr->xinc < -MAX_DELTA ) objectptr->xinc = -MAX_DELTA;
					break;
				// increase y rotation
				case '6' :
					objectptr->yinc ++;
					if ( objectptr->yinc > MAX_DELTA ) objectptr->yinc = MAX_DELTA;
					break;
				// decrease y rotation
				case '4' :
					objectptr->yinc --;
					if ( objectptr->yinc < -MAX_DELTA ) objectptr->yinc = -MAX_DELTA;
					break;
				// increase z rotation
				case '9' :
					objectptr->zinc ++;
					if ( objectptr->zinc > MAX_DELTA ) objectptr->zinc = MAX_DELTA;
					break;
				// decrease z rotation
				case '7' :
					objectptr->zinc --;
					if ( objectptr->zinc < -MAX_DELTA ) objectptr->zinc = -MAX_DELTA;
					break;
				// center object
				case '5' :
					objectptr->rotation = true;
					objectptr->xrotation = 128;
					objectptr->yrotation = 0;
					objectptr->zrotation = 0;
					objectptr->zdepth = 200;
					objectptr->xinc = 0;
					objectptr->yinc = 0;
					objectptr->zinc = 0;
					break;
				// push the object father away
				case '+' :
					objectptr->zdepth += 10;
					if ( objectptr->zdepth > 2000 ) objectptr->zdepth = 2000;
					break;
				// bring the object closer
				case '-' :
					objectptr->zdepth -= 10;
					if ( objectptr->zdepth < 200 ) objectptr->zdepth = 200;
					break;

				// increase x rotation (light)
				case 'j' :
					lightptr->xinc ++;
					if ( lightptr->xinc > MAX_DELTA ) lightptr->xinc = MAX_DELTA;
					break;
				// decrease x rotation (light)
				case 'u' :
					lightptr->xinc --;
					if ( lightptr->xinc < -MAX_DELTA ) lightptr->xinc = -MAX_DELTA;
					break;
				// increase y rotation (light)
				case 'l' :
					lightptr->yinc ++;
					if ( lightptr->yinc > MAX_DELTA ) lightptr->yinc = MAX_DELTA;
					break;
				// decrease y rotation (light)
				case 'k' :
					lightptr->yinc --;
					if ( lightptr->yinc < -MAX_DELTA ) lightptr->yinc = -MAX_DELTA;
					break;
				// center light (light)
				case 'o' :
					lightptr->xrot = 0;
					lightptr->yrot = 0;
					lightptr->xinc = 0;
					lightptr->yinc = 0;
					break;
			}
	} while ( !leave );

	endcount = gettickcount ();
	exitsequence ();
	setgfxmode(0x03);

	clrscr ();
	printf ("\n");
	printf ("  Runtime statistics:\n");
//	printf ("    Vertices in object        - %i\n", totalvertices );
//	printf ("    Faces in object           - %i\n", totalfacets );
	printf ("    Average Frames Per Second - %f\n\n", (frames*18.2)/(endcount-startcount) );
}
