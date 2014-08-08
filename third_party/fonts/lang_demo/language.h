//*****************************************************************************
// 
// language.h - header file for a compressed string table.
//
// Copyright (c) 2008 Texas Instruments Incorporated.  All rights reserved.
// TI Information - Selective Disclosure
//
// This is an auto-generated file.  Do not edit by hand.
//
//*****************************************************************************

#define SCOMP_MAX_STRLEN         202   // The maximum size of any string.

extern const uint8_t g_pui8Tablelanguage[];

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
// The following global variables and #defines are intended to aid in setting
// up codepage mapping tables for use with this string table and an appropriate
// font.  To use only this string table, GrLibInit may be called with a pointer
// to the g_GrLibDefaultlanguage structure.
//
//*****************************************************************************
extern tCodePointMap g_psCodePointMap_language[];

#define GRLIB_UNICODE_MAP_language {CODEPAGE_UTF_8, CODEPAGE_UNICODE, GrMapUTF8_Unicode}

extern tGrLibDefaults g_sGrLibDefaultlanguage;
