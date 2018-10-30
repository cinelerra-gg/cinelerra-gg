#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int ac, char **av)
{
	iconv_t cd;
	size_t inbytes = 0, outbytes = 0;
	const char *from_enc = av[1], *to_enc = av[2];
	char *input, *output, *inbfr, *outbfr;
	struct stat st;
	if( ac < 3 ) {
		fprintf(stderr,"usage: chrset from-enc to-enc < infile > outfile\n");
		exit(1);
	}
	if( fstat(fileno(stdin), &st) || !st.st_size ) {
		fprintf(stderr, "cant stat stdin\n");
		exit(1);
	}
	from_enc = av[1];
	to_enc = av[2];
	cd = iconv_open(to_enc, from_enc);
	if( cd == (iconv_t)-1 ) {
		fprintf(stderr, "Conversion from %s to %s is not available",
			from_enc, to_enc);
		exit(1);
	}
	inbytes = st.st_size;
	inbfr = input = malloc(inbytes);
	read(0,input,inbytes);
	outbytes = inbytes*2 + inbytes/2;
	outbfr = output = malloc(outbytes);
	iconv(cd, &input, &inbytes, &output, &outbytes);
	iconv_close(cd);
	write(1,outbfr,output-outbfr);
	free(outbfr);
	free(inbfr);
	return 0;
}

