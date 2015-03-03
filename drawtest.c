
#include <stdio.h>
#include <stdlib.h>
#include "draw.h"

int
main(void)
{
	short a[2] = {100, 200};
	short b[2] = {200, 300};
	short c[2] = {400, 200};
	uchar yellow[4] = {0x00, 0xff, 0xff, 0xff};
	int r;
	if(drawinit(800, 600) == -1)
		exit(1);
	for(;;){
		if((r = drawhandle(drawfd())) == -1){
			fprintf(stderr, "drawhandle error\n");
			exit(1);
		}
		if(r & 2){
			fprintf(stderr, "resized\n");
		}
		if(r & 1){
			drawtri(framebuffer, width, height, a, b, c, yellow);
			drawflush();
			drawreq();
		}
	}
	exit(0);
}
