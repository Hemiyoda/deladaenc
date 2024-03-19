// Adaptive Delta Fibonacci encoder v0.98  
// --------------------------------------
// An 8-bit lossy sound compression format. Uses 8-bit delta values called from a 256B-4K lookup table
// specifically targeted for Amiga computers with sample sizes <512Kb and aims for trivial decoding complexity
//
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
 * v0.97 First public release, only 4K LUT implemented. Room for improvements overall. There be bugs.
 * v.098 Second public release. 
 * - Added mild culling of LUTs seldomly used. 
 * - Encoder now prioritizes that the last byte in frame is aligned within 4 steps from next frame.
 * - Added one more pass.
 * - Small improvement in audioquality due to above improvements
 *
 * File format explanation, see decompressor: deladadec.c
 */
 
 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

  

    // Pointer to the file to be
    // read from
    FILE* fptr1;
    FILE* fptr2;
    FILE* fptr3;
	FILE* fptr4;
	FILE* fptr5;
	
	int dummy;
	int modmode=0;
	unsigned long filepos=0;
	unsigned long filelength=0;
	unsigned long totalerrors=0;
	unsigned long percentage;
	unsigned long totalsamples;
	unsigned long startoffset=0;
	unsigned long mindistance; 
	unsigned long distance;
	unsigned long oldfilepos;
	
	signed char nextsample;
	signed char sampleswapper;
	signed char sampledelta;
	signed char samplenewval;
	signed char firstsample;
	signed char calcedsample[256];
	signed char ltable[4097];
	signed char newtable[17];
	signed char deltatable[17];

	
	unsigned char lkptblmatch;
	unsigned char save=0;
	unsigned char spillbytes=0;
	unsigned char error;
	unsigned char framebuffer[4096];
	unsigned char readbuffer[17];
	unsigned char matchedlookupset;
	unsigned char verbose=0;
	unsigned char newframe=0;

	
	unsigned int errormatchframe=0;
	unsigned int olderrormatchframe=0;
	unsigned int errorthreshold;
	unsigned int totalframes=0;
	unsigned int frameerror[257];
	unsigned char peak_frameerror[257];
	unsigned char lastbyte_error[257];
	unsigned int frame=0;
	unsigned int stats[256];	//Should suffice for Amiga modules
	unsigned int lowest_error=0;
	
	float distortion;
	float dispdist;
	float prevdistortion[16];
	float highestdist=0;

	signed char filedata[2097152]; //2Mb
	
	
// Fibonacci table as starting point
signed char fbntable[256]={
-34, -22, -13, -8, -5, -3, -2, -1, 0, 1, 2, 3, 5, 8, 13, 22,
-128, -96, -80, -64, -48, -32, -16, -14, 0, 8, 16, 28, 32, 48, 64, 96,
-128, -112, -96, -80, -64, -48, -32, -16, 0, 16, 32, 48, 64, 80, 96, 112,
-84, -56, -42, -35, -28, -21, -14, -7, 0, 7, 14, 21, 28, 42, 84, 112,
-60, -50, -40, -30, -20, -16, -10, 0, 10, 20, 30, 40, 50, 58, 60, 70,
-60, -36, -20, -13, -10, -8, -3, -1, 3, 7, 11, 16, 20, 24, 40, 45,
-40, -19, -13, -9, -8, -7, -5, -1, 5, 7, 13, 18, 19, 26, 37, 50,
-35, -28, -20, -13, -12, -7, -3, -2, 2, 3, 11, 15, 16, 27, 33, 46,
-37, -25, -16, -11, -9, -5, -2, 3, 5, 7, 9, 14, 15, 20, 26, 48,
-31, -29, -26, -24, -16, -13, -9, -3, 0, 4, 9, 14, 18, 20, 24, 32,
-30, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 6, 8, 34,
-22, -16, -14, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 16,
-18, -14, -10, -8, -7, -6, -5, -4, -2, -1, 3, 6, 8, 12, 20, 23,
-16, -11, -9, -5, -4, -2, -1, 0, 1, 2, 4, 6, 9, 11, 13, 14,
-13, -10, -7, -6, -4, -3, -2, -1, 0, 2, 3, 6, 7, 10, 12, 14,
-11, -8, -5, -4, -3, -2, -1, 0, 1, 2, 4, 5, 6, 15, 19, 37,
};

	



