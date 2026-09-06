// Shared Win32 VERSIONINFO body -- no headers, raw RC so windres needs no real
// preprocessing. A wrapper .rc #defines RC_FILEDESC / RC_ORIGNAME (and, for an
// executable, RC_MANIFEST / RC_FILETYPE) and #includes this. windres compiles
// the result to a COFF .res the linker embeds: proper PE metadata for Windows
// Defender / SmartScreen heuristics (real trust still needs Authenticode signing).

#ifndef RC_FILETYPE
    #define RC_FILETYPE 0x1L   /* VFT_APP; a DLL wrapper #defines 0x2L */
#endif

#ifdef RC_MANIFEST
1 24 "app.manifest"
#endif

1 VERSIONINFO
FILEVERSION    0,1,0,0
PRODUCTVERSION 0,1,0,0
FILEFLAGSMASK  0x3fL
FILEFLAGS      0x0L
FILEOS         0x40004L
FILETYPE       RC_FILETYPE
FILESUBTYPE    0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName",      "MGEngine"
            VALUE "ProductName",      "MGEngine"
            VALUE "ProductVersion",   "0.1.0.0"
            VALUE "FileVersion",      "0.1.0.0"
            VALUE "FileDescription",  RC_FILEDESC
            VALUE "InternalName",     RC_ORIGNAME
            VALUE "OriginalFilename", RC_ORIGNAME
            VALUE "LegalCopyright",   "MGEngine"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END
