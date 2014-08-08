This folder contains third party fonts shipped under various licenses
including the Open Font License and Arphic Public License.  Full license
information is included in each subdirectory along with fonts converted for use
with the TivaWare Graphics Library as both C source files and binary files.

For large character sets such as Chinese and Japanese, the C format files may
contain only a subset of characters for illustration purposes.  The very
much larger binary files do, however, contain a full (or at least usable) set
of characters.

Fonts currently included are:

ofl/fonts/fonthandwriting - a handwriting script font converted from "Breip."
                            Characters U+0020-U+00FF are supported.

ofl/fonts/fonthangul      - a Korean Hangul font converted from "Nanum". C
                            versions include ASCII and Korean Jamo only.
                            Binary versions also include Hangul syllables in
                            the U+AC00-U+D7A3 range.

ofl/fonts/fontoldstandard - a Latin font converted from "OldStandard"
                            Characters U+0020-U+00FF are supported.

ofl/fonts/fontsansand     - a Latin font converted from "Andika"
                            Characters U+0020-U+00FF are supported.

ofl/fonts/fonttheano      - a Latin font converted from "Theano"
                            Characters U+0020-U+00FF are supported.

apl/fonts/fireflysung     - a Chinese font converted from "FireflySung"
                            C versions include ASCII and the first 256
                            ideographs from U+4E00. Binary versions include
                            all available ideographs from U+4E00-U+9FFF.

other/fonts/fontsazanami  - a Japanese font converted from "Sazanami". C
                            versions include ASCII, Katakana and Hiragana.
                            Binary versions add Kanji.

lang_demo/fontcustom      - a custom font built specifically for use with
                            the lang_demo example application included in
                            many releases of TivaWare.  These fonts
                            contain only the required glyphs to support
                            the string table used by this application.