int matchfinder(signed char previoussample, signed char nextsample, unsigned int lookupset, unsigned char framectr) {
unsigned int zz=0;
int errorhistory=256;
int error;
unsigned int offset=0;
unsigned char match=0;
signed char sampleresult;

				while (zz<16) { 
								offset=(lookupset*16)+zz;
								sampleresult=previoussample+ltable[offset];
															
								if (nextsample>=sampleresult) { error=nextsample-sampleresult; }
								if (nextsample<sampleresult) { error=sampleresult-nextsample; }
								if (error<errorhistory) { match=zz; errorhistory=error; } 

								if (verbose>0) {
								printf("fp %d",filepos);	
								printf(" i %d",zz);
								printf(" ls %d",lookupset);
								printf(" SD %d",previoussample); 
								printf(" NS %d",nextsample);
								printf(" LV %d",ltable[offset]); 
								printf(" SR %d",sampleresult);
								printf(" ER %d",error);
								printf(" eh %d",errorhistory);
								printf(" m %d\n",match); }								
								zz++; }
								
								frameerror[lookupset]=frameerror[lookupset]+errorhistory;
								if (errorhistory>peak_frameerror[lookupset]) { peak_frameerror[lookupset]=errorhistory; }	//Save highest error for set
								if (framectr==15) { lastbyte_error[lookupset]=errorhistory; }
								//if (lookupset>10) { printf("lookupset %d",lookupset); printf(" index %d",(lookupset*16)+(framectr)); printf(" filepos %d\n",filepos); }
								framebuffer[(lookupset*16)+(framectr)]=match;

return match;	
}

int bitpacker(unsigned long samplelength, unsigned long offset, unsigned char modmode)
{
	unsigned int c=16;
	
	unsigned char highnibble;
	unsigned char lookupset;
	unsigned char lownibble;
	unsigned char highbyte;
	unsigned char lowbyte;
	unsigned char combinedbyte;
	unsigned long comprlength=0;
	unsigned long filelength=0;
	unsigned long inputlength=0;
	unsigned long origlength=0;
	int dummy;
	

	printf("Entering Bitpacker\n");

	fptr5 = fopen("encoded.tmp", "rb");
    if (fptr5 == NULL) {
	printf("encoded.tmp can not be opened\n");
        return 1;
    }

	fptr2 = fopen("lookup.dat", "rb"); //printf("lookup.dat\n"); 
	if (fptr2 == NULL) {
	printf("lookup.dat can not be opened\n");
        return 1;
    }

	fptr3 = fopen("encoded.df+", "wb"); //printf("encoded.df+\n"); 
	if (fptr3 == NULL) {
	printf("encoded.df+ can not be opened\n");
        return 1;
    }

if (modmode==1) {	
	fptr4 = fopen("packedmod.dfm", "wb"); //printf("packedmod.dfm\n"); 
	if (fptr4 == NULL) {
	printf("packedmod.dfm can not be opened\n");
        return 1;
    }
}  	
	
		
		fseek(fptr5, 0, SEEK_SET); // Seek to beginning in temporary encoded audio-data
		fseek(fptr3, 0, SEEK_SET); // Seek to beginning in nibble file

		putc(69, fptr3); // E
		putc(46, fptr3); // .
		putc(66, fptr3); // B
		putc(46, fptr3); // .
		putc(1, fptr3); // Type. 1 = 4Kb lookup, 2 = 256 lookup, 3 = 4Kb LUT with 4-bit error stream 
		putc(0, fptr3); // Data size MSB
		putc(0, fptr3);
		putc(0, fptr3); // Data size LSB  (24bits total)
		
		putc(0, fptr3); // Reserved
		putc(0, fptr3); // Reserved
		putc(0, fptr3); // Reserved
		putc(0, fptr3); // Reserved
		putc(0, fptr3); // Reserved
		putc(0, fptr3); // Reserved
		putc(0, fptr3); // Reserved
		putc(0, fptr3); // Reserved
		
comprlength=17;
		
		c=0;
		fseek(fptr2, 0, SEEK_SET);
			while (c<4096) { putc(fgetc(fptr2), fptr3);  
							 comprlength++; 
							 c++; }  //Copy lookuptable
		
origlength=samplelength;

		fseek(fptr5, 0, SEEK_SET); 
			while ((dummy = fgetc(fptr5)) != EOF) { inputlength++; }
			printf("Inputlength=%d\n",inputlength);
		
		fseek(fptr5, 0, SEEK_SET); // Seek to beginning in encoded audio-data
		putc(fgetc(fptr5), fptr3);	//Copy first sample (full 8-bits)
		origlength--;
		inputlength--;


while (origlength>0) {		lookupset=fgetc(fptr5);
							putc(lookupset, fptr3);	//Copy 8-bit lookupvalue
							inputlength--;
							comprlength++; 

					c=0; 
					while (c<8 && origlength>0) {
					
						highnibble=fgetc(fptr5); inputlength--; origlength--; 
						
						lownibble=0;
						if (origlength>0) {lownibble=fgetc(fptr5); inputlength--; origlength--; }
					
						combinedbyte=(highnibble<<4)+lownibble;
						putc(combinedbyte, fptr3); comprlength++;					
					c++;
					}
	
}

printf("Original sample length=%d\n", samplelength);
printf("Compressed length=%d\n",comprlength);

//printf("Counter=%d\n", origlength);


fseek(fptr3, 5, SEEK_SET);
						highbyte=samplelength>>16;
						putc(highbyte, fptr3);
						samplelength &= ~(0xF0000);
						highbyte=samplelength>>8;
						putc(highbyte, fptr3);
						samplelength &= ~(0xFFF00);
						lowbyte=samplelength;
						putc(lowbyte, fptr3);
						
printf("Bitpacking done.\n");  
  
    // Close the files
    fclose(fptr2);  
    fclose(fptr3); 
	fclose(fptr5);
	

if (modmode==1) {
	fptr3 = fopen("encoded.df+", "rb");  
	if (fptr3 == NULL) {
	printf("encoded.df+ can not be opened\n");
        return 1;
    }

		c=0;
		fseek(fptr1, 0, SEEK_SET);
		fseek(fptr4, 0, SEEK_SET);		
		while (c<offset) { putc(fgetc(fptr1), fptr4);  c++; }  //Copy pattern+metadata

		fseek(fptr3, 0, SEEK_SET);
		while ((dummy = getc(fptr3)) != EOF) { filelength++; }	
		fseek(fptr3, 0, SEEK_SET);
		c=0;
		while (c<filelength) { putc(fgetc(fptr3), fptr4);  c++; }  //Copy compressed data

	
printf("Splicing of pattern/packed samples to packedmod.dfm done.\n");
	fclose(fptr1);
	fclose(fptr3);
	fclose(fptr4); 
}
}

