// Hemiyodas Adaptive Delta-(Fibonacci) decoder v0.97
//   
// --------------------------------------
// A lossy sound compression format using delta values and a 4K lookup table
// Inspired by Steve Hayes Fibonacci Delta sound compression technique

/*
 * ----------------------------------------------------------------------------
 * <hemiyoda@gmail.com> wrote this file. 
 * As long as you retain this notice you can do whatever you want with this stuff. 
 * If you find any use for this kludge it would be fantastic with a shout-out. 
 * Signed: Erik Berg aka Hemiyoda / DMS       2024
 * ----------------------------------------------------------------------------
 *
 * -- Version History --
 * v0.97 First public release, only 4K LUT decoding implemented.
 * 
 *
 * Encoded file format:
 * --------------------------------------------------
 *	Unsigned Byte 0: ASCII E
 *	Unsigned Byte 1: ASCII .
 *	Unsigned Byte 2: ASCII B
 *	Unsigned Byte 3: ASCII .     (Those who know, they know ;) )
 *	Unsigned Byte 4: Compression used, 1 = 4Kb lookup, 16 bytes frame size, 2 = 256B lookup, 3 = 4Kb LUT + compacted error stream
 *	Unsigned Byte 5: Uncompressed data size MSB
 *	Unsigned Byte 5: Uncompressed data size MSB
 *	Unsigned Byte 7: Uncompressed data size LSB  (24bits total)
 *	Bytes 8-15: Spares
 *	Signed Byte 16-4111: Lookup table 256x16 Signed Bytes
 * 	Signed Byte 4112: Uncompressed first sample
 *	-----------Delta data starts---------------------
 * 	Unsigned Byte: Lookupset used (4K lut = 256 tables)
 *	Unsigned Byte: 8 bytes (16 nibbles) referring to signed lookup values
 * 	Unsigned Byte: Lookupset used
 *	Unsigned Byte: 8 bytes (16 nibbles) referring to signed lookup values
 *	etc etc. until EOF  ( You can use 24-bit uncompressed data size figure to keep track of decompression progress )
 *
 */

#include <stdio.h>
#include <stdlib.h>
    
  
// Filename given as the command
// line argument

    // Pointer to the file to be
    // read from
    FILE* fptr1;
    FILE* fptr2;

int main(int argc, char* argv[])
{
    unsigned int c=0;
		
	int dummy;
	unsigned long filelength=0;
	unsigned long sampleidx=0;
	unsigned long samplelength=0;

	signed char sampledata;

	unsigned char default_n=1;
	unsigned char deltadata;
	unsigned char lookupset=0;
	unsigned char chardata;
	unsigned int intdata;
	
	signed char ltable[4096];
	
printf("\nAdaptive Delta-(Fibonacci) decoder v0.97\n\n");

if (argc>2) { default_n=0; }
if (argc<2) { 
	
	printf("Usage: deladadec filename outputfilename \n");
	printf("Will default to decoded.pcm if no output filename is specified\n\n");
	printf(" Hemiyoda hemiyoda@gmail.com 2024 \n\n");
	printf("<<No input file specified, exiting>>\n"); return 1; }

printf("Input filename: "); printf("%s", argv[1]); printf("\n");

	fptr1 = fopen(argv[1], "rb");

    if (fptr1 == NULL) {
	printf("Compressed file can not be opened\n");
        return 1;
    }

	if (default_n==1) {
	fptr2 = fopen("decoded.pcm", "wb"); printf("Output filename: decoded.pcm\n\n"); }

	if (default_n==0) {
	fptr2 = fopen(argv[2], "wb"); printf("Output filename: "); printf("%s\n", argv[2]); }
	

	if (fptr2 == NULL) {
	printf("Output can not be opened\n");
        return 1;
	}
  
// -------------------  "Decode" original sample length ---------------------------
		fseek(fptr1, 5, SEEK_SET);

			chardata=fgetc(fptr1);
			samplelength=chardata;
			samplelength=samplelength<<16;
			
			chardata=fgetc(fptr1);
			intdata=chardata;
			intdata=intdata<<8;
			samplelength=samplelength+intdata;
			
			chardata=fgetc(fptr1);
			samplelength=samplelength+chardata;
// -----------------------------------------------------------------------------------			

		fseek(fptr1, 0, SEEK_SET);
		while ((dummy = fgetc(fptr1)) != EOF) { filelength++; }
		printf("Input filelength=%d\n",filelength);


	//Read lookup-table to array
		c=0;
		fseek(fptr1, 16, SEEK_SET);
			while (c<4096) { ltable[c]=fgetc(fptr1); c++; }
			
		
		fseek(fptr2, 0, SEEK_SET); // Seek to beginning in decoded audio-data
		sampledata=fgetc(fptr1);
	
		putc(sampledata, fptr2); //Copy first sample
		sampleidx=1;


while (sampleidx<samplelength) {

	lookupset=fgetc(fptr1); 	//Read lookupset

	c=0;
	while (c<8 && sampleidx<samplelength) {
											chardata=fgetc(fptr1);
													
											deltadata=chardata>>4; 	//Extract MSB nibble
											sampledata = sampledata+ltable[(lookupset*16)+deltadata];
											putc(sampledata, fptr2);
											sampleidx++;

			if (sampleidx<samplelength) {	deltadata=chardata &= ~(0xF0); 	//Extract LSB nibble
											sampledata = sampledata+ltable[(lookupset*16)+deltadata];
											putc(sampledata, fptr2);
											sampleidx++; }

	c++;
	}
		
		
}

	printf("Sample length=%d\n", samplelength);
	printf("Samples processed=%d\n", sampleidx);

  
    // Close the files
    fclose(fptr1);
    fclose(fptr2);  

    return 0;
}