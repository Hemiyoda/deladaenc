Adaptive Delta (Fibonacci) encoder suite v0.97  
----------------------------------------------
An 8-bit lossy sound compression format. Uses 8-bit delta values called from a 4K lookup table.
A format specifically targeted for Amiga computers with sample sizes <512Kb.
Aims for trivial decoding complexity with decent compression and audio quality.

Inspired by Steve Hayes Fibonacci Delta sound compression technique

 - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  <hemiyoda@gmail.com> wrote this file. 
  If you find any use for this kludge it would be fantastic with a shout-out.
  Signed: Erik Berg aka Hemiyoda / DMS       2024
 - - -  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

-- Version History --
v0.97 First public release, only 4K LUT implemented. Still room for improvements. There may be bugs.
 
 
    	File format explanation, see decompressor: deladadec.c
 
 
 	deladaenc.c		  The encoder, try: deladaenc freshhouse.mod -m
 	deladadec.c		  The decoder, try: deladadec encoded.df+ 
 	unladamod.c		  Decoder for mod-file with compressed data from deladaenc. Try: unladamod packedmod.dfm 
	differ.c		     Diffs 2 files, useful for evaluating compression artifacts.
	modmodder.c		  Inserts unpacked sample data into modfile. For comparing sound quality with other compressors
 
 	Windows gcc compile:         gcc deladaenc.c -o deladaenc.exe -O3
 	Windows/Amiga gcc crossdev:  m68k-amigaos-gcc unladamod.c -o unladamod -Os -noixemul
 	(note that the resulting amiga executables seem to require a very large stack to run)
  
 	The encoder/compressor is extremely slow due to it's brute force nature.
 
 	In the next version: 	-Cull close values (the current implementation is buggy). 
							-Include pre-calced "statistical" LUTs.
							-Your suggestion  
 
 
 	Greetz to the whole Amiga scene! Erik 'Hemiyoda' Berg 
 
  
