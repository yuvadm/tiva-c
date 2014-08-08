//*****************************************************************************
//
// other_fonts.h - Public header declaring prototypes for the various extended
//                 fonts found under third_party/fonts/other.
//
//*****************************************************************************
#ifndef __OTHER_FONTS_H__
#define __OTHER_FONTS_H__

//*****************************************************************************
//
// Fonts containing ASCII, Hiragana and Katakana characters.  Similarly named
// fonts in the binfonts subdirectory also contain Kanji (and are, of course,
// very much larger).
//
//*****************************************************************************
extern const unsigned char g_pucSazanamigothic10x19[];
#define g_pFontSazanamigothic10x19 \
    (const tFont *)g_pucSazanamigothic10x19
extern const unsigned char g_pucSazanamigothic6x10[];
#define g_pFontSazanamigothic6x10 \
    (const tFont *)g_pucSazanamigothic6x10
extern const unsigned char g_pucSazanamigothic7x13[];
#define g_pFontSazanamigothic7x13 \
    (const tFont *)g_pucSazanamigothic7x13
extern const unsigned char g_pucSazanamigothic8x15[];
#define g_pFontSazanamigothic8x15 \
    (const tFont *)g_pucSazanamigothic8x15
extern const unsigned char g_pucSazanamigothic9x17[];
#define g_pFontSazanamigothic9x17 \
    (const tFont *)g_pucSazanamigothic9x17
extern const unsigned char g_pucSazanamimincho10x19[];
#define g_pFontSazanamimincho10x19 \
    (const tFont *)g_pucSazanamimincho10x19
extern const unsigned char g_pucSazanamimincho6x10[];
#define g_pFontSazanamimincho6x10 \
    (const tFont *)g_pucSazanamimincho6x10
extern const unsigned char g_pucSazanamimincho7x13[];
#define g_pFontSazanamimincho7x13 \
    (const tFont *)g_pucSazanamimincho7x13
extern const unsigned char g_pucSazanamimincho8x15[];
#define g_pFontSazanamimincho8x15 \
    (const tFont *)g_pucSazanamimincho8x15
extern const unsigned char g_pucSazanamimincho9x17[];
#define g_pFontSazanamimincho9x17 \
    (const tFont *)g_pucSazanamimincho9x17

#endif // __OTHER_FONTS_H__
