// A small header file that defines naming conventions for library symbols.

#ifndef LIBSCA_SYMBOLS_H
#define LIBSCA_SYMBOLS_H

// "LF" = "Library Function". Prefix for all internal functions.
#define LF(name) __libsca_func_##name
// "LS" = "Library Struct". Prefix for all internal structs.
#define LS(name) __libsca_struct_##name
// "LE" = "Library Enum". Prefix for all internal enums.
#define LE(name) __libsca_enum_##name
// "LG" = "Library Global". Prefix for all internal globals.
#define LG(name) __libsca_global_##name

// "PF" = "Public Function". Prefix for all public functions.
#define PF(name) sca_##name
// "PS" = "Public Struct". Prefix for all public structs.
#define PS(name) sca_##name
// "PE" = "Public Enum". Prefix for all public enums.
#define PE(name) sca_##name
// "PG" = "Public Global". Prefix for all public globals.
#define PG(name) sca_##name

#endif

