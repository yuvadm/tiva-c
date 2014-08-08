//*****************************************************************************
// 
// langremap.h - header file for a compressed string table.
//
// Copyright (c) 2008 Texas Instruments Incorporated.  All rights reserved.
// TI Information - Selective Disclosure
//
// This is an auto-generated file.  Do not edit by hand.
//
//*****************************************************************************

#define SCOMP_MAX_STRLEN         202   // The maximum size of any string.

extern const uint8_t g_pui8Tablelangremap[];

//*****************************************************************************
//
// SCOMP_STR_INDEX is an enumeration list that is used to select a string
// from the string table using the GrStringGet() function.
//
//*****************************************************************************
enum SCOMP_STR_INDEX
{
    STR_ENGLISH,                  
    STR_ITALIANO,                 
    STR_DEUTSCH,                  
    STR_ESPANOL,                  
    STR_CHINESE,                  
    STR_KOREAN,                   
    STR_JAPANESE,                 
    STR_APPNAME,                  
    STR_PLUS,                     
    STR_MINUS,                    
    STR_CONFIG,                   
    STR_INTRO,                    
    STR_UPDATE,                   
    STR_LANGUAGE,                 
    STR_INTRO_1,                  
    STR_INTRO_2,                  
    STR_INTRO_3,                  
    STR_UPDATE_TEXT,              
    STR_UPDATING,                 
    STR_UART,                     
    STR_MAC,                      
    STR_HEXDIGITS,                
};