void savebestframe(unsigned int lowest_errorset, unsigned char stopbyte) {
unsigned char i2=0;
										
					putc(lowest_errorset, fptr3);					
					while (i2<stopbyte) 
									{ 
									putc(framebuffer[(lowest_errorset*16)+i2], fptr3);
									
									//printf("lookupset %d",lowest_errorset); printf(" byte %d", i2); printf(" data %d\n", framebuffer[(lowest_errorset*16)+i2]);
									i2++; }	
}



void encoder(unsigned int no_oflookupsets, unsigned int errorthreshold, unsigned long distanceth) {


// 	  	Calc sample delta
// 		Find closest delta lookup value (go thru all lookup values). 
// 		Apply looked up delta value to sample and use as starting point for next sample 
// 		loop 16 times for one frame
//   Increment lookup base multiplier, rewind file position 16 bytes
//   Rinse and repeat up to 256 times 
// Write best frame lookup table number and 16 bytes of lookup index data
// Sounds easy huh!


unsigned int skip=0;
unsigned int i;
unsigned int i2;
unsigned char framectr;
unsigned char stop;
unsigned int lookupset;
unsigned char dc=0;
unsigned char dc2=0;

unsigned char lowest_errorset=0;
unsigned char lowest_lastbyteerror=0;


		//fseek(fptr1, startoffset, SEEK_SET);		
		//firstsample=getc(fptr1);  Replace with file read into array for speed.
		firstsample=filedata[startoffset];
		filepos=startoffset+1;

if (save==1) {		
		fseek(fptr3, 0, SEEK_SET);
		putc(firstsample, fptr3); //Copy first sample
}


// Exceptions for first frame ------------------------------------------------------------------------------
i=0; while (i<16) { readbuffer[i]=filedata[filepos+i]; i++; } i=0;  // Read in first frame into array

//framectr=255;
framectr=0;
lookupset=0;

frame=0;
totalerrors=0;

i=0; while (i<256) { frameerror[i]=0; i++; }  //Reset frameerrors between passes
i=0; while (i<256) { peak_frameerror[i]=0; i++; }  //Reset frameerrors between passes
i=0; while (i<256) { calcedsample[i]=firstsample; i++; }   //Copy first sample to all 256 "old sample" buffer
// -----------------------------------------------------------------------------------------------------------

while (filepos<filelength+16 && skip==0) {
				
 		
		
				if (framectr>15) {  // Advance lookupset when frame has been processed
													framectr=0;
													
													//printf("Filepos %d",(16*frame)+1); 
													//printf(" lookupset %d\n",lookupset);
													lookupset++; 
				}
				//framectr++;
		
filepos=startoffset+(frame*16)+framectr+1;			
			
			
if (lookupset>no_oflookupsets) {		//All lookupsets checked, now find best result

		lowest_errorset=0;
		lowest_error=4096; // Frame max error = 255x16
		skip=0;
		
					i2=0;						
					while (i2<no_oflookupsets+1) {  // Find lookupset with least errors 
									//printf("frame %d",frame); printf(" lookupset %d",i2); printf(" calcedsample %d", calcedsample[i2]); printf(" error count %d\n", frameerror[i2]);
									if (frameerror[i2]<lowest_error) { 
																		if (lastbyte_error[i2]<5) {
																		lowest_error=frameerror[i2]; 
																		lowest_errorset=i2; } 
																	//printf("set %d",i2);
																	//printf(" frameerror %d",frameerror[i2]);
																	//printf(" lastbyte_error %d",lastbyte_error[i2]); 
																	//printf(" lowest errorset %d\n",lowest_errorset); 
																	}
									frameerror[i2]=0;   // Reset frameerrors
									i2++;
					}
					//printf("frame %d",frame);	 printf(" lowest error lookupset %d\n",lowest_errorset);
					
					
					if (oldfilepos>filepos) { distance=(filelength-oldfilepos)+(filepos-startoffset); }
					if (oldfilepos<filepos) { distance=filepos-oldfilepos; }
					
					if (lowest_error>errorthreshold && lowest_error<(errorthreshold+25) && frame>errormatchframe) { // Search for frame exceeding threshold for lookup-generation
															if (distance>distanceth) {		                
															errormatchframe=frame;
															oldfilepos=filepos;
															skip=1;
															printf("Frame %d",errormatchframe); 
															printf(" distance %d",distance);
															printf(" errors %d",lowest_error); 
															printf(" th %d\n",errorthreshold); 
															}
					}
															
					totalerrors=totalerrors+lowest_error;
					
					
					distortion=lowest_error; distortion=distortion/4096;
					if (distortion>highestdist) { highestdist=distortion; }
					
					if (dc>15) { dc=0; dc2=0; while (dc2<16) { dispdist=dispdist+prevdistortion[dc2]; dc2++; } dispdist=dispdist/16; }
					prevdistortion[dc]=distortion; dc++;

		frame++; 
		lookupset=0;

							if (save==1) {  if (frame>totalframes) { if (spillbytes>0) { savebestframe(lowest_errorset, spillbytes); 
																								printf("  frame: %d\n", frame); }}
											if (frame<=totalframes) { savebestframe(lowest_errorset, 16); }
											//printf("  S-pos: %d\n",filepos);  
							}
											
							// Swap in last calced sample from best set to all samplesets
							sampleswapper=calcedsample[lowest_errorset];
							i=0; while (i<256) { calcedsample[i]=sampleswapper; i++; } 
										
							filepos=startoffset+(frame*16)+1; 
							i=0; while (i<16) { readbuffer[i]=filedata[filepos+i]; i++; } 

										
}			
	
			if (skip!=1) {
				nextsample=readbuffer[framectr]; // Load in next sample
					lkptblmatch=matchfinder(calcedsample[lookupset], nextsample, lookupset, framectr);
						calcedsample[lookupset] = calcedsample[lookupset]+ltable[(lookupset*16)+lkptblmatch];
				framectr++; }
				
				// if (skip==1) { filepos=filelength; skip=0; } // Skip rest of pass if errorthreshold has been exceeded.
		
}

printf("  Totalerrors: %d\n",totalerrors); 
printf("  Avg. 16 frame distortion: %4.2f",dispdist*100); printf(" %\n"); 
printf("  Highest distortion: %4.2f",highestdist*100); printf("%\n");

skip=0;

}

