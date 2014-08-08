#******************************************************************************
#
# Makefile - Makefile rules to build the custom font required by the 
#            lang_demo example application.
#
# Copyright (c) 2008 Texas Instruments Incorporated.  All rights reserved.
# TI Information - Selective Disclosure
#
#******************************************************************************

#
# Build the C version of the images and the language table.
#
all: language.c langremap.c fontcustom14pt.c fontcustomr14pt.c fontcustom20pt.c fontcustomr20pt.c

#
# Build binary versions of each of the fonts used by the application.  This is
# purely for test purposes since these can be placed on an SDCard (renamed to
# FONT.BIN) to allow their contents to be viewed using the fontview application.
#
binfonts: fontcustom14pt.bin fontcustomr14pt.bin fontcustom20pt.bin fontcustomr20pt.bin
 
#
# Build our string table from the CSV file.
#
language.c: language.csv
	 ../../../tools/bin/mkstringtable -u -f language.csv -b language -s utf8 -t

#
# Build our remapped string table from the CSV file.
#
langremap.c: language.csv
	 ../../../tools/bin/mkstringtable -u -c 0x8000 -f language.csv -b langremap -s utf8 -r

#
# Build the custom font used by the application based on the output of the
# mkstringtable step.  We build 2 fonts, one for use with the remapped 
# string table and the other indexed using normal Unicode codepoints.
#
fontcustom14pt.c: language.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c language.txt -r -f custom -s 14 AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf

fontcustomr14pt.c: langremap.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c langremap.txt -r -z 0x8000 -f customr -s 14 AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf

fontcustom20pt.c: language.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c language.txt -r -f custom -s 20 AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf

fontcustomr20pt.c: langremap.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c langremap.txt -r -z 0x8000 -f customr -s 20 AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf

#
# Build binary versions of the custom fonts for test purposes.  This allows us
# to check the contents of the font by placing the file on an SDCard and running
# the fontview application.
#
fontcustom14pt.bin: language.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c language.txt -r -f custom -s 14 -y AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf

fontcustomr14pt.bin: langremap.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c langremap.txt -r -z 0x8000 -f customr -s 14 -y AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf

fontcustom20pt.bin: language.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c language.txt -r -f custom -s 20 -y AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf

fontcustomr20pt.bin: langremap.c AndBasR.ttf fireflysung.ttf  NanumMyeongjo.ttf sazanami-gothic.ttf
	../../../tools/bin/ftrasterize -u -c langremap.txt -r -z 0x8000 -f customr -s 20 -y AndBasR.ttf fireflysung.ttf NanumMyeongjo.ttf sazanami-gothic.ttf
	
	
#
# Copy font files into the local directory if needed.
# 
fireflysung.ttf:
	cp ../apl/ttf/fireflysung.ttf .

sazanami-gothic.ttf:
	cp ../other/ttf/sazanami-gothic.ttf .

NanumMyeongjo.ttf:
	cp ../ofl/ttf/NanumMyeongjo.ttf .

AndBasR.ttf:
	cp ../ofl/ttf/AndBasR.ttf .
#
# Clean out the things produced by this makefile.
#
clean:
	@rm -f language.c language.h language.txt fontcustom14pt.c fontcustom20pt.c
	@rm -f langremap.c langremap.h langremap.txt fontcustomr14pt.c fontcustomr20pt.c
	@rm -f fontcustom14pt.bin fontcustom20pt.bin f fontcustomr14pt.bin fontcustomr20pt.bin