//*****************************************************************************
//
// Mappings from original codepage to remapped character codes.
//
//*****************************************************************************
#define MAP8000_CHAR_000020 0x1
#define MAP8000_CHAR_000065 0x2
#define MAP8000_CHAR_000061 0x3
#define MAP8000_CHAR_000069 0x4
#define MAP8000_CHAR_00006e 0x5
#define MAP8000_CHAR_000074 0x6
#define MAP8000_CHAR_00006c 0x7
#define MAP8000_CHAR_00006f 0x8
#define MAP8000_CHAR_000072 0x9
#define MAP8000_CHAR_000073 0xa
#define MAP8000_CHAR_000064 0xb
#define MAP8000_CHAR_000075 0xc
#define MAP8000_CHAR_000063 0xd
#define MAP8000_CHAR_000067 0xe
#define MAP8000_CHAR_000068 0xf
#define MAP8000_CHAR_000070 0x10
#define MAP8000_CHAR_00006d 0x11
#define MAP8000_CHAR_000066 0x12
#define MAP8000_CHAR_00002d 0x13
#define MAP8000_CHAR_000031 0x14
#define MAP8000_CHAR_000062 0x15
#define MAP8000_CHAR_000041 0x16
#define MAP8000_CHAR_000030 0x17
#define MAP8000_CHAR_00003a 0x18
#define MAP8000_CHAR_000045 0x19
#define MAP8000_CHAR_00007a 0x1a
#define MAP8000_CHAR_00002e 0x1b
#define MAP8000_CHAR_000054 0x1c
#define MAP8000_CHAR_000077 0x1d
#define MAP8000_CHAR_00002b 0x1e
#define MAP8000_CHAR_000032 0x1f
#define MAP8000_CHAR_000035 0x20
#define MAP8000_CHAR_000038 0x21
#define MAP8000_CHAR_000049 0x22
#define MAP8000_CHAR_00004d 0x23
#define MAP8000_CHAR_000044 0x24
#define MAP8000_CHAR_00004c 0x25
#define MAP8000_CHAR_000076 0x26
#define MAP8000_CHAR_000055 0x27
#define MAP8000_CHAR_000079 0x28
#define MAP8000_CHAR_00c5b4 0x29
#define MAP8000_CHAR_000043 0x2a
#define MAP8000_CHAR_00006b 0x2b
#define MAP8000_CHAR_000053 0x2c
#define MAP8000_CHAR_003092 0x2d
#define MAP8000_CHAR_00c774 0x2e
#define MAP8000_CHAR_000033 0x2f
#define MAP8000_CHAR_000034 0x30
#define MAP8000_CHAR_000036 0x31
#define MAP8000_CHAR_000037 0x32
#define MAP8000_CHAR_000039 0x33
#define MAP8000_CHAR_000046 0x34
#define MAP8000_CHAR_00004e 0x35
#define MAP8000_CHAR_000052 0x36
#define MAP8000_CHAR_00005f 0x37
#define MAP8000_CHAR_0000f1 0x38
#define MAP8000_CHAR_0000f3 0x39
#define MAP8000_CHAR_003001 0x3a
#define MAP8000_CHAR_003002 0x3b
#define MAP8000_CHAR_00306e 0x3c
#define MAP8000_CHAR_004e2d 0x3d
#define MAP8000_CHAR_0056fd 0x3e
#define MAP8000_CHAR_0065e5 0x3f
#define MAP8000_CHAR_00672c 0x40
#define MAP8000_CHAR_007684 0x41
#define MAP8000_CHAR_00ad6d 0x42
#define MAP8000_CHAR_00d55c 0x43
#define MAP8000_CHAR_000057 0x44
#define MAP8000_CHAR_003059 0x45
#define MAP8000_CHAR_0065b0 0x46
#define MAP8000_CHAR_0066f4 0x47
#define MAP8000_CHAR_008a00 0x48
#define MAP8000_CHAR_003057 0x49
#define MAP8000_CHAR_003067 0x4a
#define MAP8000_CHAR_00306f 0x4b
#define MAP8000_CHAR_00308b 0x4c
#define MAP8000_CHAR_0030e9 0x4d
#define MAP8000_CHAR_00b97c 0x4e
#define MAP8000_CHAR_00002c 0x4f
#define MAP8000_CHAR_000042 0x50
#define MAP8000_CHAR_000047 0x51
#define MAP8000_CHAR_00307e 0x52
#define MAP8000_CHAR_0030fc 0x53
#define MAP8000_CHAR_006309 0x54
#define MAP8000_CHAR_00b2e4 0x55
#define MAP8000_CHAR_000027 0x56
#define MAP8000_CHAR_00304c 0x57
#define MAP8000_CHAR_00304d 0x58
#define MAP8000_CHAR_003053 0x59
#define MAP8000_CHAR_003066 0x5a
#define MAP8000_CHAR_003068 0x5b
#define MAP8000_CHAR_00306b 0x5c
#define MAP8000_CHAR_0030a3 0x5d
#define MAP8000_CHAR_0030c6 0x5e
#define MAP8000_CHAR_0030d5 0x5f
#define MAP8000_CHAR_0030ea 0x60
#define MAP8000_CHAR_0030f3 0x61
#define MAP8000_CHAR_007f6e 0x62
#define MAP8000_CHAR_0080fd 0x63
#define MAP8000_CHAR_008a9e 0x64
#define MAP8000_CHAR_008bed 0x65
#define MAP8000_CHAR_009762 0x66
#define MAP8000_CHAR_00b294 0x67
#define MAP8000_CHAR_00bcf4 0x68
#define MAP8000_CHAR_00c5b8 0x69
#define MAP8000_CHAR_00c740 0x6a
#define MAP8000_CHAR_00c9c0 0x6b
#define MAP8000_CHAR_00d2b8 0x6c
#define MAP8000_CHAR_00004b 0x6d
#define MAP8000_CHAR_000050 0x6e
#define MAP8000_CHAR_0000f6 0x6f
#define MAP8000_CHAR_0000fc 0x70
#define MAP8000_CHAR_003042 0x71
#define MAP8000_CHAR_003044 0x72
#define MAP8000_CHAR_003048 0x73
#define MAP8000_CHAR_003055 0x74
#define MAP8000_CHAR_00305f 0x75
#define MAP8000_CHAR_00308c 0x76
#define MAP8000_CHAR_0030a2 0x77
#define MAP8000_CHAR_0030a4 0x78
#define MAP8000_CHAR_0030b7 0x79
#define MAP8000_CHAR_0030bf 0x7a
#define MAP8000_CHAR_0030c3 0x7b
#define MAP8000_CHAR_0030c8 0x7c
#define MAP8000_CHAR_0030cd 0x7d
#define MAP8000_CHAR_0030d1 0x7e
#define MAP8000_CHAR_0030dc 0x7f
#define MAP8000_CHAR_0030eb 0x80
#define MAP8000_CHAR_004e0b 0x81
#define MAP8000_CHAR_004f9b 0x82
#define MAP8000_CHAR_005207 0x83
#define MAP8000_CHAR_0053ef 0x84
#define MAP8000_CHAR_005b9a 0x85
#define MAP8000_CHAR_005e8f 0x86
#define MAP8000_CHAR_0062bc 0x87
#define MAP8000_CHAR_0063d0 0x88
#define MAP8000_CHAR_00677f 0x89
#define MAP8000_CHAR_007528 0x8a
#define MAP8000_CHAR_00793a 0x8b
#define MAP8000_CHAR_0079fb 0x8c
#define MAP8000_CHAR_007a0b 0x8d
#define MAP8000_CHAR_007acb 0x8e
#define MAP8000_CHAR_008a2d 0x8f
#define MAP8000_CHAR_0090e8 0x90
#define MAP8000_CHAR_00914d 0x91
#define MAP8000_CHAR_0094ae 0x92
#define MAP8000_CHAR_00acfc 0x93
#define MAP8000_CHAR_00ad6c 0x94
#define MAP8000_CHAR_00adf8 0x95
#define MAP8000_CHAR_00b110 0x96
#define MAP8000_CHAR_00b2c8 0x97
#define MAP8000_CHAR_00b370 0x98
#define MAP8000_CHAR_00b7ec 0x99
#define MAP8000_CHAR_00b9ac 0x9a
#define MAP8000_CHAR_00bc84 0x9b
#define MAP8000_CHAR_00c131 0x9c
#define MAP8000_CHAR_00c18c 0x9d
#define MAP8000_CHAR_00c218 0x9e
#define MAP8000_CHAR_00c2dc 0x9f
#define MAP8000_CHAR_00c5c5 0xa0
#define MAP8000_CHAR_00c5ec 0xa1
#define MAP8000_CHAR_00c640 0xa2
#define MAP8000_CHAR_00c744 0xa3
#define MAP8000_CHAR_00c788 0xa4
#define MAP8000_CHAR_00c815 0xa5
#define MAP8000_CHAR_00d2bc 0xa6
#define MAP8000_CHAR_00d328 0xa7
#define MAP8000_CHAR_00d504 0xa8
#define MAP8000_CHAR_00d558 0xa9
#define MAP8000_CHAR_00004f 0xaa
#define MAP8000_CHAR_000051 0xab
#define MAP8000_CHAR_000059 0xac
#define MAP8000_CHAR_00005b 0xad
#define MAP8000_CHAR_00005d 0xae
#define MAP8000_CHAR_000071 0xaf
#define MAP8000_CHAR_0000c4 0xb0
#define MAP8000_CHAR_0000e4 0xb1
#define MAP8000_CHAR_0000e9 0xb2
#define MAP8000_CHAR_00201c 0xb3
#define MAP8000_CHAR_00201d 0xb4
#define MAP8000_CHAR_00304f 0xb5
#define MAP8000_CHAR_003050 0xb6
#define MAP8000_CHAR_003058 0xb7
#define MAP8000_CHAR_003060 0xb8
#define MAP8000_CHAR_00306a 0xb9
#define MAP8000_CHAR_003081 0xba
#define MAP8000_CHAR_003089 0xbb
#define MAP8000_CHAR_00308a 0xbc
#define MAP8000_CHAR_0030a6 0xbd
#define MAP8000_CHAR_0030a7 0xbe
#define MAP8000_CHAR_0030af 0xbf
#define MAP8000_CHAR_0030b0 0xc0
#define MAP8000_CHAR_0030b1 0xc1
#define MAP8000_CHAR_0030b5 0xc2
#define MAP8000_CHAR_0030b8 0xc3
#define MAP8000_CHAR_0030b9 0xc4
#define MAP8000_CHAR_0030bd 0xc5
#define MAP8000_CHAR_0030d6 0xc6
#define MAP8000_CHAR_0030d7 0xc7
#define MAP8000_CHAR_0030dd 0xc8
#define MAP8000_CHAR_0030e1 0xc9
#define MAP8000_CHAR_0030e5 0xca
#define MAP8000_CHAR_0030e6 0xcb
#define MAP8000_CHAR_0030e7 0xcc
#define MAP8000_CHAR_004e0e 0xcd
#define MAP8000_CHAR_004e4b 0xce
#define MAP8000_CHAR_004ecb 0xcf
#define MAP8000_CHAR_004ee3 0xd0
#define MAP8000_CHAR_004ee5 0xd1
#define MAP8000_CHAR_004ef6 0xd2
#define MAP8000_CHAR_004f1a 0xd3
#define MAP8000_CHAR_004fe1 0xd4
#define MAP8000_CHAR_0050cf 0xd5
#define MAP8000_CHAR_005141 0xd6
#define MAP8000_CHAR_00529b 0xd7
#define MAP8000_CHAR_0052a8 0xd8
#define MAP8000_CHAR_0052b9 0xd9
#define MAP8000_CHAR_0052d5 0xda
#define MAP8000_CHAR_005373 0xdb
#define MAP8000_CHAR_0053d6 0xdc
#define MAP8000_CHAR_00540e 0xdd
#define MAP8000_CHAR_00548c 0xde
#define MAP8000_CHAR_0054cd 0xdf
#define MAP8000_CHAR_0056fe 0xe0
#define MAP8000_CHAR_005728 0xe1
#define MAP8000_CHAR_005831 0xe2
#define MAP8000_CHAR_005b9e 0xe3
#define MAP8000_CHAR_005c06 0xe4
#define MAP8000_CHAR_005c4f 0xe5
#define MAP8000_CHAR_005e55 0xe6
#define MAP8000_CHAR_005e93 0xe7
#define MAP8000_CHAR_005e94 0xe8
#define MAP8000_CHAR_005e95 0xe9
#define MAP8000_CHAR_005f62 0xea
#define MAP8000_CHAR_005f71 0xeb
#define MAP8000_CHAR_005f8c 0xec
#define MAP8000_CHAR_00606f 0xed
#define MAP8000_CHAR_0060a8 0xee
#define MAP8000_CHAR_0060c5 0xef
#define MAP8000_CHAR_006210 0xf0
#define MAP8000_CHAR_006301 0xf1
#define MAP8000_CHAR_006362 0xf2
#define MAP8000_CHAR_0063db 0xf3
#define MAP8000_CHAR_00652f 0xf4
#define MAP8000_CHAR_006620 0xf5
#define MAP8000_CHAR_00662f 0xf6
#define MAP8000_CHAR_0066ff 0xf7
#define MAP8000_CHAR_00679c 0xf8
#define MAP8000_CHAR_0069cb 0xf9
#define MAP8000_CHAR_006a5f 0xfa
#define MAP8000_CHAR_006b21 0xfb
#define MAP8000_CHAR_006b64 0xfc
#define MAP8000_CHAR_006f14 0xfd
#define MAP8000_CHAR_0073b0 0xfe
#define MAP8000_CHAR_00753b 0xff
#define MAP8000_CHAR_007b80 0x100
#define MAP8000_CHAR_00898b 0x101
#define MAP8000_CHAR_0089c1 0x102
#define MAP8000_CHAR_008bb8 0x103
#define MAP8000_CHAR_008bbe 0x104
#define MAP8000_CHAR_008f6f 0x105
#define MAP8000_CHAR_009593 0x106
#define MAP8000_CHAR_0095ea 0x107
#define MAP8000_CHAR_0095f4 0x108
#define MAP8000_CHAR_00ac00 0x109
#define MAP8000_CHAR_00ac19 0x10a
#define MAP8000_CHAR_00ac1c 0x10b
#define MAP8000_CHAR_00ac83 0x10c
#define MAP8000_CHAR_00ac8c 0x10d
#define MAP8000_CHAR_00acf5 0x10e
#define MAP8000_CHAR_00ae30 0x10f
#define MAP8000_CHAR_00aed8 0x110
#define MAP8000_CHAR_00b204 0x111
#define MAP8000_CHAR_00b20c 0x112
#define MAP8000_CHAR_00b2a5 0x113
#define MAP8000_CHAR_00b2e8 0x114
#define MAP8000_CHAR_00b300 0x115
#define MAP8000_CHAR_00b3d9 0x116
#define MAP8000_CHAR_00b418 0x117
#define MAP8000_CHAR_00b77c 0x118
#define MAP8000_CHAR_00b798 0x119
#define MAP8000_CHAR_00b7a8 0x11a
#define MAP8000_CHAR_00b85c 0x11b
#define MAP8000_CHAR_00b86d 0x11c
#define MAP8000_CHAR_00b974 0x11d
#define MAP8000_CHAR_00b9cc 0x11e
#define MAP8000_CHAR_00ba70 0x11f
#define MAP8000_CHAR_00ba74 0x120
#define MAP8000_CHAR_00bbf8 0x121
#define MAP8000_CHAR_00be0c 0x122
#define MAP8000_CHAR_00c0ac 0x123
#define MAP8000_CHAR_00c0c8 0x124
#define MAP8000_CHAR_00c124 0x125
#define MAP8000_CHAR_00c2ed 0x126
#define MAP8000_CHAR_00c5c8 0x127
#define MAP8000_CHAR_00c5d0 0x128
#define MAP8000_CHAR_00c624 0x129
#define MAP8000_CHAR_00c6a9 0x12a
#define MAP8000_CHAR_00c6d0 0x12b
#define MAP8000_CHAR_00c6e8 0x12c
#define MAP8000_CHAR_00c720 0x12d
#define MAP8000_CHAR_00c73c 0x12e
#define MAP8000_CHAR_00c74c 0x12f
#define MAP8000_CHAR_00c751 0x130
#define MAP8000_CHAR_00c758 0x131
#define MAP8000_CHAR_00c785 0x132
#define MAP8000_CHAR_00c804 0x133
#define MAP8000_CHAR_00c81c 0x134
#define MAP8000_CHAR_00c904 0x135
#define MAP8000_CHAR_00c90d 0x136
#define MAP8000_CHAR_00c989 0x137
#define MAP8000_CHAR_00ccb4 0x138
#define MAP8000_CHAR_00d2f0 0x139
#define MAP8000_CHAR_00d2f8 0x13a
#define MAP8000_CHAR_00d53d 0x13b
#define MAP8000_CHAR_00d560 0x13c
#define MAP8000_CHAR_00d568 0x13d
#define MAP8000_CHAR_00d654 0x13e
#define MAP8000_CHAR_00d658 0x13f
#define MAP8000_CHAR_00d6a8 0x140
#define MAP8000_CHAR_00d6c4 0x141
#define MAP8000_CHAR_00ff0c 0x142
#define MAP8000_CHAR_00ff1a 0x143

//*****************************************************************************
//
// The following global variables and #defines are intended to aid in setting
// up codepage mapping tables for use with this string table and an appropriate
// font.  To use only this string table, GrLibInit may be called with a pointer
// to the g_GrLibDefaultlangremap structure.
//
//*****************************************************************************
extern tCodePointMap g_psCodePointMap_langremap[];

#define GRLIB_CUSTOM_MAP_langremap {0x8000, 0x8000, GrMapUTF8_Unicode}

extern tGrLibDefaults g_sGrLibDefaultlangremap;