void sortlut(void) {

unsigned int lu_idx=0;
signed int tv;
signed int tu;
unsigned int fr_idx;
unsigned char tblmatchidx;
signed char tblmatchval;

while (lu_idx<17) { newtable[lu_idx]=0; lu_idx++; } 
lu_idx=0;

tv=0;
while (tv<16) {
	
tblmatchidx=16;
tblmatchval=0;
lu_idx=0;


	while (lu_idx<16) {  //Sort negative values

		if (deltatable[lu_idx]<tblmatchval) { tblmatchidx=lu_idx; tblmatchval=deltatable[lu_idx]; }
	
	lu_idx++;  
	}

deltatable[tblmatchidx]=0;
newtable[tv]=tblmatchval;
tv++;
}

tv=0;
while (newtable[tv]!=0) {  				//Find first zero in new table
							tv++;
}
//printf("tv:  %d\n ",tv);


tu=15;
while (tu>tv-1) {

tblmatchidx=16;
tblmatchval=0;
lu_idx=0;

	while (lu_idx<16) {  //Sort positive values

		if (deltatable[lu_idx]>tblmatchval) { tblmatchidx=lu_idx; tblmatchval=deltatable[lu_idx]; }
	
	lu_idx++;  
	}

deltatable[tblmatchidx]=0;
newtable[tu]=tblmatchval;
//printf("tu:  %d\n ",tu);
tu--;
}	

}

