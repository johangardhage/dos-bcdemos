//
// Retro programming in Borland C++ 3.1
//
// Environment shading demonstration
//
#include "lib/types.h"
#include "lib/engine.h"
#include "lib/graphics.h"
#include "lib/misc.h"
#include "lib/polygons.h"

objecttype	*objectptr;
colortype       palette[256];
char		filename[]	= "assets/chrmface.3d_";
char		metalfile[]	= "assets/metal.raw";
char		palettefile[]	= "assets/metal.pal";
word		vscreen, enviromap;

void	drawfacets ( void ) {
	screentype	*v1, *v2, *v3;
	int		bin, bintotal, binfacet, facet;

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

			envirotriangle ( v1, v2, v3, enviromap, vscreen );
		}
		bincount[bin] = 0;
	}
}

void	drawframe ( void ) {
	cls (vscreen);

	calcangles (objectptr);
	initmatrix ();
	rotatevertices (objectptr);
	rotatenormalsenviro (objectptr);
	sortfacets (objectptr);
	drawfacets ();

	copyscreen ( vscreen, 0xa000 );
}

void	main ( void ) {
	dword	startcount, endcount, frames;

	clrscr ();
	printf ( "\n" );
	printf ( "Environment shading demonstration\n\n" );
	printf ( "  Runtime controls:\n" );
	printf ( "    '+' and '-' to change object distance\n" );
	printf ( "    '8' and '2' to change object x rotation\n" );
	printf ( "    '4' and '6' to change object y rotation\n" );
	printf ( "    '7' and '9' to change object z rotation\n" );
	printf ( "    '5' to center object\n\n" );
	printf ( "    'spacebar' to toggle rotations\n" );
	printf ( "    'q' or 'esc' to quit\n\n" );
	printf ( "  Press a key to start demonstration" );
	getch ();

	objectptr = initobject (filename);
	enviromap = initpicture (metalfile);
	initpalette (palettefile, palette);
	initsincos ();
	facetorder = initvirtual (65536);

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
