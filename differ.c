// Diffextractor 1.0 by Hemiyoda hemiyoda@gmail.com
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

    // Pointer to the file to be
    // read from
    FILE* fptr1;
    FILE* fptr2;
    FILE* fptr3;
  

	unsigned long samplestart;
	unsigned long filelength;
	unsigned long totalsamples;
	unsigned long filepos=0;


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
	
	signed char decodeddata;
	signed char origdata;
	signed char diffdata;
	signed char frameerror;
	signed char optcheck=1;
	signed char default_n=1;

	int dummy;
	unsigned char frameidx=0;
	unsigned int frame=0;
	unsigned int errorcount=0;
	
	unsigned char modmode=0;
	

printf("\nDiff file generator v1.0\n\n");


if (argc>1) { if (argc>2) { optcheck=strcmp (argv[2], "-m"); } if (optcheck==0) { modmode=1; } }
if (argc>2 && modmode==0) { default_n=0; if (argc>3) { optcheck=strcmp (argv[3], "-m"); } if (optcheck==0) { modmode=1; } }
if (argc<2) { 
	
	printf("Usage: differ file1 file2 [-m]\n\n");
	printf("- Will default to decoded.pcm if second file is omitted\n");
	printf("- Use option -m if file1 is a mod file\n\n");
	printf("     - Hemiyoda 2024 -\n\n");

	printf("<<No input file specified, exiting>>\n"); return 1; }

if (modmode==1) { printf("mod mode = yes\n"); }
printf("Input filename: "); printf("%s", argv[1]); printf("\n");

	fptr1 = fopen(argv[1], "rb");

    if (fptr1 == NULL) {
	printf("File1 can not be opened\n");
        return 1;
    }

	if (default_n==1) {
	fptr2 = fopen("decoded.pcm", "rb"); printf("File2 filename: decoded.pcm\n"); }

	if (default_n==0) {
	fptr2 = fopen(argv[2], "rb"); printf("File2 filename: "); printf("%s\n", argv[2]); }
	
	if (fptr2 == NULL) {
	printf("file2 can not be opened, exiting\n");
        return 1;
	}

	fptr3 = fopen("diff.raw", "wb"); printf("Output filename: diff.raw\n"); 
	if (fptr3 == NULL) {
	printf("diff.raw can not be opened\n");
        return 1;
    }

  		
		
		fseek(fptr3, 0, SEEK_SET); // Seek to beginning in diff file


samplestart=0;
if (modmode==1) { samplestart=modparser(); }

filelength=0;


while ((dummy = fgetc(fptr2)) != EOF) { filelength++; }
printf("Input filelength=%d\n",filelength);

totalsamples=filelength-samplestart;

fseek(fptr1, samplestart, SEEK_SET); 
fseek(fptr2, 0, SEEK_SET);
		
while (filelength>0) {

		origdata=fgetc(fptr1); 
		decodeddata=fgetc(fptr2);
		diffdata=origdata-decodeddata;
		
		if (filepos>0) {  
							if (diffdata<0) { frameerror=frameerror+((diffdata*2)-diffdata); } 
							if (diffdata>0) { frameerror=frameerror+diffdata; }
							frameidx++;  
		if (frameidx>15) { frameidx=0; 
							if (frameerror>9) { printf("Error=%d",errorcount); 
												printf(" Filepos=%d",filepos); 
												printf(" Frame=%d",frame); 
												printf(" Error=%d\n",frameerror); errorcount++; }
		frame++; frameerror=0;	
		}
		}
		
		putc(diffdata, fptr3);  
		//printf("Origdata=%d",origdata); printf(" Decodeddata=%d",decodeddata); printf(" Diffdata=%d\n",diffdata);
		filelength=filelength-1;
		filepos++;
}
  
  
    // Close the files
    fclose(fptr1);
    fclose(fptr2);  
    fclose(fptr3);  


    return 0;
}