void copytable(void) {
unsigned char lu_idx=0;

lu_idx=0;
while (lu_idx<16) { deltatable[lu_idx]=newtable[lu_idx]; newtable[lu_idx]=0; lu_idx++; }
deltatable[16]=0; 
}

unsigned int fill_lut(unsigned int framelength, unsigned char frame, unsigned char zeroinorigset) {

signed int fz;
signed int lz;
signed int tv;

signed int dtp;
signed int match;
signed char zeroes;
signed char calcedsample;
unsigned char lv=0;
	

fz=0;
while (newtable[fz]!=0 && fz<17) {  				//Find first zero in new table
							fz++;
}
//printf("fz:  %d\n ",fz);

lz=15;
while (newtable[lz]!=0 && lz>0) {  				//Find last zero in new table
							lz--;
}
//printf("lz:  %d\n ",lz);


zeroes=0;
if (fz<16 && lz>0) { zeroes=lz-fz; }

filepos=startoffset+((errormatchframe+frame)*16);	//Advance one frame
calcedsample=filedata[filepos];

 
//printf("Exframe %d:", frame);
lv=1; while (lv<17) { deltatable[lv-1]=filedata[filepos+lv]-calcedsample;

					  //printf(" %d", deltatable[lv-1]); 
					  calcedsample=filedata[filepos+lv]; lv++; } 
					  //printf("\n"); 
					  lv=0;

if (zeroes>zeroinorigset-1) {

dtp=0;
while (dtp<16 && zeroes>zeroinorigset-1) {
		
		match=0;
		tv=0;
		while (tv<16) {										//Copy in additional values from next frame if possible
							if (newtable[tv]==deltatable[dtp]) { match=1; } 
							if (newtable[tv] < -50) { if (newtable[tv]==deltatable[dtp]-1 || newtable[tv]==deltatable[dtp]+1) { match=1; }}	
							if (newtable[tv] > 50) { if (newtable[tv]==deltatable[dtp]-1 || newtable[tv]==deltatable[dtp]+1) { match=1; }}								
		tv++;
		}
		
	if (match==0) { newtable[lz-zeroes]=deltatable[dtp]; zeroes--; }	

//printf("zeroes:  %d\n ",zeroes);
dtp++;
if (zeroes>zeroinorigset-1) { framelength++; }
}

/*
printf("Afframe: "); 
lv=1; while (lv<17) { 
						printf(" %d", newtable[lv-1]); 
						lv++; } 
						printf("\n"); 
						lv=0;
*/
copytable();
sortlut();
}
return framelength;	
}



