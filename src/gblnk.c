#include <unistd.h>
#include <sys/io.h>
#include <stdio.h>
#include <time.h>
#include <allegro.h>

/* 

    gblnk - for transferring images between your gameboy camera and your PC
    Copyright (C) 2002 Chris McCormick

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

to do:

a) get more sleep
b) put in a 'batch mode' transfer for multiple prints
c) Figure out what the REAL compression algorithm is

*/

/* Set up our parallel port address */
#define lptbase 0x378
#define DATA	lptbase+0
#define STATUS	lptbase+1
#define CONTROL lptbase+2

/* how long to wait until we know the data is transferred it */
/* if your pictures are incomplete increase this val */
#define TIMEOUT 2

int main(int argc, char **argv)
	{
	/* our incoming data buffer */
	unsigned char buffer[0x1760];

	/* byte to be read, and a counter */
	int inbyte, count =0 , x = 0, y = 0, col=0, row=0, c=0;

	/* our timeout waiters */
	int zerowait = 0;
	int datastartwait = 0;
	
	/* how big is the next block of picture data? */
	int readSize=640;
	/* the beginning of the next block to read */
	int start=0;

	/* map of gameboy photo colors */
	int colmap[4];

	/* characters for creating the turny thing */
	char turner[4] = {'|', '/', '-', '\\'};
	
	/* temporary bitmap */
	BITMAP *tmpbmp;

	/* set up a new palette */
	PALETTE pal;

	/* test if they've put in the right number of arguments */
	if (argc != 2)
		{
		printf("Usage: %s filename\n", argv[0]);
		return -1;
		}
	
	printf("Initializing allegro\n");
	/* initialize allegro */
	allegro_init();

	/* install the allegro keyboard handler */
	install_keyboard(); 

	/* set the graphics mode */
	printf("Launching allegro window\n");
	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 300, 300, 0, 0) == 1)
		{
		printf("Sorry, I can't create a window.");
		return -1;
		}

	printf("Creating our bitmap\n");
	/* initialize our temporary bitmap to 8 bit */
	tmpbmp = create_bitmap(150, 150);
	clear_bitmap(tmpbmp);

	/* create color map */
	printf("Creating color map\n");
	colmap[3] = makecol(0,0,0);
	colmap[2] = makecol(127, 127, 127);
	colmap[1] = makecol(191, 191, 191);
	colmap[0] = makecol(255, 255, 255);

	/* get the current palette */
	get_palette(pal);

	/* Get permission to write to the parallel port - possible security hazard. */
	/* if anyone knows how i should fix this Please tell me! */
	printf("Getting permission to access the parallel port.\n");
	if (ioperm(lptbase, 3, 1))
		{
		printf("Error getting parallel port permission.\n");
		printf("You must have write access to the parallel port (log in as root?)\n");
		return 1;
		}
	
	printf("Resetting device.\n");
	
	/* get the time right now */
	datastartwait = time(NULL);
	do 
		{
		outb(0x24, CONTROL);

		/* get the time right now */
		zerowait = time(NULL);
		while ((inb(STATUS) & 0x20) == 0)
			{ 
			/* make sure this doesn't ever take longer than 5 seconds */
			if ((time(NULL) - zerowait) > 5)
				{
				/* lose permission to parallel port */
				ioperm(lptbase, 3, 0);
				printf("Timeout while resetting the parallel port.\n");
				return 1;
				}
			}
		inbyte = inw(DATA) & 0xF;
		
		/*printf("%x\n",inbyte);*/
		/* check for a timeout */
		if ((time(NULL) - datastartwait) > 10)
			{
			/* lose permission to parallel port */
			ioperm(lptbase, 3, 0);
			printf("Timeout while resetting the parallel port.\n");
			return 1;
			}
		}
	while (inbyte != 4);
	
	outb(0x22, CONTROL);
	while ((inb(STATUS) & 0x20) != 0)
		{
		if (key[KEY_ESC])
			{
			printf("User exit\n");
			return 1;
			}
		}	
	outb(0x26, CONTROL);

	/* Start the actual data receive cycle */
	textprintf(screen, font, 10, 10, makecol(255, 255, 255), "Awaiting data transfer:");
	textprintf(screen, font, 10, 20, makecol(255, 255, 255), "(Escape to cancel)");

	datastartwait = time(NULL) + 60;
	do 	{
		outb(0x26, CONTROL);
		while (((inb(STATUS) & 0x20) == 0) && (!key[KEY_ESC]) && ((time(NULL) - datastartwait) < TIMEOUT));
		inbyte = inw(DATA) & 0xF;
		outb(0x22, CONTROL);
		while (((inb(STATUS) & 0x20) != 0) && (!key[KEY_ESC]) && ((time(NULL) - datastartwait) < TIMEOUT));
		outb(0x26, CONTROL);
		while (((inb(STATUS) & 0x20) == 0) && (!key[KEY_ESC]) && ((time(NULL) - datastartwait) < TIMEOUT));
		inbyte |= (inw(DATA) & 0xF) << 4;
		outb(0x22, CONTROL);
		while (((inb(STATUS) & 0x20) != 0) && (!key[KEY_ESC]) && ((time(NULL) - datastartwait) < TIMEOUT));

		/* store our last byte */
		buffer[count++] = inbyte;

		/* progress meter */
		rectfill(screen, 5, 5, 20, 20, makecol(0, 0, 0));
		textprintf(screen, font, 5, 5, makecol(255, 255, 255), "%c", turner[count%4]);

		/* reset our timeout value */
		if ((time(NULL) - datastartwait) < TIMEOUT)
			datastartwait = time(NULL);
		}
	while (!key[KEY_ESC] && ((time(NULL) - datastartwait) < TIMEOUT));

	for (c = 0; c < count; c++)
		{
		/* if we encounter a header */
		if ((buffer[c] == 136) & (buffer[c+1] == 51))
			{
			/* if we get any of these headers, jump forward 10 */
			if (buffer[c+2] == 1) c=c+10;
			if (buffer[c+2] == 2) c=c+10;
			if (buffer[c+2] == 15) c=c+10;
			
			/* if this header is a length header */
			if (buffer[c+2] == 4)
				{
				/* get in a block of data */
				readSize = (buffer[c+5] * 256) + buffer[c+4];
				/* skip forward 6 */
				c=c+6;
				/* set the start point for this chunk of data */
				start = c;
				}			
			}
		else
			{
			if (((c-start) % 320) == 0)
				{
				row++;
				col=0;
				y=0;
				}
			else if (((c-start) % 16) == 0)
				{
				col++;
				y=0;
				}
			else if (((c-start) % 2) == 0)
				{
				y++;
				}
			x=(c-start)%2;

			if (x==0)
				{
				putpixel(tmpbmp, (x + col*2)*4 + 7, y + row*8, colmap[buffer[c+1] >> 0 & 0x03]);	
				putpixel(tmpbmp, (x + col*2)*4 + 6, y + row*8, colmap[buffer[c] >> 0 & 0x03]);	

				putpixel(tmpbmp, (x + col*2)*4 + 5, y + row*8, colmap[buffer[c+1] >> 2 & 0x03]);	
				putpixel(tmpbmp, (x + col*2)*4 + 4, y + row*8, colmap[buffer[c] >> 2 & 0x03]);	

				putpixel(tmpbmp, (x + col*2)*4 + 3, y + row*8, colmap[buffer[c+1] >> 4 & 0x03]);	
				putpixel(tmpbmp, (x + col*2)*4 + 2, y + row*8, colmap[buffer[c] >> 4 & 0x03]);	
				
				putpixel(tmpbmp, (x + col*2)*4 + 1, y + row*8, colmap[buffer[c+1] >> 6 & 0x03]);	
				putpixel(tmpbmp, (x + col*2)*4 + 0, y + row*8, colmap[buffer[c] >> 6 & 0x03]);	
				}
			}
		}

	/* plaster it onto the screen */
	stretch_blit(tmpbmp, screen, 0, 0, 150, 150, 0, 0, 150, 150);

	while(!key[KEY_ESC])
		{
		if (key[KEY_D])
			{
			/* set it to double size */
			stretch_blit(tmpbmp, screen, 0, 0, 150, 150, 0, 0, 300, 300);
			}
		if (key[KEY_S])
			{
			/* write this bitmap to file */
			save_bitmap(argv[1], screen, pal);
			}
		if (key[KEY_A])
			{
			/* set it to normal size */
			clear(screen);
			stretch_blit(tmpbmp, screen, 0, 0, 150, 150, 0, 0, 150, 150);
			}
		}


	/* lose permission to parallel port */
	ioperm(lptbase, 3, 0);
	return 0;
	}

END_OF_MAIN();
