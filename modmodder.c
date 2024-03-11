// Hemiyoda's mod sampleswapper 1.0
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
  
    // Pointer to the file to be
    // read from
    FILE* fptr1;
    FILE* fptr2;
    FILE* fptr3;
 
 
// Filename given as the command
// line argument
unsigned long samplestart;


unsigned long modparser(void) {

unsigned long hpattern=0;	
unsigned char x=0;
unsigned char data;
unsigned long offset;

fseek(fptr1, 952, SEEK_SET); // Seek to beginning of pattern order data	

x=0;
while (x<128) { 
				data = getc(fptr1); 
				if (data>hpattern) { hpattern=data; }
				x++;
}	
	offset=(hpattern*1024)+1084+1024;
return offset;

}

int main(int argc, char* argv[])
{
    unsigned int c=16;
	
	unsigned char combinedbyte;
	long inputlength=0;
	long outputlength=0;
	long samplelength=0;
	int dummy;
	

	printf("\nHemiyoda's mod sample swapper v1.0\n\n");
	if (argc<2) { 
	printf("Usage: modmodder mod-filename    (Pro/Noisetracker format only)\n");
	printf("decoded.pcm is spliced onto mod pattern/meta data\n");
	printf("modded.mod is the output\n\n"); }

    // If the file exists and has
    // read permission

if (argc<2) { printf("<<No input file specified, exiting>>\n"); return 1; }

printf("Input filename: "); printf("%s", argv[1]); printf("\n");

	fptr1 = fopen(argv[1], "rb");

    if (fptr1 == NULL) {
	printf("Mod file can not be opened\n");
        return 1;
    }

	fptr2 = fopen("decoded.pcm", "rb"); printf("Samplefile: decoded.pcm\n"); 
	if (fptr2 == NULL) {
	printf("decoded.pcm can not be opened\n");
        return 1;
	}	

	fptr3 = fopen("modded.mod", "wb"); //printf("Output filename: modded.mod\n"); 
	if (fptr3 == NULL) {
	printf("Output file can not be opened\n");
        return 1;
    }

		fseek(fptr1, 0, SEEK_SET); // Seek to beginning in amiga mod-data

			while ((dummy = fgetc(fptr1)) != EOF) { inputlength++; }
	
		fseek(fptr1, 0, SEEK_SET); // Seek to beginning in amiga mod-data
		fseek(fptr2, 0, SEEK_SET); // Seek to beginning in decoded audio-data
		fseek(fptr3, 0, SEEK_SET); // Seek to beginning in modded mod file

c=0;
samplestart=modparser();

		fseek(fptr1, 0, SEEK_SET); 
		fseek(fptr3, 0, SEEK_SET); 

while (c<samplestart) { putc(fgetc(fptr1), fptr3); c++; outputlength++; }

samplelength=0;

while ((dummy = fgetc(fptr2)) != EOF) { samplelength++; }
printf("Sampledata length=%d\n",samplelength);

fseek(fptr2, 0, SEEK_SET); 
		
while (samplelength>0) {
	
		putc(fgetc(fptr2), fptr3);
		samplelength--;
		outputlength++;
		
}

printf("Input length=%d\n", inputlength);
printf("Output length=%d\n",outputlength);

if (outputlength!=inputlength) { printf("<< Warning! Input/Output length does not match >>\n");}   
  
    // Close the files
    fclose(fptr1);
    fclose(fptr2);  
    fclose(fptr3);  


    return 0;
}