void lookupgenerator(unsigned int lset) {

unsigned int lu_idx=0;
signed int tv;
signed int tu;
signed char diff;
unsigned int fr_idx;
unsigned char tblmatchidx;
signed char tblmatchval;
signed char calcedsample;
unsigned char zeroinorigset=0;
signed char bestlookupvalue;
unsigned char lv=0;
unsigned int framelength=16;
unsigned char oldframelength=15;

lset++; 


filepos=startoffset+(errormatchframe*16);
printf("Generating lookup for frame %d", errormatchframe); printf(" filepos %d", filepos+1); printf(" filelength %d\n", filelength);
printf("Frame:  %d ",filedata[filepos]); lv=1; while (lv<17) { printf(" %d", filedata[filepos+lv]); lv++; } printf("\n"); lv=0;
printf("Deltas: "); 
calcedsample=filedata[filepos]; 
lv=1; while (lv<17) { deltatable[lv-1]=filedata[filepos+lv]-calcedsample;
						if (deltatable[lv-1]==0) { zeroinorigset=1; }	//Frame contains zero
						printf(" %d", deltatable[lv-1]); 
						calcedsample=filedata[filepos+lv]; lv++; } 
						printf("\n"); lv=0;

sortlut();

/*
printf("Sort 1: ");
lv=1; while (lv<17) { printf(" %d", newtable[lv-1]); lv++; } printf("\n"); lv=0;
*/

tv=0;
while (tv<16) {			//Prune table of values close to each other, save lower value

				if (newtable[tv]==newtable[tv+1]) { newtable[tv]=0; } //Remove duplicates
tv++;
}

/*
printf("Dupes:  ");
lv=1; while (lv<17) { printf(" %d", newtable[lv-1]); lv++; } printf("\n"); lv=0;
*/

copytable();
sortlut();

/*
printf("Sort 2: ");
lv=1; while (lv<17) { printf(" %d", newtable[lv-1]); lv++; } printf("\n"); lv=0;
*/

tv=0;
while (tv<16) {			//Prune table of values close to each other, save lower value
				
				diff=newtable[tv+1]-newtable[tv];
				
				if (newtable[tv]<-70) { if (diff<4 && diff>0) { 
																/*
																printf("nv1: %d ",newtable[tv]); 
																printf("nv2: %d ",newtable[tv+1]); 
																printf("diff: %d",diff); 
																printf(". "); 
																*/
																newtable[tv]=0; 
																//printf("hitn\n"); 
										} 
				}
				
				if (newtable[tv]>70) { if (diff<4 && diff>0) { 
																/*
																printf("pv1: %d ",newtable[tv]); 
																printf("pv2: %d ",newtable[tv+1]); 
																printf("diff: %d",diff); 
																printf(". "); 
																*/
																newtable[tv+1]=0; 
																//printf(" hitp\n"); 
									   } 
				}
tv++;	
}

copytable();
sortlut();

/*
printf("Prune:  ");
lv=1; while (lv<17) { printf(" %d", newtable[lv-1]); lv++; } printf("\n"); lv=0;
*/

tv=1;
while (tv<42 && oldframelength!=framelength) {		
												copytable();
												sortlut();
												oldframelength=framelength;
												framelength=fill_lut(framelength, tv, zeroinorigset);
												tv++;
}


tv=0; 
while (tv<16) {			//Copy new LUT to main LUT array

				ltable[(16*lset)+tv]=newtable[tv];
tv++;	
}

 printf("New LUT %d: ",lset); lv=0; while (lv<16) { printf(" %d", ltable[(16*lset)+lv]); lv++; } printf("\n");
 printf("Search distance: %d",framelength); printf(" bytes\n");
}



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

void passhandler(unsigned int passno, unsigned int totalpasses, unsigned long distanceth2) {

unsigned char div=0;
highestdist=0;
	
	while (passno<totalpasses) { 
						olderrormatchframe=errormatchframe;
						encoder(passno, errorthreshold, distanceth2);
						if (olderrormatchframe==errormatchframe) { 	errormatchframe=1;						
																if (errorthreshold>12) { errorthreshold=errorthreshold-1; } 
																if (errorthreshold<13) { if (distanceth2>1024) { distanceth2-=64; } }
																encoder(passno, errorthreshold, distanceth2);
																highestdist=0;
																}

						printf("encoder done pass: "); printf("%d\n", passno);
						if (passno>0) { lookupgenerator(passno); }
						if (div>6) { if (errorthreshold>6) { errorthreshold=errorthreshold-1; } div=255; }
						div++;
						//errormatchframe=0;
						passno++; 
						}					
						
						errorthreshold=4096;
						save=1; printf("Save last pass.\n ");
						encoder(totalpasses, errorthreshold, mindistance); // Last pass
	
}

