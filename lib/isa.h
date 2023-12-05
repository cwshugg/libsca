// A small header file that's used to detect the host machine's ISA (Instruction
// Set Architecture). This is used to determine how to behave in certain parts
// of my code. It's implemented entirely in C-preprocessor so I can compile only
// the C code I need for a specific ISA into the binary.
//
// Thanks for FreakAnon on StackOverflow for the code he wrote on this post:
// https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time

#ifndef LIBSCA_ISA_H
#define LIBSCA_ISA_H

// Global #defines for tracking the ISA (defined below)
//  ISA             int         The identifier of the machine's ISA
//  ISA_NAME        string      The name of the machine's ISA
//  ISA_VERSION     int         The version number (if applicable) of the ISA
//  ISA_BITS        int         The core bit-width of the ISA

// ISA key value definitions
#define ISA_X86         0x1
#define ISA_ARM         0x2
#define ISA_MIPS        0x3

// Name definitions
#define ISA_NAME_X86    "x86"
#define ISA_NAME_ARM    "arm"
#define ISA_NAME_MIPS   "mips"

// ====================== Pre-Processing ISA Detection ====================== //
#if defined(__x86_64__) || defined(_M_X64)
    #define ISA ISA_X86
    #define ISA_NAME ISA_NAME_X86
    #define ISA_VERSION 0
    #define ISA_BITS 64
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    #define ISA ISA_X86
    #define ISA_NAME ISA_NAME_X86
    #define ISA_VERSION 0
    #define ISA_BITS 32
#elif defined(__ARM_ARCH_2__)
    #define ISA ISA_ARM
    #define ISA_NAME ISA_NAME_ARM
    #define ISA_VERSION 2
    #define ISA_BITS 32
#elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
    #define ISA ISA_ARM
    #define ISA_NAME ISA_NAME_ARM
    #define ISA_VERSION 3
    #define ISA_BITS 32
#elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
    #define ISA ISA_ARM
    #define ISA_NAME ISA_NAME_ARM
    #define ISA_VERSION 4
    #define ISA_BITS 32
#elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
    #define ISA ISA_ARM
    #define ISA_NAME ISA_NAME_ARM
    #define ISA_VERSION 5
    #define ISA_BITS 32
#elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_) defined(__ARM_ARCH_6__) || \
      defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || \
      defined(__ARM_ARCH_6ZK__)
    #define ISA ISA_ARM
    #define ISA_NAME ISA_NAME_ARM
    #define ISA_VERSION 6
    #define ISA_BITS 32
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    #define ISA ISA_ARM
    #define ISA_NAME ISA_NAME_ARM
    #define ISA_VERSION 7
    #define ISA_BITS 32
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ISA ISA_ARM
    #define ISA_NAME ISA_NAME_ARM
    #define ISA_VERSION 8
    #define ISA_BITS 64
#elif defined(mips) || defined(__mips__) || defined(__mips)
    #define ISA ISA_MIPS
    #define ISA_NAME ISA_NAME_MIPS
    #define ISA_VERSION 0
    #define ISA_BITS 32
#else
    #define ISA 0x0
    #define ISA_NAME "UNKNOWN"
    #define ISA_VERSION 0
    #define ISA_BITS 0
#endif

// Error-checking
#if !(ISA == ISA_X86 || ISA == ISA_ARM)
    #error "Unsupported instruction set architecture. Only Intel x86 and Arm are supported."
#endif

#endif

