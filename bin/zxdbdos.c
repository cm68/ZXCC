#include "zxcc.h"
#include "zxbdos.h"
#include "zxdbdos.h"

/* This file used to deal with all disc-based BDOS calls. 
  Now the calls have been moved into libcpmredir, it's a bit empty round
  here. 

   ZXCC does a few odd things when searching, to make Hi-Tech C behave
   properly.
*/


/* If a file could not be found on the default drive, try again on a "search"
  drive (A: for .COM files, B: for .LIB and .OBJ files) */
  
int fcbforce(byte *fcb, byte *odrv)
{
	byte drive;
	char nam[9];
	char typ[4];	
	int n;

	for (n = 0; n < 8; n++) nam[n] = fcb[n+1] & 0x7F;
	nam[8] = 0;
	for (n = 0; n < 3; n++) typ[n] = fcb[n+9] & 0x7F;
	typ[3] = 0;

	drive = 0;
	if (*fcb) return 0;	/* not using default drive */

	/* Microsoft BASIC compiler run-time */
	if (!strcmp(nam,"BCLOAD  ") && !strcmp(typ, "   ")) drive = 2;

	/* HI-TECH C options help file */
	if (!strcmp(nam,"OPTIONS ") && !strcmp(typ, "   ")) drive = 1;

	/* binaries, libraries and object files */
	if (!strcmp(typ, "COM")) drive = 1;
	if (!strcmp(typ, "LIB")) drive = 2; 
	if (!strcmp(typ, "OBJ")) drive = 2; 

	/* some extras for messages, overlays, includes */
	if (!strcmp(typ, "HLP")) drive = 1;
	if (!strcmp(typ, "MSG")) drive = 1;
	if (!strcmp(typ, "OVR")) drive = 1;
	if (!strcmp(typ, "OVL")) drive = 1;
	if (!strcmp(typ, "REL")) drive = 2;
	if (!strcmp(typ, "IRL")) drive = 2;
	if (!strcmp(typ, "H  ")) drive = 3;

	if (!drive) return 0;
	
	*odrv = *fcb;
	*fcb = drive;
	return 1;
}

/* zxcc has a trick with some filenames: If it can't find them where they
       should be, and a drive wasn't specified, it searches BINDIR80, 
       LIBDIR80 or INCDIR80 (depending on the type of the file).
 */

static void fcb_to_name(byte *fcb, char *buf)
{
	int i, p = 0;
	int drv = fcb[0] & 0x7F;

	if (drv) buf[p++] = 'A' + drv - 1;
	else     buf[p++] = 'P';
	buf[p++] = ':';

	for (i = 1; i <= 8; i++)
	{
		char c = fcb[i] & 0x7F;
		if (c != ' ') buf[p++] = c;
	}
	if ((fcb[9] & 0x7F) != ' ')
	{
		buf[p++] = '.';
		for (i = 9; i <= 11; i++)
		{
			char c = fcb[i] & 0x7F;
			if (c != ' ') buf[p++] = c;
		}
	}
	buf[p] = 0;
}

word x_fcb_open(byte *fcb, byte *dma)
{
	word rv;
	byte odrv;

	if (fileverbose)
	{
		char name[16];
		int drv = fcb[0] & 0x7F;
		char *dir;

		fcb_to_name(fcb, name);
		if (!drv) drv = cpm_drive + 1;
		dir = xlt_getcwd(drv - 1);
		fprintf(stderr, "%s: open %s -> %s\n", progname, name,
			dir ? dir : "(unmapped)");
	}

	rv = fcb_open(fcb, dma);

	if (rv == 0xFF)
	{
		if (fcbforce(fcb, &odrv))
		{
			if (fileverbose)
			{
				char name[16];
				int drv = fcb[0] & 0x7F;
				char *dir;

				fcb_to_name(fcb, name);
				dir = xlt_getcwd(drv - 1);
				fprintf(stderr, "%s: retry %s -> %s\n",
					progname, name,
					dir ? dir : "(unmapped)");
			}
			rv = fcb_open(fcb, dma);
			*fcb = odrv;
		}
	}
	return rv;
}



word x_fcb_stat(byte *fcb)
{
        word rv = fcb_stat(fcb);
        byte odrv;

        if (rv == 0xFF)
        {
                if (fcbforce(fcb, &odrv))
                {
                        rv = fcb_stat(fcb);
                        *fcb = odrv;
                }
        }
        return rv;
}