void statdecoder(void) {
	
unsigned char lookupset=0;
unsigned int j;
unsigned char deltadata; 
unsigned char sampledata;
unsigned long codedfilelength=0;
unsigned long fileidx=0;


fseek(fptr3, 0, SEEK_SET); // Seek to beginning in encoded audio-data
while ((dummy = getc(fptr3)) != EOF) { codedfilelength++; }

		fseek(fptr3, 0, SEEK_SET); // Seek to beginning in encoded audio-data
		sampledata=getc(fptr3);  fileidx++; //First sample

	while (j<256) { stats[j]=0; j++; }	//Reset stats
	j=0;

printf("Entering stat decoder.\n ");

while (fileidx<codedfilelength) {

lookupset=getc(fptr3); fileidx++;

stats[lookupset]=stats[lookupset]+1;

			j=0;
			while (j<16 && fileidx<codedfilelength) {
							deltadata=fgetc(fptr3); fileidx++;	
			j++;
			}
	
}
printf("Read %d ", codedfilelength); printf(" bytes\n");
printf("Processed %d ", fileidx); printf(" bytes\n");
}



unsigned int lu_reorder(void) {

unsigned int c=0;
unsigned int c2=0;
unsigned char c3=0;
unsigned int matchval;
unsigned char match=0;
unsigned char pass;
signed char newtable[4096];
unsigned char ones=0;

printf("Entering Lookup-reordering.\n ");

matchval=65535;
c3=0;
while (matchval>0) {
	c2=0; //printf("matchval: %d\n", matchval);
	match=0;
	while (c2<256) {
					if (stats[c2]==matchval) {	stats[c2]=0; match=1; if (matchval>0 && matchval<3) { ones++; }
											printf("Set: %d", c2); printf(" new order: %d", c3); printf(" times used: %d\n", matchval);
														
											c=0; while (c<16) { newtable[(c3*16)+c]=ltable[(c2*16)+c]; c++; } // Copy LU-entry to new table

											c3++; pass=c3; 	
					}
	c2++;
	} 
if (match==0) { matchval--; }	
}

if (ones>6) { ones=ones/3; }

c2=0;
while (c2<pass) { 

					c=0; while (c<16) { ltable[(c2*16)+c]=newtable[(c2*16)+c]; c++; } // Copy back reordered LU-entry to old table
c2++;
}	

c2=pass-ones; // Cull some sets only used once
pass=c2;
printf(" ones: %d\n", ones); 
printf(" new pass: %d\n", c2);
printf(" old pass: %d\n", pass);
pass=c2;
while (c2<256) { 

					c=0; while (c<16) { ltable[(c2*16)+c]=0; c++; } // Reset unused sets to 0
c2++;
}	

printf("Exiting LU-reorder. Next pass: %d\n ",pass);
return pass;	
}


int openencodedasread(void) {
	
	    fclose(fptr3);  
	fptr3 = fopen("encoded.tmp", "rb"); printf("encoded.tmp\n"); 
	if (fptr3 == NULL) {
	printf("encoded.tmp can not be opened\n");
        return 1;
    }
}


int openencodedaswrite(void) {

    fclose(fptr3);  
	fptr3 = fopen("encoded.tmp", "wb"); //printf("encoded.tmp\n"); 
	if (fptr3 == NULL) {
	printf("encoded.tmp file can not be opened\n");
        return 1;
    }
	
}

