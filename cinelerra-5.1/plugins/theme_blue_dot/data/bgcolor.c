#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//for f in picture.png scope.png; do
//  pngtopam -alpha < $f > /tmp/a
//  pngtopnm < $f | /tmp/a.out > /tmp/b
//  pnmtopng -alpha=/tmp/a < /tmp/b > /tmp/data1/$f
//done
int main(int ac, char **av)
{
	int r, g, b;
	char line[1024];
	do {
		fputs(fgets(line,sizeof(line),stdin),stdout);
	} while( strcmp(line,"255\n") );
	while( (r=getc(stdin)) >= 0 && (g=getc(stdin)) >= 0 && (b=getc(stdin)) >= 0 ) {
		double rr = r/255., gg = g/255., bb = b/255.;
		int y = (int)(( 0.29900*rr + 0.58700*gg + 0.11400*bb)*255. + 0.5);
		int u = (int)((-0.16874*rr - 0.33126*gg + 0.50000*bb + 0.5) * 255. + 0.5);
		int v = (int)(( 0.50000*rr - 0.41869*gg - 0.08131*bb + 0.5) * 255. + 0.5);
		if( abs(u-0x80) < 2 && abs(v-0x80) < 2 && y <= 0x48 )
			if( (y += 0x100-0x48) >= 0x100 ) y = 0x100-1;
		double yy = y/255., uu = (u-128)/255., vv = (v-128)/255.;
		r = (int)((yy + 1.40200*vv) * 255. + 0.5);
		g = (int)((yy- 0.34414*uu - 0.71414*vv) * 255. + 0.5);
		b = (int)((yy+ 1.77200*uu) * 255. + 0.5);
		putc(r, stdout); putc(g, stdout); putc(b, stdout);
	}
	return 0;
}
