//*****************************************************************************
//
// apl_fonts.h - Public header declaring prototypes for the various extended
//               fonts found under third_party/fonts/apl.
//
//*****************************************************************************
#ifndef __APL_FONTS_H__
#define __APL_FONTS_H__

//*****************************************************************************
//
// Unified Chinese/Japanese/Korean ideagraphs + ASCII.  These fonts contain
// the first 256 ideographs from Unicode U+4E00 to U+0x4EFF range and are
// intended for format illustration purposes only.  Binary fonts containing
// glyphs between U+4E00 and U+9FFF can be found in the binfonts subdirectory.
//
//*****************************************************************************
extern const unsigned char g_pucFireflysung6x12[];
#define g_pFireflysung6x12 (const tFont *)g_pucFireflysung6x12
extern const unsigned char g_pucFireflysung6x14[];
#define g_pFireflysung6x14 (const tFont *)g_pucFireflysung6x14
extern const unsigned char g_pucFireflysung7x14[];
#define g_pFireflysung7x14 (const tFont *)g_pucFireflysung7x14
extern const unsigned char g_pucFireflysung7x15[];
#define g_pFireflysung7x15 (const tFont *)g_pucFireflysung7x15

#endif // __APL_FONTS_H__