// Filename & mode given as the command line argument
int main(int argc, char* argv[])
{

unsigned int il=0;
unsigned int passno;
unsigned char c3=0;
int optcheck=1;

unsigned int totalpasses;

if (argc>2) { optcheck=strcmp (argv[2], "-m"); } if (optcheck==0) { modmode=1; }


while (il<4096) { ltable[il]=0; il++; }
il=0;


while (il<256) { ltable[il]=fbntable[il]; il++; }
il=0;

	printf("Adaptive Delta-(Fibonacci) encoder v0.98\n");
	printf(" Hemiyoda hemiyoda@gmail.com 2024 \n\n");
	printf(" Usage: dfaenc filename [-m]  \n\n");
	printf("filename"); printf("		Soundfile in signed 8-bit format or .mod file (Noisetracker/Protracker)\n");
	printf("-m"); printf("			extract samples from .mod file and output packed samples and packed mod file\n");
	printf("\n");
	
if (argc<2) { printf( "Filename missing, exiting.\n"); return 1; }

    // If the file exists and has
    // read permission
	fptr1 = fopen(argv[1], "rb");

    if (fptr1 == NULL) {
	printf("PCM/MOD file can not be opened\n");
        return 1;
    }

	fptr2 = fopen("lookup.dat", "wb");  
	fptr3 = fopen("encoded.tmp", "wb"); 

	    
	if (fptr2 == NULL) {
	printf("lookup.dat can not be opened\n");
        return 1;
    }
	if (fptr3 == NULL) {
	printf("encoded.tmp can not be opened\n");
        return 1;
    }
	
		startoffset=0;
		if (modmode>0) { startoffset=modparser(); }
		
		fseek(fptr1, 0, SEEK_SET); // Seek to beginning of raw amiga audio-data
		fseek(fptr3, 0, SEEK_SET); // Seek to beginning in encoded audio-data
		
		while (filelength<2097152) { filedata[filelength]=0; filelength++; } filelength=0;  //Clear filedata-array
		while ((dummy = getc(fptr1)) != EOF) { filedata[filelength]=dummy; filelength++; } //Read whole file into array.		

		totalsamples=filelength-startoffset;
		mindistance=totalsamples/64;
		totalframes=((totalsamples-1)/16);
		spillbytes=(totalsamples-1)-(totalframes*16);
		
		printf("filelength=%d\n",filelength);
		printf("startoffset=%d\n",startoffset);
		printf("samplelength=%d\n",totalsamples);
		printf("totalframes=%d\n",totalframes);
		printf("spillbytes=%d\n",spillbytes);
		printf("mindistance=%d\n\n",mindistance);

mindistance=totalsamples/64;
save=0;
passno=16;
totalpasses=64; //64
errorthreshold=20; //20
passhandler(passno, totalpasses, mindistance);						


openencodedasread();
statdecoder();
passno=lu_reorder();   //Optimize lookup table and discard seldomly used sets


openencodedaswrite();

save=0;
errorthreshold=65;
totalpasses=110;
mindistance=totalsamples/128;
passhandler(passno, totalpasses, mindistance);  //Do second pass to find better values		


openencodedasread();
statdecoder();
passno=lu_reorder();   //Optimize lookup table and discard seldomly used sets
openencodedaswrite();

save=0;
totalpasses=180;
errorthreshold=17;
mindistance=totalsamples/220;
passhandler(passno, totalpasses, mindistance);  //Do fourth pass to find better values		


openencodedasread();
statdecoder();
passno=lu_reorder();   //Optimize lookup table and discard seldomly used sets
openencodedaswrite();

save=0;
totalpasses=235;
errorthreshold=85;
mindistance=totalsamples/4096;
passhandler(passno, totalpasses, mindistance);  //Do fifth pass to find better values		


openencodedasread();
statdecoder();
passno=lu_reorder();   //Optimize lookup table and discard seldomly used sets
openencodedaswrite();

save=0;
totalpasses=255;
errorthreshold=20;
mindistance=totalsamples/256;
passhandler(passno, totalpasses, mindistance);  //Do sixth pass to find better values		


openencodedasread();
statdecoder();
passno=lu_reorder();   //Optimize lookup table and discard seldomly used sets
openencodedaswrite();

save=0;
totalpasses=255;
errorthreshold=10;
mindistance=totalsamples/256;
passhandler(passno, totalpasses, mindistance);  //Do seventh pass to find better values		


		
	fseek(fptr2, 0, SEEK_SET); // Seek to beginning in lookup-data
	
	while (il<4096) { putc(ltable[il], fptr2); il++; } // Write final lookuptable to file.
printf("Write lookup table\n\n "); 
	
    // Close files
    fclose(fptr2);  
    fclose(fptr3);  

//Enter bitpacker
bitpacker(totalsamples, startoffset, modmode);

printf("All done successfully!\n\n ");

    return 0;
}