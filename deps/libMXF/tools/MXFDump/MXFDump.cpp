//=---------------------------------------------------------------------=
//
// $Id$ $Name$
//
// The contents of this file are subject to the AAF SDK Public Source
// License Agreement Version 2.0 (the "License"); You may not use this
// file except in compliance with the License.  The License is available
// in AAFSDKPSL.TXT, or you may obtain a copy of the License from the
// Advanced Media Workflow Association, Inc., or its successor.
//
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
// the License for the specific language governing rights and limitations
// under the License.  Refer to Section 3.3 of the License for proper use
// of this Exhibit.
//
// WARNING:  Please contact the Advanced Media Workflow Association,
// Inc., for more information about any additional licenses to
// intellectual property covering the AAF Standard that may be required
// to create and distribute AAF compliant products.
// (http://www.amwa.tv/policies).
//
// Copyright Notices:
// The Original Code of this file is Copyright 1998-2009, licensor of the
// Advanced Media Workflow Association.  All rights reserved.
//
// The Initial Developer of the Original Code of this file and the
// licensor of the Advanced Media Workflow Association is
// Avid Technology.
// All rights reserved.
//
//=---------------------------------------------------------------------=

// KLV/MXF file dumper.
//
// Tim Bingham - October 2002 - Tim_Bingham@avid.com

// This program is deliberately constructed as one file with no dependencies
// so that it can easily be ported to a new host. You'll be able to make some
// progress understanding an MXF file even if all you have is this source and
// a C++ compiler.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#if defined (_MSC_VER) && defined(_M_IX86) && defined(_WIN32)
#define MXF_COMPILER_MSC_INTEL_WINDOWS
#define MXF_OS_WINDOWS
#elif defined (_MSC_VER) && defined(_M_X64) && defined(_WIN32)
#define MXF_COMPILER_MSC_X64_WINDOWS
#define MXF_OS_WINDOWS
#elif defined(__GNUC__) && defined(__i386__) && defined(_WIN32)
#define MXF_COMPILER_GCC_INTEL_WINDOWS
#define MXF_OS_WINDOWS
#elif defined(__GNUC__) && defined(__i386__) && defined(__linux__)
#define MXF_COMPILER_GCC_INTEL_LINUX
#define MXF_OS_UNIX
#elif defined(__GNUC__) && defined(__x86_64__) && defined(__linux__)
#define MXF_COMPILER_GCC_X86_64_LINUX
#define MXF_OS_UNIX
#elif defined(__GNUC__) && defined(__aarch64__) && defined(__linux__)
#define MXF_COMPILER_GCC_ARM64_LINUX
#define MXF_OS_UNIX
#elif defined(__MWERKS__) && defined(__POWERPC__) && defined(macintosh)
#define MXF_COMPILER_MWERKS_PPC_MACOS
#define MXF_OS_MACOS
#elif defined(__MWERKS__) && defined(__MACH__)
#define MXF_COMPILER_MWERKS_PPC_MACOSX
#define MXF_OS_MACOSX
#elif defined(__GNUC__) && defined(__ppc__) && defined(__APPLE__)
#define MXF_COMPILER_GCC_PPC_MACOSX
#define MXF_OS_MACOSX
#elif defined(__GNUC__) && defined(__i386__) && defined(__APPLE__)
#define MXF_COMPILER_GCC_INTEL_MACOSX
#define MXF_OS_MACOSX
#elif defined(__GNUC__) && defined(__x86_64__) && defined(__APPLE__)
#define MXF_COMPILER_GCC_X86_64_MACOSX
#define MXF_OS_UNIX
#define MXF_COMPILER_GCC_INTEL_X86_64_MACOSX
#define MXF_OS_MACOSX
#elif defined(__GNUC__) && defined(__arm64__) && defined(__APPLE__)
#define MXF_COMPILER_GCC_ARM64_MACOSX
#define MXF_OS_UNIX
#define MXF_OS_MACOSX
#elif defined(__GNUC__) && defined(__powerpc__) && defined(__linux__)
#define MXF_COMPILER_GCC_PPC_LINUX
#define MXF_OS_UNIX
#elif defined(mips) && defined(sgi)
#define MXF_COMPILER_SGICC_MIPS_SGI
#define MXF_OS_UNIX
#elif defined(__GNUC__) && defined(__i386__) && defined(__FreeBSD__)
#define MXF_COMPILER_GCC_INTEL_FREEBSD
#define MXF_OS_UNIX
#elif defined(__GNUC__) && defined(__i386__) && defined(__CYGWIN__)
#define MXF_COMPILER_GCC_INTEL_CYGWIN
#define MXF_OS_UNIX
#elif defined(__GNUC__) && defined(__sparc__) && defined(__sun__)
#define MXF_COMPILER_GCC_SPARC_SUNOS
#define MXF_OS_UNIX
#else
#error "Unknown compiler"
#endif

#if defined(MXF_OS_MACOS)
#define MXF_USE_CONSOLE
#endif

#if defined(MXF_USE_CONSOLE)
#include <console.h>
#endif

#if defined(MXF_COMPILER_MSC_INTEL_WINDOWS)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned int           mxfUInt32;
typedef unsigned __int64       mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "u"
#define MXFPRIu64 "I64u"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "x"
#define MXFPRIx64 "I64x"
#elif defined(MXF_COMPILER_MSC_X64_WINDOWS)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned int           mxfUInt32;
typedef unsigned __int64       mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "u"
#define MXFPRIu64 "I64u"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "x"
#define MXFPRIx64 "I64x"
#elif defined(MXF_COMPILER_GCC_INTEL_WINDOWS)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_INTEL_LINUX)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_X86_64_LINUX) || defined(MXF_COMPILER_GCC_X86_64_MACOSX)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned int	      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "u"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "x"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_MWERKS_PPC_MACOS)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_PPC_MACOSX)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_INTEL_X86_64_MACOSX)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned int           mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "u"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "x"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_ARM64_MACOSX) || defined(MXF_COMPILER_GCC_ARM64_LINUX)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned int           mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "u"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "x"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_INTEL_MACOSX)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_PPC_LINUX)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_SGICC_MIPS_SGI)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_INTEL_FREEBSD)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_INTEL_CYGWIN)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#elif defined(MXF_COMPILER_GCC_SPARC_SUNOS)
typedef unsigned char          mxfUInt08;
typedef unsigned short int     mxfUInt16;
typedef unsigned long int      mxfUInt32;
typedef unsigned long long int mxfUInt64;

#define MXFPRIu08 "u"
#define MXFPRIu16 "hu"
#define MXFPRIu32 "lu"
#define MXFPRIu64 "llu"
#define MXFPRIx08 "x"
#define MXFPRIx16 "hx"
#define MXFPRIx32 "lx"
#define MXFPRIx64 "llx"
#endif

#if defined(MXF_COMPILER_MSC_INTEL_WINDOWS)
  // - 'identifier' : identifier was truncated to 'number' characters in
  //   the debug information
#pragma warning(disable:4786) // Gak !
#endif

#include <list>
#include <map>
#include <set>

namespace { // unnamed namespace

typedef mxfUInt64 mxfLength;
typedef mxfUInt08 mxfByte;
struct mxfKey {
  mxfByte octet0;
  mxfByte octet1;
  mxfByte octet2;
  mxfByte octet3;
  mxfByte octet4;
  mxfByte octet5;
  mxfByte octet6;
  mxfByte octet7;
  mxfByte octet8;
  mxfByte octet9;
  mxfByte octet10;
  mxfByte octet11;
  mxfByte octet12;
  mxfByte octet13;
  mxfByte octet14;
  mxfByte octet15;
};
typedef mxfUInt16 mxfLocalKey;

struct aafUID {
  mxfUInt32 Data1;
  mxfUInt16 Data2;
  mxfUInt16 Data3;
  mxfUInt08 Data4[8];
};

aafUID nullAafUID =
{0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

struct mxfRational {
  mxfUInt32 numerator;
  mxfUInt32 denominator;
};

typedef enum ModeTag {
  unspecifiedMode,
  klvMode,
  localSetMode,
  mxfMode,
  aafMode,
  klvValidateMode,
  setValidateMode,
  mxfValidateMode,
  aafValidateMode} Mode;
Mode mode = unspecifiedMode;

mxfKey nullMxfKey = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#define NULLMXFKEY {0}
#define NULLAAFUID {0}
#else
#define NULLMXFKEY nullMxfKey
#define NULLAAFUID nullAafUID
#endif

bool reorder(void);
mxfUInt08 hostByteOrder(void);

// Primitives
#if defined(MXF_OS_WINDOWS)
#include <windows.h>
typedef HANDLE mxfFile;
#else
typedef FILE* mxfFile;
#endif

int mxfExitStatus(int status);

mxfFile openExistingRead(char* fileName);
void closeFile(mxfFile infile);
void setPosition(mxfFile infile, const mxfUInt64 position);
mxfUInt64 position(mxfFile infile);
size_t read(mxfFile infile, void* buffer, size_t count);
mxfUInt64 size(mxfFile infile);

void skipBytes(const mxfUInt64 byteCount, mxfFile infile);

void readMxfUInt08(mxfByte& b, mxfFile infile);
void readMxfUInt16(mxfUInt16& i, mxfFile infile);
void readMxfUInt32(mxfUInt32& i, mxfFile infile);
void readMxfUInt64(mxfUInt64& i, mxfFile infile);
void readMxfRational(mxfRational& r, mxfFile infile);
void readMxfLabel(mxfKey& k, mxfFile infile);
void readMxfKey(mxfKey& k, mxfFile infile);
bool readOuterMxfKey(mxfKey& k, mxfFile infile);
int readBERLength(mxfUInt64& i, mxfUInt08& ll, mxfFile infile);
int readMxfLength(mxfLength& l, mxfUInt08& ll, mxfFile infile);
void readMxfLocalKey(mxfLocalKey& k, mxfFile infile);

void reorder(mxfUInt16& i);
void reorder(mxfUInt32& i);
void reorder(mxfUInt64& i);
void reorder(aafUID& u);

void printDecField(FILE* f, mxfUInt08& i);
void printDecField(FILE* f, mxfUInt16& i);
void printDecField(FILE* f, mxfUInt32& i);
void printDecField(FILE* f, mxfUInt64& i);

void printSignedDecField(FILE* f, mxfUInt08& i);

void printDecFieldPad(FILE* f, mxfUInt08& i);
void printDecFieldPad(FILE* f, mxfUInt16& i);
void printDecFieldPad(FILE* f, mxfUInt32& i);
void printDecFieldPad(FILE* f, mxfUInt64& i);

void printDec(FILE* f, mxfUInt08& i);
void printDec(FILE* f, mxfUInt16& i);
void printDec(FILE* f, mxfUInt32& i);
void printDec(FILE* f, mxfUInt64& i);

void printHexField(FILE* f, mxfUInt08& i);
void printHexField(FILE* f, mxfUInt16& i);
void printHexField(FILE* f, mxfUInt32& i);
void printHexField(FILE* f, mxfUInt64& i);

void printHexFieldPad(FILE* f, mxfUInt08& i);
void printHexFieldPad(FILE* f, mxfUInt16& i);
void printHexFieldPad(FILE* f, mxfUInt32& i);
void printHexFieldPad(FILE* f, mxfUInt64& i);

void printHex(FILE* f, mxfUInt08& i);
void printHex(FILE* f, mxfUInt16& i);
void printHex(FILE* f, mxfUInt32& i);
void printHex(FILE* f, mxfUInt64& i);

void printMxfKey(const mxfKey& k, FILE* f);
void printMxfLength(mxfLength& l, FILE* f);
void printMxfLocalKey(mxfLocalKey& k, FILE* f);

void printMxfUInt08(FILE* f, const char* label, mxfUInt08& i);
void printMxfUInt16(FILE* f, const char* label, mxfUInt16& i);
void printMxfUInt32(FILE* f, const char* label, mxfUInt32& i);
void printMxfUInt64(FILE* f, const char* label, mxfUInt64& i);

void printMxfRational(FILE* f, const char* label, mxfRational& r);

void printMxfKey(FILE* f, const char* label, mxfKey& k);

void dumpMxfUInt08(const char* label, mxfFile infile);
void dumpMxfUInt16(const char* label, mxfFile infile);
void dumpMxfUInt32(const char* label, mxfFile infile);
void dumpMxfUInt64(const char* label, mxfFile infile);
void dumpMxfRational(const char* label, mxfFile infile);
void dumpMxfBoolean(const char* label,
                    const char* trueLabel,
                    const char* falseLabel,
                    mxfFile infile);
void dumpMxfString(const char* label, mxfFile infile);
void dumpOptionalMxfString(const char* label, mxfFile infile);
void dumpMxfLabel(const char* label, mxfFile infile);
void dumpMxfOperationalPattern(const char* label, mxfFile infile);

void dumpMxfUInt08Array(const char* label, size_t count, mxfFile infile);

void printOperationalPattern(mxfKey& k, FILE* outfile);

void klvDumpFile(mxfFile infile);
void setDumpFile(mxfFile infile);
void mxfDumpFile(mxfFile infile);
void aafDumpFile(mxfFile infile);

bool lookupMXFKey(mxfKey& k, size_t& index);
bool lookupAAFKey(mxfKey& k, size_t& index);
bool findAAFKey(mxfKey& k, size_t& index, char** flag);

bool lookupMXFLocalKey(mxfLocalKey& k, size_t& index);
bool lookupMXFPropertyKey(const mxfKey& k, size_t& index);
void updateMXFLocalKey(const mxfKey& key, const mxfLocalKey localKey);
void checkLocalKeyTable(void);
bool lookupAAFLocalKey(mxfLocalKey& k, size_t& index);
bool lookupAAFPropertyKey(const mxfKey& k, size_t& index);
void updateAAFLocalKey(const mxfKey& key, const mxfLocalKey localKey);
void checkAAFLocalKeyTable(void);

const char* aafKeyName(const mxfKey& k);
const char* mxfKeyName(const mxfKey& k);

const char* mxfLocalKeyName(mxfLocalKey& k);
const char* aafLocalKeyName(mxfLocalKey& k);

bool isEssenceElement(mxfKey& k);

bool isIndexSegment(mxfKey& k);

void checkKey(mxfKey& k);
bool isNullKey(const mxfKey& k);
bool isDark(mxfKey& k, Mode mode);
bool isFill(mxfKey& k);
bool isPartition(mxfKey& key);
bool isHeader(mxfKey& key);
bool isFooter(mxfKey& key);
bool isClosed(const mxfKey& key);
bool isComplete(const mxfKey& key);

bool isRandomIndex(const mxfKey& key);

bool isPrivateLabel(mxfKey& key);
void printPrivateLabelName(mxfKey& k, FILE* outfile);
void printPrivateLabel(mxfKey& k, FILE* outfile);

const char* keyName(const mxfKey& key);

void checkFill(const mxfKey& key,
               mxfUInt64 keyPosition,
               const mxfKey& previousKey);
void printFill(mxfKey& k, mxfLength& len, mxfFile infile);

void skipV(mxfLength length, mxfFile infile);

void checkPartitionLength(mxfUInt64& length);

// Size of the fixed portion of a partition
mxfUInt64 partitionFixedSize =
  sizeof(mxfUInt16) + // Major Version
  sizeof(mxfUInt16) + // Minor Version
  sizeof(mxfUInt32) + // KAGSize
  sizeof(mxfUInt64) + // ThisPartition
  sizeof(mxfUInt64) + // PreviousPartition
  sizeof(mxfUInt64) + // FooterPartition
  sizeof(mxfUInt64) + // HeaderByteCount
  sizeof(mxfUInt64) + // IndexByteCount
  sizeof(mxfUInt32) + // IndexSID
  sizeof(mxfUInt64) + // BodyOffset
  sizeof(mxfUInt32) + // BodySID
  sizeof(mxfKey)    + // Operational pattern
  sizeof(mxfUInt32) + // Essence container label count
  sizeof(mxfUInt32);  // Essence container label size

void print(const char* format, ...);

void vprint(const char* format, va_list ap);

void error(const char* format, ...);

void verror(const char* format, va_list ap);

void fatalError(const char* format, ...);

void warning(const char* format, ...);

void vwarning(const char* format, va_list ap);

void message(const char* format, ...);

void vmessage(const char* format, va_list ap);

const char* baseName(const char* fullName);

void setProgramName(const char* programName);

char* programName(void);

void mxfError(const char* format, ...);

void vmxfError(const char* format, va_list ap);

void mxfWarning(const char* format, ...);

void vmxfWarning(const char* format, va_list ap);

void mxfWarning(const mxfKey& key, mxfUInt64 keyPosition, const char* format, ...);

void vmxfWarning(const mxfKey& key,
                 mxfUInt64 keyPosition,
                 const char* format,
                 va_list ap);

void mxfError(const mxfKey& key, mxfUInt64 keyPosition, const char* format, ...);

void vmxfError(const mxfKey& key,
               mxfUInt64 keyPosition,
               const char* format,
               va_list ap);

void printLocation(const mxfKey& key, mxfUInt64 keyPosition);

void print(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vprint(format, ap);
  va_end(ap);
}

void vprint(const char* format, va_list ap)
{
  vfprintf(stderr, format, ap);
}

void error(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  verror(format, ap);
  va_end(ap);
}

void verror(const char* format, va_list ap)
{
  fprintf(stderr, "%s : Error : ", programName());
  vfprintf(stderr, format, ap);
}

void fatalError(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "%s : Error : ", programName());
  vfprintf(stderr, format, ap);
  exit(EXIT_FAILURE);
  va_end(ap);
}

void warning(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vwarning(format, ap);
  va_end(ap);
}

void vwarning(const char* format, va_list ap)
{
  fprintf(stderr, "%s : Warning : ", programName());
  vfprintf(stderr, format, ap);
}

void message(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vmessage(format, ap);
  va_end(ap);
}

void vmessage(const char* format, va_list ap)
{
  fprintf(stderr, "%s : ", programName());
  vfprintf(stderr, format, ap);
}

char progName[FILENAME_MAX];

void setProgramName(const char* programName)
{
  const char* base = baseName(programName);
  const char* suffix = strrchr(base, '.');
  size_t length;
  if (suffix != 0) {
    length = suffix - base;
  } else {
    length = strlen(base);
  }
  if (length >= FILENAME_MAX) {
    length = FILENAME_MAX - 1;
  }
  strncpy(progName, base, length);
  progName[length] = '\0';
}

char* programName(void)
{
  return progName;
}

mxfUInt32 errors = 0;
const mxfUInt32 maxErrors = 100;

void mxfError(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vmxfError(format, ap);
  va_end(ap);
}

void vmxfError(const char* format, va_list ap)
{
  verror(format, ap);
  errors = errors + 1;
  if (errors >= maxErrors) {
    error("Too many errors (%" MXFPRIu32 ") encountered - giving up.\n", errors);
    exit(mxfExitStatus(EXIT_FAILURE));
  }
}

mxfUInt32 warnings = 0;

void mxfWarning(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vmxfWarning(format, ap);
  va_end(ap);
}

void vmxfWarning(const char* format, va_list ap)
{
  vwarning(format, ap);
  warnings = warnings + 1;
}

void mxfWarning(const mxfKey& key, mxfUInt64 keyPosition, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vmxfWarning(key, keyPosition, format, ap);
  va_end(ap);
}

void vmxfWarning(const mxfKey& key,
                 mxfUInt64 keyPosition,
                 const char* format,
                 va_list ap)
{
  vmxfWarning(format, ap);
  printLocation(key, keyPosition);
}

void mxfError(const mxfKey& key, mxfUInt64 keyPosition, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  vmxfError(key, keyPosition, format, ap);
  va_end(ap);
}

void vmxfError(const mxfKey& key,
               mxfUInt64 keyPosition,
               const char* format, va_list ap)
{
  vmxfError(format, ap);
  printLocation(key, keyPosition);
}

void printLocation(const mxfKey& key, mxfUInt64 keyPosition)
{
  const char *name = keyName(key);
  print(" (following %s key at offset 0x%" MXFPRIx64 ").\n",
        name,
        keyPosition);
}

void missingProperty(const mxfKey& key,
                     mxfUInt64 keyPosition,
                     const char* label);

void missingProperty(const mxfKey& key,
                     mxfUInt64 keyPosition,
                     const char* label)
{
  mxfError(key,
           keyPosition,
           "Required property \"%s\" missing",
           label);
}

bool verbose = false;
bool debug = false;

mxfUInt64 keyPosition;   // position/address of current key
mxfKey currentKey = nullMxfKey;
mxfKey previousKey = nullMxfKey;
mxfUInt64 primerPosition = 0;

mxfKey currentPartitionKey = nullMxfKey;
mxfKey previousPartitionKey = nullMxfKey;

bool inIndex = false;
mxfUInt32 indexSID = 0;
mxfKey indexLabel = nullMxfKey;
mxfUInt64 indexPosition = 0;

bool inEssence = false;
mxfUInt32 essenceSID = 0;
mxfKey essenceLabel = nullMxfKey;
mxfUInt64 essencePosition = 0;

mxfUInt64 fillStart = 0;
mxfUInt64 fillEnd = 0;

void markMetadataStart(mxfUInt64 primerKeyPosition);
void markMetadataEnd(mxfUInt64 endKeyPosition);
void markIndexStart(mxfUInt32 sid, mxfUInt64 indexKeyPosition);
void markIndexEnd(mxfUInt64 endKeyPosition);
void markEssenceSegmentStart(mxfUInt32 sid, mxfUInt64 essenceKeyPosition);
void markEssenceSegmentEnd(mxfUInt64 endKeyPosition);
void markFill(mxfUInt64 fillKeyPosition,
              mxfUInt64 fillEndPosition);

void newSegment(bool isEssence,
                mxfUInt32 sid,
                mxfKey& label,
                mxfUInt64 start,
                mxfUInt64 size,
                mxfUInt64 free);

void newEssenceSegment(mxfUInt32 sid,
                       mxfKey& label,
                       mxfUInt64 start,
                       mxfUInt64 size,
                       mxfUInt64 free);

void newIndexSegment(mxfUInt32 sid,
                     mxfKey& label,
                     mxfUInt64 start,
                     mxfUInt64 size,
                     mxfUInt64 free);

// Frame wrapped essence
bool frames = false;     // if true, treat essence as frame wrapped.
bool iFlag = false;      // if true, only print maxFrames frames
mxfUInt32 maxFrames = 0;

// Frame index tables
bool cFlag = true;      // if true, only print maxIndex entries
mxfUInt32 maxIndex = 0;

// Essence elements
bool lFlag = false;      // if true, limit/no-limit specified on command line
bool limitBytes = false; // if true, only print limit bytes
mxfUInt32 limit = 0;
bool dumpANC = false; // if true, dump the contents of ANC Frame Elements

mxfUInt32 runInLimit = 64 * 1024;


// Help
bool hFlag = false;

#if defined(MXF_OS_WINDOWS)

mxfFile openExistingRead(char* fileName)
{
  HANDLE result = CreateFileA(fileName,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             0,
                             OPEN_EXISTING,
                             0,
                             0);
  if (result == INVALID_HANDLE_VALUE) {
    result = 0;
  }
  return result;
}

void closeFile(mxfFile infile)
{
  BOOL result = CloseHandle(infile);
  if (!result) {
    fatalError("CloseHandle() failed.\n");
  }
}

void setPosition(mxfFile infile, const mxfUInt64 position)
{
  LARGE_INTEGER li;
  li.QuadPart = position;
  li.LowPart = SetFilePointer(infile, li.LowPart, &li.HighPart, FILE_BEGIN);
  if ((li.LowPart == INVALID_SET_FILE_POINTER) && GetLastError() != NO_ERROR) {
    fatalError("SetFilePointer() failed.\n");
  }
}

size_t read(mxfFile infile, void* buffer, size_t count)
{
  DWORD byteCount = static_cast<DWORD>(count);
  DWORD bytesRead;
  BOOL result = ReadFile(infile, buffer, byteCount, &bytesRead, 0);
  if (!result) {
    fatalError("ReadFile() failed.\n");
  }
  return bytesRead;
}

mxfUInt64 position(mxfFile infile)
{
  mxfUInt64 result;
  LARGE_INTEGER li;
  li.QuadPart = 0;
  li.LowPart = SetFilePointer(infile, li.LowPart, &li.HighPart, FILE_CURRENT);
  if ((li.LowPart == INVALID_SET_FILE_POINTER) && GetLastError() != NO_ERROR) {
    fatalError("SetFilePointer() failed.\n");
  }
  result = li.QuadPart;
  return result;
}

mxfUInt64 size(mxfFile infile)
{
  mxfUInt64 result;
  ULARGE_INTEGER li;
  li.LowPart = GetFileSize(infile, &li.HighPart);
  if ((li.LowPart == INVALID_FILE_SIZE) && GetLastError() != NO_ERROR) {
    fatalError("GetFileSize() failed.\n");
  }
  result = li.QuadPart;
  return result;
}

#else

#if defined(__GLIBC__) && defined(__GNUC_MINOR__)
#if (__GLIBC__ >= 2) && (__GLIBC_MINOR__ >=3)
// fseeko present in 2.3 but not in 2.2
#define MXF_SEEKO
#endif
#endif

mxfFile openExistingRead(char* fileName)
{
  return  fopen(fileName, "rb");
}

void closeFile(mxfFile infile)
{
  fclose(infile);
}

void seek64(mxfFile infile, const mxfUInt64 position, int whence);

void seek64(mxfFile infile, const mxfUInt64 position, int whence)
{
#if defined(MXF_COMPILER_GCC_INTEL_LINUX) && defined(MXF_SEEKO)
  int status = fseeko(infile, position, whence);
#elif defined(MXF_COMPILER_MWERKS_PPC_MACOS)
  int status = _fseek(infile, position, whence);
#elif defined(MXF_COMPILER_MWERKS_PPC_MACOSX)
  int status = _fseek(infile, position, whence);
#elif defined(MXF_COMPILER_GCC_PPC_MACOSX)
  int status = fseeko(infile, position, whence);
#elif defined(MXF_COMPILER_GCC_INTEL_MACOSX)
  int status = fseeko(infile, position, whence);
#elif defined(MXF_COMPILER_GCC_INTEL_X86_64_MACOSX)
  int status = fseeko(infile, position, whence);
#elif defined(MXF_COMPILER_SGICC_MIPS_SGI)
  int status = fseeko64(infile, position, whence);
#else
  long offset = static_cast<long>(position);
  if (position != static_cast<mxfUInt64>(offset)) {
    fatalError("Offset too large.\n");
  }
  int status = fseek(infile, offset, whence);
#endif
  if (status != 0) {
    fatalError("Failed to seek.\n");
  }
}

void setPosition(mxfFile infile, const mxfUInt64 position)
{
  seek64(infile, position, SEEK_SET);
}

size_t read(mxfFile infile, void* buffer, size_t count)
{
  return fread(buffer, 1, count, infile);
}

mxfUInt64 tell64(mxfFile infile);

mxfUInt64 tell64(mxfFile infile)
{
#if defined(MXF_COMPILER_GCC_INTEL_LINUX) && defined(MXF_SEEKO)
  mxfUInt64 result = ftello(infile);
#elif defined(MXF_COMPILER_MWERKS_PPC_MACOS)
  mxfUInt64 result = _ftell(infile);
#elif defined(MXF_COMPILER_MWERKS_PPC_MACOSX)
  mxfUInt64 result = _ftell(infile);
#elif defined(MXF_COMPILER_GCC_PPC_MACOSX)
  mxfUInt64 result = ftello(infile);
#elif defined(MXF_COMPILER_GCC_INTEL_MACOSX)
  mxfUInt64 result = ftello(infile);
#elif defined(MXF_COMPILER_GCC_INTEL_X86_64_MACOSX)
  mxfUInt64 result = ftello(infile);
#elif defined(MXF_COMPILER_SGICC_MIPS_SGI)
  mxfUInt64 result = ftello64(infile);
#else
  mxfUInt64 result = ftell(infile);
#endif
  if (result == (mxfUInt64)-1) {
    fatalError("Failed to tell.\n");
  }
  return result;
}

mxfUInt64 position(mxfFile infile)
{
  return tell64(infile);
}

mxfUInt64 size(mxfFile infile)
{
  mxfUInt64 savedPosition = position(infile);
  seek64(infile, 0, SEEK_END);
  mxfUInt64 result = position(infile);
  setPosition(infile, savedPosition);
  return result;
}
#endif

bool findPattern(mxfUInt64 currentPosition,
                 mxfUInt64& patternPosition,
                 mxfByte* pattern,
                 size_t patternSize,
                 mxfUInt32 limit,
                 mxfFile infile);

bool findPattern(mxfUInt64 currentPosition,
                 mxfUInt64& patternPosition,
                 mxfByte* pattern,
                 size_t patternSize,
                 mxfUInt32 limit,
                 mxfFile infile)
{
  bool found = false;
  mxfUInt64 pos = currentPosition;
  size_t c = 0;
  size_t i = 0;
  do {
    mxfByte b;
    c = read(infile, &b, 1);
    if (c == 1) {
      if (b != pattern[i]) {
        pos = pos + i + 1;
        i = 0;
      } else {
        if (i < patternSize - 1) {
          i = i + 1;
        } else {
          patternPosition = pos;
          found = true;
        }
      }
    }
  } while ((!found) && (pos < limit) && (c == 1));
  return found;
}

mxfByte headerPrefix[] = { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01,
                           0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x02};

bool findHeader(mxfFile infile, mxfUInt64& headerPosition);

bool findHeader(mxfFile infile, mxfUInt64& headerPosition)
{
  mxfUInt64 startPosition = 0;
  setPosition(infile, startPosition);
  return findPattern(startPosition,
                     headerPosition,
                     headerPrefix,
                     sizeof(headerPrefix),
                     runInLimit,
                     infile);
}

bool isMxfFile(mxfFile infile, mxfUInt64& headerPosition);

bool isMxfFile(mxfFile infile, mxfUInt64& headerPosition)
{
  bool result = false;

  if (findHeader(infile, headerPosition)) {
    setPosition(infile, headerPosition);
    mxfKey k;
    readMxfKey(k, infile);
    if (isHeader(k)) {
      result = true;
    }
  }

  return result;
}

void skipBytes(const mxfUInt64 byteCount, mxfFile infile)
{
  setPosition(infile, position(infile) + byteCount);
}

void readMxfUInt08(mxfByte& b, mxfFile infile)
{
  size_t c = read(infile, &b, sizeof(mxfByte));
  if (c != sizeof(mxfByte)) {
    fatalError("Failed to read byte.\n");
  }
}

void readMxfUInt16(mxfUInt16& i, mxfFile infile)
{
  size_t c = read(infile, &i, sizeof(mxfUInt16));
  if (c != sizeof(mxfUInt16)) {
    fatalError("Failed to read mxfUInt16.\n");
  }
  if (reorder()) {
    reorder(i);
  }
}

void readMxfUInt32(mxfUInt32& i, mxfFile infile)
{
  size_t c = read(infile, &i, sizeof(mxfUInt32));
  if (c != sizeof(mxfUInt32)) {
    fatalError("Failed to read mxfUInt32.\n");
  }
  if (reorder()) {
    reorder(i);
  }
}

void readMxfUInt64(mxfUInt64& i, mxfFile infile)
{
  size_t c = read(infile, &i, sizeof(mxfUInt64));
  if (c != sizeof(mxfUInt64)) {
    fatalError("Failed to read mxfUInt64.\n");
  }
  if (reorder()) {
    reorder(i);
  }
}

void readMxfRational(mxfRational& r, mxfFile infile)
{
  readMxfUInt32(r.numerator, infile);
  readMxfUInt32(r.denominator, infile);
}

void readMxfLabel(mxfKey& k, mxfFile infile)
{
  size_t c = read(infile, &k, sizeof(mxfKey));
  if (c != sizeof(mxfKey)) {
    fatalError("Failed to read label.\n");
  }
}

void readMxfKey(mxfKey& k, mxfFile infile)
{
  keyPosition = position(infile);
  size_t c = read(infile, &k, sizeof(mxfKey));
  if (c != sizeof(mxfKey)) {
    fatalError("Failed to read key.\n");
  }
}

bool readOuterMxfKey(mxfKey& k, mxfFile infile)
{
  bool result = true;
  keyPosition = position(infile);
  size_t c = read(infile, &k, sizeof(mxfKey));
  if (c == sizeof(mxfKey)) {
    result = true;
  } else if (c == 0) {
    result = false;
  } else {
    fatalError("Failed to read key.\n");
  }
  memcpy(&previousKey, &currentKey, sizeof(mxfKey));
  memcpy(&currentKey, &k, sizeof(mxfKey));
  return result;
}

int readBERLength(mxfUInt64& i, mxfUInt08& ll, mxfFile infile)
{
  int bytesRead = 0;
  mxfUInt08 b;
  readMxfUInt08(b, infile);
  bytesRead = bytesRead + 1;
  if (b == 0x80) {
    // unknown length
    i = 0;
    ll = 0;
  } else if ((b & 0x80) != 0x80) {
    // short form
    i = b;
    ll = 1;
  } else {
    // long form
    int length = b & 0x7f;
    i = 0;
    for (int k = 0; k < length; k++) {
      readMxfUInt08(b, infile);
      bytesRead = bytesRead + 1;
      i = i << 8;
      i = i + b;
    }
    ll = length + 1;
  }
  return bytesRead;
}

int readMxfLength(mxfLength& l, mxfUInt08& ll, mxfFile infile)
{
  mxfUInt64 x = 0;
  mxfUInt08 xl = 0;
  int bytesRead = readBERLength(x, xl, infile);
  if (bytesRead > 9) {
    mxfError(currentKey, keyPosition, "Invalid BER encoded length");
  }
  if (x == 0) {
    mxfWarning(currentKey, keyPosition, "Length is zero");
  }
  l = x;
  ll = xl;
  return bytesRead;
}

void readMxfLocalKey(mxfLocalKey& k, mxfFile infile)
{
  readMxfUInt16(k, infile);
}

void reorder(mxfUInt16& i)
{
  mxfUInt08* p = (mxfUInt08*)&i;
  mxfUInt08 temp;

  temp = p[0];
  p[0] = p[1];
  p[1] = temp;
}

void reorder(mxfUInt32& i)
{
  mxfUInt08* p = (mxfUInt08*)&i;
  mxfUInt08 temp;

  temp = p[0];
  p[0] = p[3];
  p[3] = temp;

  temp = p[1];
  p[1] = p[2];
  p[2] = temp;
}

void reorder(mxfUInt64& i)
{
  mxfUInt08* p = (mxfUInt08*)&i;
  mxfUInt08 temp;

  temp = p[0];
  p[0] = p[7];
  p[7] = temp;

  temp = p[1];
  p[1] = p[6];
  p[6] = temp;

  temp = p[2];
  p[2] = p[5];
  p[5] = temp;

  temp = p[3];
  p[3] = p[4];
  p[4] = temp;
}

void reorder(aafUID& u)
{
  reorder(u.Data1);
  reorder(u.Data2);
  reorder(u.Data3);
  // no need to reorder Data4
}

void printDecField(FILE* f, mxfUInt08& i)
{
  fprintf(f, "%3" MXFPRIu08, i);
}

void printDecField(FILE* f, mxfUInt16& i)
{
  fprintf(f, "%5" MXFPRIu16, i);
}

void printDecField(FILE* f, mxfUInt32& i)
{
  fprintf(f, "%10" MXFPRIu32, i);
}

void printDecField(FILE* f, mxfUInt64& i)
{
  fprintf(f, "%10" MXFPRIu64, i); // tjb - should be 20
}

// print unsigned byte as signed
void printSignedDecField(FILE* f, mxfUInt08& i)
{
  if ((i & 0x80) == 0) {
    fprintf(f, " ");
    printDecField(f, i);
  } else {
    mxfUInt08 x =  (0xff - i) + 1;
    fprintf(f, "-");
    printDecField(f, x);
  }
}

void printDecFieldPad(FILE* f, mxfUInt08& i)
{
  fprintf(f, "%03" MXFPRIu08, i);
}

void printDecFieldPad(FILE* f, mxfUInt16& i)
{
  fprintf(f, "%05" MXFPRIu16, i);
}

void printDecFieldPad(FILE* f, mxfUInt32& i)
{
  fprintf(f, "%010" MXFPRIu32, i);
}

void printDecFieldPad(FILE* f, mxfUInt64& i)
{
  fprintf(f, "%010" MXFPRIu64, i); // tjb - should be 20
}

void printDec(FILE* f, mxfUInt08& i)
{
  fprintf(f, "%" MXFPRIu08, i);
}

void printDec(FILE* f, mxfUInt16& i)
{
  fprintf(f, "%" MXFPRIu16, i);
}

void printDec(FILE* f, mxfUInt32& i)
{
  fprintf(f, "%" MXFPRIu32, i);
}

void printDec(FILE* f, mxfUInt64& i)
{
  fprintf(f, "%" MXFPRIu64, i);
}

void printHexField(FILE* f, mxfUInt08& i)
{
  fprintf(f, "%2" MXFPRIx08, i);
}

void printHexField(FILE* f, mxfUInt16& i)
{
  fprintf(f, "%4" MXFPRIx16, i);
}

void printHexField(FILE* f, mxfUInt32& i)
{
  fprintf(f, "%8" MXFPRIx32, i);
}

void printHexField(FILE* f, mxfUInt64& i)
{
  fprintf(f, "%16" MXFPRIx64, i);
}

void printHexFieldPad(FILE* f, mxfUInt08& i)
{
  fprintf(f, "%02" MXFPRIx08, i);
}

void printHexFieldPad(FILE* f, mxfUInt16& i)
{
  fprintf(f, "%04" MXFPRIx16, i);
}

void printHexFieldPad(FILE* f, mxfUInt32& i)
{
  fprintf(f, "%08" MXFPRIx32, i);
}

void printHexFieldPad(FILE* f, mxfUInt64& i)
{
  fprintf(f, "%016" MXFPRIx64, i);
}

void printHex(FILE* f, mxfUInt08& i)
{
  fprintf(f, "%" MXFPRIx08, i);
}

void printHex(FILE* f, mxfUInt16& i)
{
  fprintf(f, "%" MXFPRIx16, i);
}

void printHex(FILE* f, mxfUInt32& i)
{
  fprintf(f, "%" MXFPRIx32, i);
}

void printHex(FILE* f, mxfUInt64& i)
{
  fprintf(f, "%" MXFPRIx64, i);
}

void printMxfKey(const mxfKey& k, FILE* f)
{
  fprintf(f,
          "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x."
          "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
          k.octet0,
          k.octet1,
          k.octet2,
          k.octet3,
          k.octet4,
          k.octet5,
          k.octet6,
          k.octet7,
          k.octet8,
          k.octet9,
          k.octet10,
          k.octet11,
          k.octet12,
          k.octet13,
          k.octet14,
          k.octet15);
}

void printMxfLength(mxfLength& l, FILE* f)
{
  printDecField(f, l);
  fprintf(f, " ");
  fprintf(f, "(");
  printHex(f, l);
  fprintf(f, ")");
}

void printMxfLocalKey(mxfLocalKey& k, FILE* f)
{
  unsigned int msb = (k & 0xff00) >> 8;
  unsigned int lsb = (k & 0x00ff);
  fprintf(f, "%02x.%02x", msb, lsb);
}

void printMxfUInt08(FILE* f, const char* label, mxfUInt08& i)
{
  fprintf(f, "%20s = ", label);
  printHexFieldPad(f, i);
  fprintf(f, "\n");
}

void printMxfUInt16(FILE* f, const char* label, mxfUInt16& i)
{
  fprintf(f, "%20s = ", label);
  printHexFieldPad(f, i);
  fprintf(f, "\n");
}

void printMxfUInt32(FILE* f, const char* label, mxfUInt32& i)
{
  fprintf(f, "%20s = ", label);
  printHexFieldPad(f, i);
  fprintf(f, "\n");
}

void printMxfUInt64(FILE* f, const char* label, mxfUInt64& i)
{
  fprintf(f, "%20s = ", label);
  printHexFieldPad(f, i);
  fprintf(f, "\n");
}

void printMxfRational(FILE* f, const char* label, mxfRational& r)
{
  fprintf(f, "%20s = ", label);
  fprintf(f, "( ");
  printDecField(f, r.numerator);
  fprintf(f, " / ");
  printDecField(f, r.denominator);
  fprintf(f, " )");
  fprintf(f, "\n");
}

void printMxfKey(FILE* f, const char* label, mxfKey& k)
{
  fprintf(f, "%20s = ", label);
  printMxfKey(k, f);
  fprintf(f, "\n");
}

void dumpMxfUInt08(const char* label, mxfFile infile)
{
  mxfUInt08 i;
  readMxfUInt08(i, infile);
  printMxfUInt08(stdout, label, i);
}

void dumpMxfUInt16(const char* label, mxfFile infile)
{
  mxfUInt16 i;
  readMxfUInt16(i, infile);
  printMxfUInt16(stdout, label, i);
}

void dumpMxfUInt32(const char* label, mxfFile infile)
{
  mxfUInt32 i;
  readMxfUInt32(i, infile);
  printMxfUInt32(stdout, label, i);
}

void dumpMxfUInt64(const char* label, mxfFile infile)
{
  mxfUInt64 i;
  readMxfUInt64(i, infile);
  printMxfUInt64(stdout, label, i);
}

void dumpMxfRational(const char* label, mxfFile infile)
{
  mxfRational r;
  readMxfRational(r, infile);
  fprintf(stdout, "%20s = ", label);
  fprintf(stdout, "( ");
  printDecField(stdout, r.numerator);
  fprintf(stdout, " / ");
  printDecField(stdout, r.denominator);
  fprintf(stdout, " )");
  fprintf(stdout, "\n");
}

void dumpMxfBoolean(const char* label,
                    const char* trueLabel,
                    const char* falseLabel,
                    mxfFile infile)
{
  mxfUInt08 b;
  readMxfUInt08(b, infile);
  if (b == 0x00) {
    fprintf(stdout, "%20s = %s\n", label, falseLabel);
  } else {
    fprintf(stdout, "%20s = %s\n", label, trueLabel);
    if (b != 0xff) {
      mxfWarning(currentKey,
                 keyPosition,
                 "Invalid value for boolean, "
                 "0x%" MXFPRIx08 " interpreted as true",
                 b);
    }
  }
}

void printString(FILE* f, mxfUInt16* str);

void printString(FILE* f, mxfUInt16* str)
{
  fprintf(f, "\"");
  mxfUInt16 character;
  while ((character = *str++) != 0) {
    fprintf(f, "%c", (char)(character)); // tjb - hack
  }
  fprintf(f, "\"\n");
}

void readString(mxfUInt16* buffer, mxfUInt32 bufferSize, mxfFile infile);

void readString(mxfUInt16* buffer, mxfUInt32 bufferSize, mxfFile infile)
{
  for (mxfUInt32 i = 0; i < bufferSize; i++) {
    readMxfUInt16(buffer[i], infile);
  }
}

void dumpMxfString(const char* label, mxfFile infile)
{
  mxfUInt16 characterCount; // including terminator
  readMxfUInt16(characterCount, infile);

  mxfUInt16* buffer = new mxfUInt16[characterCount];
  readString(buffer, characterCount, infile);

  fprintf(stdout, "%20s = ", label);
  printString(stdout, buffer);

  delete [] buffer;
}

void dumpOptionalMxfString(const char* label, mxfFile infile)
{
  mxfUInt16 characterCount; // including terminator
  readMxfUInt16(characterCount, infile);

  if (characterCount > 0) {
    mxfUInt16* buffer = new mxfUInt16[characterCount];
    readString(buffer, characterCount, infile);

    fprintf(stdout, "%20s = ", label);
    printString(stdout, buffer);

    delete [] buffer;
  } else {
    fprintf(stdout, "%20s   <not present>\n", label);
  }

}

void dumpExtensionDefinition(mxfFile infile);

void dumpExtensionDefinition(mxfFile infile)
{
  dumpMxfLabel("identification", infile);
  dumpMxfString("name", infile);
  dumpOptionalMxfString("description", infile);
}

void dumpExtensionClass(mxfFile infile);

void dumpExtensionClass(mxfFile infile)
{
  fprintf(stdout, "class = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("parent class", infile);
  dumpMxfBoolean("concrete/abstract", "concrete", "abstract", infile);
  fprintf(stdout, "]\n");
}

void dumpExtensionProperty(mxfFile infile);

void dumpExtensionProperty(mxfFile infile)
{
  fprintf(stdout, "property = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("type", infile);
  dumpMxfBoolean("required/optional", "required", "optional", infile);
  dumpMxfLabel("member of", infile);
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeInteger(mxfFile infile);

void dumpExtensionTypeInteger(mxfFile infile)
{
  fprintf(stdout, "integer type = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfUInt08("size", infile);
  dumpMxfBoolean("signed/unsigned", "signed", "unsigned", infile);
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeCharacter(mxfFile infile);

void dumpExtensionTypeCharacter(mxfFile infile)
{
  fprintf(stdout, "character type = [\n");
  dumpExtensionDefinition(infile);

  fprintf(stdout, "]\n");
}

void dumpExtensionTypeStrongObjectReference(mxfFile infile);

void dumpExtensionTypeStrongObjectReference(mxfFile infile)
{
  fprintf(stdout, "strong object reference type = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("referenced type", infile);
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeWeakObjectReference(mxfFile infile);

void dumpExtensionTypeWeakObjectReference(mxfFile infile)
{
  fprintf(stdout, "weak object reference type = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("referenced type", infile);

  // TargetPathElementCount
  mxfUInt32 count;
  readMxfUInt32(count, infile);
  printMxfUInt32(stdout, "element count", count);
  // TargetPath
  for (mxfUInt32 i = 0; i < count; i++) {
    dumpMxfLabel("property", infile);
  }
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeRename(mxfFile infile);

void dumpExtensionTypeRename(mxfFile infile)
{
  fprintf(stdout, "rename type = [\n");
  dumpExtensionDefinition(infile);

  // RenamedType
  dumpMxfLabel("renamed type", infile);
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeEnumerated(mxfFile infile);

void dumpExtensionTypeEnumerated(mxfFile infile)
{
  fprintf(stdout, "enumerated type = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("element type", infile);
  // ElementCount
  mxfUInt32 count;
  readMxfUInt32(count, infile);
  printMxfUInt32(stdout, "element count", count);
  for (mxfUInt32 i = 0; i < count; i++) {
    // ElementName
    dumpMxfString("name", infile);
    // ElementValue
    dumpMxfUInt64("value", infile);
  }
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeFixedArray(mxfFile infile);

void dumpExtensionTypeFixedArray(mxfFile infile)
{
  fprintf(stdout, "fixed array type = [\n");
  dumpExtensionDefinition(infile);

  // ElementType
  dumpMxfLabel("element type", infile);
  // ElementCount
  dumpMxfUInt32("element count", infile);

  fprintf(stdout, "]\n");
}

void dumpExtensionTypeVaryingArray(mxfFile infile);

void dumpExtensionTypeVaryingArray(mxfFile infile)
{
  fprintf(stdout, "varying array type = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("element type", infile);

  fprintf(stdout, "]\n");
}

void dumpExtensionTypeSet(mxfFile infile);

void dumpExtensionTypeSet(mxfFile infile)
{
  fprintf(stdout, "set type = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("element type", infile);

  fprintf(stdout, "]\n");
}

void dumpExtensionTypeRecord(mxfFile infile);

void dumpExtensionTypeRecord(mxfFile infile)
{
  fprintf(stdout, "record type = [\n");
  dumpExtensionDefinition(infile);
  // MemberCount
  mxfUInt32 count;
  readMxfUInt32(count, infile);
  printMxfUInt32(stdout, "member count", count);
  for (mxfUInt32 i = 0; i < count; i++) {
    // MemberType
    dumpMxfLabel("type", infile);
    // MemberName
    dumpMxfString("name", infile);
  }
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeStream(mxfFile infile);

void dumpExtensionTypeStream(mxfFile infile)
{
  fprintf(stdout, "stream type = [\n");
  dumpExtensionDefinition(infile);

  fprintf(stdout, "]\n");
}

void dumpExtensionTypeString(mxfFile infile);

void dumpExtensionTypeString(mxfFile infile)
{
  fprintf(stdout, "string type = [\n");
  dumpExtensionDefinition(infile);

  dumpMxfLabel("element type", infile);

  fprintf(stdout, "]\n");
}

void dumpExtensionTypeExtendibleEnumerated(mxfFile infile);

void dumpExtensionTypeExtendibleEnumerated(mxfFile infile)
{
  fprintf(stdout, "extendible enumerated type = [\n");
  dumpExtensionDefinition(infile);

  // ElementCount
  mxfUInt32 count;
  readMxfUInt32(count, infile);
  printMxfUInt32(stdout, "element count", count);
  for (mxfUInt32 i = 0; i < count; i++) {
    // ElementName
    dumpMxfString("name", infile);
    // ElementValue
    dumpMxfLabel("value", infile);
  }
  fprintf(stdout, "]\n");
}

void dumpExtensionTypeIndirect(mxfFile infile);

void dumpExtensionTypeIndirect(mxfFile infile)
{
  fprintf(stdout, "indirect type = [\n");
  dumpExtensionDefinition(infile);

  fprintf(stdout, "]\n");
}

void dumpExtensionTypeOpaque(mxfFile infile);

void dumpExtensionTypeOpaque(mxfFile infile)
{
  fprintf(stdout, "opaque type = [\n");
  dumpExtensionDefinition(infile);

  fprintf(stdout, "]\n");
}

void dumpMxfLabel(const char* label, mxfFile infile)
{
  mxfKey k;
  readMxfLabel(k, infile);
  fprintf(stdout, "%20s = ", label);
  printMxfKey(k, stdout);
  fprintf(stdout, "\n");
}

void dumpMxfUInt08Array(const char* label, size_t count, mxfFile infile)
{
  for (size_t i = 0; i < count; i++) {
    if ((i % 16) == 0) {
      if (i == 0) {
        fprintf(stdout, "%20s = ", label);
      } else {
        fprintf(stdout, "%20s   ", "");
      }
    }
    mxfUInt08 b;
    readMxfUInt08(b, infile);
    printHexFieldPad(stdout, b);
    if ((i % 16) == 15) {
      fprintf(stdout, "\n");
    } else {
      fprintf(stdout, " ");
    }
  }
  fprintf(stdout, "\n");
}

void dumpMxfOperationalPattern(const char* label, mxfFile infile)
{
  mxfKey k;
  readMxfLabel(k, infile);
  fprintf(stdout, "%20s = ", label);
  printMxfKey(k, stdout);
  fprintf(stdout, "\n");
  fprintf(stdout, "%20s = ", "");
  printOperationalPattern(k, stdout);
  fprintf(stdout, "\n");
}

mxfByte opPrefix1[] =
{0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01};

mxfByte opPrefix2[] =
{0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02, 0x0d, 0x01, 0x02, 0x01};

mxfByte opPrefix3[] =
{0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x03, 0x0d, 0x01, 0x02, 0x01};

bool isOperationalPattern(mxfKey& k);

bool isOperationalPattern(mxfKey& k)
{
  bool result = false;
  if (memcmp(&k, &opPrefix1, sizeof(opPrefix2)) == 0) {
    result = true;
  } else if (memcmp(&k, &opPrefix2, sizeof(opPrefix2)) == 0) {
    result = true;
  } else if (memcmp(&k, &opPrefix3, sizeof(opPrefix3)) == 0) {
    result = true;
  } else if (isPrivateLabel(k)) {
    result = true;
  }
  return result;
}

void printGeneralizedOperationalPattern(mxfKey& k, FILE* outfile);

void printGeneralizedOperationalPattern(mxfKey& k, FILE* outfile)
{
  mxfByte itemComplexity = k.octet12;
  if ((itemComplexity >= 1) && (itemComplexity <= 3)) {
    const char* number = 0;
    const char* itemCplxName;
    switch (itemComplexity) {
    case 1:
      number = "1";
      itemCplxName = "Single Item";
      break;
    case 2:
      number = "2";
      itemCplxName = "Play-list Items";
      break;
    case 3:
      number = "3";
      itemCplxName = "Edit Items";
      break;
    default:
      itemCplxName = "Unknown";
      break;
    }

    mxfByte packageComplexity = k.octet13;
    const char* letter = 0;
    const char* packageCplxName;
    switch (packageComplexity) {
    case 1:
      letter = "a";
      packageCplxName = "Single Package";
      break;
    case 2:
      letter = "b";
      packageCplxName = "Ganged Packages";
      break;
    case 3:
      letter = "c";
      packageCplxName = "Alternate Versions";
      break;
    default:
      packageCplxName = "Unknown";
     break;
    }
    // mxfByte qualifiers = k.octet14; // tjb not yet decoded
    if ((number != 0) && (letter != 0)) {
      fprintf(outfile, "%s%s - %s, %s",
              number, letter, itemCplxName, packageCplxName);
    } else {
      fprintf(outfile, "%s, %s", itemCplxName, packageCplxName);
    }
  }
}

void printAtomOperationalPattern(mxfKey& /* k */, FILE* outfile);

void printAtomOperationalPattern(mxfKey& /* k */, FILE* outfile)
{
  // tjb - should check for specific Atom patterns
  fprintf(outfile, "Atom");
}

void printSpecializedOperationalPattern(mxfKey& k, FILE* outfile);

void printSpecializedOperationalPattern(mxfKey& k, FILE* outfile)
{
  mxfByte itemComplexity = k.octet12;
  if (itemComplexity == 0x10) {
    printAtomOperationalPattern(k, outfile);
  } else {
    fprintf(outfile, "Unknown");
  }
}

void printOperationalPattern(mxfKey& k, FILE* outfile)
{
  if (isOperationalPattern(k)) {
    fprintf(outfile, "[ ");
    if (!isPrivateLabel(k)) {
      mxfByte itemComplexity = k.octet12;
      if ((itemComplexity >= 1) && (itemComplexity <= 3)) {
        printGeneralizedOperationalPattern(k, outfile);
      } else if ((itemComplexity >= 0x10) && (itemComplexity <= 0x7f)) {
        printSpecializedOperationalPattern(k, outfile);
      } else {
        fprintf(outfile, "Unknown");
      }
    } else {
      printPrivateLabelName(k, outfile);
    }
    fprintf(outfile, " ]");
  } else {
    fprintf(outfile, "[ Invalid ]");
  }
}

mxfByte pvtPrefix1[] = {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0e};
mxfByte pvtPrefix2[] = {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02, 0x0e};
mxfByte pvtPrefix3[] = {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x03, 0x0e};

bool isPrivateLabel(mxfKey& k)
{
  bool result = false;
  if (memcmp(&k, &pvtPrefix1, sizeof(pvtPrefix1)) == 0) {
    result = true;
  } else if (memcmp(&k, &pvtPrefix2, sizeof(pvtPrefix2)) == 0) {
    result = true;
  } else if (memcmp(&k, &pvtPrefix3, sizeof(pvtPrefix3)) == 0) {
    result = true;
  }
  return result;
}

void printPrivateLabelName(mxfKey& k, FILE* outfile)
{
  mxfByte organization = k.octet9;
  const char* name = "Unknown organization";
  switch (organization) {
  case 1:
    name = "MISB Systems";
    break;
  case 2:
    name = "ASPA";
    break;
  case 3:
    name = "MISB Classified";
    break;
  case 4:
    name = "Avid Technology, Inc.";
    break;
  case 5:
    name = "CNN";
    break;
  case 6:
    name = "Sony Corporation";
    break;
  case 7:
    name = "IdeasUnlimited.TV";
    break;
  case 8:
    name = "IPV Ltd";
    break;
  case 9:
    name = "Dolby Laboratories Inc.";
    break;
  case 10:
    name = "Snell & Wilcox";
    break;
  case 11:
    name = "Omneon Video Networks";
    break;
  case 12:
    name = "Ascent Media Group, Inc.";
    break;
  case 13:
    name = "Quantel Ltd";
    break;
  case 14:
    name = "Panasonic";
    break;
  case 15:
    name = "Grass Valley, Inc.";
    break;
  }
  fprintf(outfile, "Private - %s", name);
}

void printPrivateLabel(mxfKey& k, FILE* outfile)
{
  fprintf(outfile, "[ ");
  printPrivateLabelName(k, outfile);
  fprintf(outfile, " ]");
}

typedef std::map<mxfUInt16, const char *> tree;

struct node {
  tree _map;
  const char* _prefix;
  mxfUInt16 _key;
};

typedef std::map<mxfUInt16, node*> forest;

forest f;

node* newNode(mxfUInt16 key, const char* prefix);
void addLabel(node* n, mxfUInt16 key, const char* suffix);
void addNode(node* n);
void initEssenceContainerLabelMap(void);
void decode(mxfUInt16 tag1, mxfUInt32 tag2, FILE* outfile);
bool isEssenceContainerLabel(mxfKey& key);
void printEssenceContainerLabelName(mxfKey& label, FILE* outfile);
void decode(mxfKey& label, FILE* outfile);
void printEssenceContainerLabel(mxfKey& label, mxfUInt32 index, FILE* outfile);

node* newNode(mxfUInt16 key, const char* prefix)
{
  node* result = new node;
  result->_prefix = prefix;
  result->_key = key;
  return result;
}

void addLabel(node* n, mxfUInt16 key, const char* suffix)
{
  n->_map.insert(tree::value_type(key, suffix));
}

void addNode(node* n)
{
  f.insert(forest::value_type(n->_key, n));
}

#define MXF_ESSENCE_CONTAINER_NODE(x, y, p) n = newNode((x << 8) + y, p);
#define MXF_ESSENCE_CONTAINER_LABEL(a, b, s) addLabel(n, (a << 8) + b, s);
#define MXF_ESSENCE_CONTAINER_END() addNode(n);

void initEssenceContainerLabelMap(void)
{
  node* n = 0;
#include "MXFLabels.h"
}

void decode(mxfUInt16 tag1, mxfUInt32 tag2, FILE* outfile)
{
  forest::const_iterator fiter = f.find(tag1);
  if (fiter != f.end()) {
    const node* n = fiter->second;
    fprintf(outfile, "%s", n->_prefix);
    tree::const_iterator titer = n->_map.find(tag2);
    if (titer != n->_map.end()) {
      const char* name = titer->second;
      fprintf(outfile, " - %s", name);
    }
  } else {
    fprintf(outfile, "Not recognized");
  }
}

void decode(mxfKey& label, FILE* outfile)
{
  mxfUInt16 tag1 = 0;
  tag1 = tag1 + (label.octet12 << 8);
  tag1 = tag1 + (label.octet13 << 0);

  mxfUInt16 tag2 = 0;
  tag2 = tag2 + (label.octet14 << 8);
  tag2 = tag2 + (label.octet15 << 0);

  decode(tag1, tag2, outfile);
}

mxfByte eclPfx[] = {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01,
                    0x00, // the version byte is ignored during comparison
                    0x0d, 0x01, 0x03, 0x01};

bool isEssenceContainerLabel(mxfKey& label)
{
  eclPfx[7] = label.octet7; // Will ignore the version byte
  bool result = false;
  if (memcmp(&label, &eclPfx, sizeof(eclPfx)) == 0) {
    result = true;
  }
  eclPfx[7] = 0; // Make the version byte invalid again
  return result;
}

void printEssenceContainerLabelName(mxfKey& label, FILE* outfile)
{
  if (isEssenceContainerLabel(label)) {
    decode(label, outfile);
  } else if (isPrivateLabel(label)) {
    printPrivateLabelName(label, outfile);
  } else {
    fprintf(outfile, "Invalid");
  }
}

void printEssenceContainerLabel(mxfKey& label, mxfUInt32 index, FILE* outfile)
{
  fprintf(stdout, "          ");
  printDecField(stdout, index);
  fprintf(stdout, " = ");
  printMxfKey(label, outfile);
  fprintf(stdout, "\n");
  fprintf(stdout, "                     = [ ");
  printEssenceContainerLabelName(label, outfile);
  fprintf(stdout, " ]\n");
}

bool reorder(void)
{
  bool result;
  if (hostByteOrder() == 'B') {
    result = false;
  } else {
    result = true;
  }
  return result;
}

mxfUInt08 hostByteOrder(void)
{
  mxfUInt16 word = 0x1234;
  mxfUInt08  byte = *((mxfUInt08*)&word);
  mxfUInt08 result;

  if (byte == 0x12) {
    result = 'B';
  } else {
    result = 'L';
  }
  return result;
}

// Define values of MXF keys

#define MXF_LABEL(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
                 {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p}

#define MXF_DEFINE_PACK_KEY(n, k)     const mxfKey n = k;
#define MXF_DEFINE_KEY(n, k)          const mxfKey n = k;
#define MXF_CLASS(name, id, parent, concrete) const mxfKey name = id;

#include "MXFMetaDictionary.h"
// keys not in MXFMetaDictionary.h
const mxfKey V10RandomIndexMetadata =
  {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01,
   0x0d, 0x01, 0x02, 0x01, 0x01, 0x11, 0x00, 0x00};
const mxfKey V10IndexTableSegment =
  {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01,
   0x0d, 0x01, 0x02, 0x01, 0x01, 0x10, 0x00, 0x00};
const mxfKey SystemMetadata =
  {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01,
   0x0d, 0x01, 0x03, 0x01, 0x04, 0x01, 0x01, 0x00};
const mxfKey V1KLVFill =
  {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01,
   0x03, 0x01, 0x02, 0x10, 0x01, 0x00, 0x00, 0x00};
const mxfKey BogusFill =
  {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01,
   0x03, 0x01, 0x02, 0x10, 0x01, 0x01, 0x01, 0x00};
//
const mxfKey ObjectDirectory =
  {0x96, 0x13, 0xb3, 0x8a, 0x87, 0x34, 0x87, 0x46,
   0xf1, 0x02, 0x96, 0xf0, 0x56, 0xe0, 0x4d, 0x2a};

const mxfKey MetaDictionary =
  {0x8A, 0xE5, 0x95, 0x9D, 0x57, 0xB3, 0xDA, 0x33,
   0x8A, 0x5F, 0xB4, 0x11, 0x4D, 0x66, 0x4B, 0x40};

// Define map key <-> key name

#define MXF_DEFINE_PACK_KEY(n, k)     {#n, &n},
#define MXF_DEFINE_KEY(n, k)          {#n, &n},
#define MXF_CLASS(name, id, parent, concrete) {"MXF"#name, &name},

struct Key {
  const char* _name;
  const mxfKey* _key;
} mxfKeyTable [] = {
#include "MXFMetaDictionary.h"
  // keys not in MXFMetaDictionary.h
  {"V10RandomIndexMetadata", &V10RandomIndexMetadata},
  {"V10IndexTableSegment", &V10IndexTableSegment},
  {"SystemMetadata", &SystemMetadata},
  // Although technically invalid, the version 1 Fill key is widely used.
  // Let the dumper show it as regular Fill item.
  {"KLVFill", &V1KLVFill},
  {"BogusFill", &BogusFill},
  //
  {"ObjectDirectory", &ObjectDirectory},
  {"MetaDictionary", &MetaDictionary},
  //
  {"bogus", 0}
};

size_t mxfKeyTableSize = (sizeof(mxfKeyTable)/sizeof(mxfKeyTable[0])) - 1;

#define MXF_LABEL(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
                 {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p}
#define MXF_PROPERTY(name, id, tag, type, mandatory, isuid, container) \
{#name, tag, id},

struct MXFLocalKey {
  const char* _name;
  mxfLocalKey _localKey;
  mxfKey _key;
} mxfLocalKeyTable [] = {
#include "MXFMetaDictionary.h"
  // local keys not in MXFMetaDictionary.h
  // Index Table (not a metadata set)
  {"EditUnitByteCount",    0x3f05, NULLMXFKEY},
  {"IndexSID",             0x3f06, NULLMXFKEY},
  {"BodySID",              0x3f07, NULLMXFKEY},
  {"SliceCount",           0x3f08, NULLMXFKEY},
  {"DeltaEntryArray",      0x3f09, NULLMXFKEY},
  {"IndexEntryArray",      0x3f0a, NULLMXFKEY},
  {"IndexEditRate",        0x3f0b, NULLMXFKEY},
  {"IndexStartPosition",   0x3f0c, NULLMXFKEY},
  {"IndexDuration",        0x3f0d, NULLMXFKEY},
  {"PosTableCount",        0x3f0e, NULLMXFKEY},
  // Sentinel
  {"bogus",                0x0000, NULLMXFKEY}
};

size_t mxfLocalKeyTableSize =
                    (sizeof(mxfLocalKeyTable)/sizeof(mxfLocalKeyTable[0])) - 1;

bool lookupMXFLocalKey(mxfLocalKey& k, size_t& index)
{
  bool result = false;
  for (size_t i = 0; i < mxfLocalKeyTableSize; i++) {
    if (mxfLocalKeyTable[i]._localKey == k) {
      index = i;
      result = true;
      break;
    }
  }
  return result;
}

bool lookupMXFPropertyKey(const mxfKey& k, size_t& index)
{
  bool result = false;
  for (size_t i = 0; i < mxfLocalKeyTableSize; i++) {
    if (memcmp(&mxfLocalKeyTable[i]._key, &k, sizeof(mxfKey)) == 0) {
      index = i;
      result = true;
      break;
    }
  }
  return result;
}

void updateMXFLocalKey(const mxfKey& key, const mxfLocalKey localKey)
{
  size_t index;
  bool found = lookupMXFPropertyKey(key, index);
  if (found) {
    if (mxfLocalKeyTable[index]._localKey != localKey) {
      if (mxfLocalKeyTable[index]._localKey == 0x0000) {
        // Set dynamically allocated local key
        mxfLocalKeyTable[index]._localKey = localKey;
      } else {
        mxfWarning("Cannot remap static local key as specified by Primer Pack "
                   "(property \"%s\" has local key %04" MXFPRIx16 " in the MXF "
                   "dictionary and %04" MXFPRIx16 " in the Primer)\n",
                   mxfLocalKeyTable[index]._name,
                   mxfLocalKeyTable[index]._localKey,
                   localKey );
      }
    }
  }
}

void checkLocalKeyTable(void)
{
  size_t i;
  size_t j;
  for (i = 0; i < mxfLocalKeyTableSize; i++) {
    for (j = 0; j < mxfLocalKeyTableSize; j++) {
      if (i != j) {
        if (mxfLocalKeyTable[i]._localKey == mxfLocalKeyTable[j]._localKey) {
          error("MXF local key is not unique - %s has the same key as %s.\n",
                mxfLocalKeyTable[i]._name,
                mxfLocalKeyTable[j]._name);
        }
      }
    }
  }
}

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#define NULLMXFKEY {0}
#else
#define NULLMXFKEY nullMxfKey
#endif

#define AAF_CLASS(name, id, parent, concrete) {"AAF"#name, NULLMXFKEY, id},

#if defined(DMS1)
#define DMS1_CLASS(name, id) {"DMS1"#name, NULLMXFKEY, id},
#define DMS1_LITERAL_AUID(l, w1, w2,  b1, b2, b3, b4, b5, b6, b7, b8) \
                         {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#endif

struct AAFKey {
  const char* _name;
  mxfKey _key;
  aafUID _aafKey;
} aafKeyTable [] = {
#include "AAFMetaDictionary.h"
#if defined(DMS1)
#include "DMS1MetaDictionary.h"
#endif
  // keys not in AAFMetaDictionary.h
  {"Root", NULLMXFKEY,
// {B3B398A5-1C90-11d4-8053-080036210804}
{0xb3b398a5, 0x1c90, 0x11d4,{0x80, 0x53, 0x08, 0x00, 0x36, 0x21, 0x08, 0x04}}},
  {"Root", NULLMXFKEY,
// {0D010101-0300-0000-060E-2B3402060101}
{0x0d010101, 0x0300, 0x0000,{0x06, 0x0e, 0x2b, 0x34, 0x02, 0x06, 0x01, 0x01}}},
  //
  {"AAFMPEG2VideoDescriptor", NULLMXFKEY,
// {0D010101-0101-5100-060E-2B3402060101}
// 06.0E.2B.34.02.53.01.01.0D.01.01.01.01.01.51.00
{0x0d010101, 0x0101, 0x5100,{0x06, 0x0e, 0x2b, 0x34, 0x02, 0x06, 0x01, 0x01}}},
  // Avid specific class extensions
// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
  {"AvidRLEDescriptor", NULLMXFKEY,
{0xb54e6c75, 0xdf3a, 0x11d3,{0xa0, 0x78, 0x00, 0x60, 0x94, 0xeb, 0x75, 0xcb}}},
// {AD7212AF-21F4-11d5-B3B7-0006294303FA}
  {"AvidMPGIDescriptor", NULLMXFKEY,
{0xad7212af, 0x21f4, 0x11d5,{0xb3, 0xb7, 0x00, 0x06, 0x29, 0x43, 0x03, 0xfa}}},
// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
  {"AvidSD2Descriptor", NULLMXFKEY,
{0xf0c92c05, 0xefad, 0x11d3,{0xa0, 0x7a, 0x00, 0x60, 0x94, 0xeb, 0x75, 0xcb}}},
// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
  {"AvidMCMobReference", NULLMXFKEY,
{0x6619f8e0, 0xfe77, 0x11d3,{0xa0, 0x84, 0x00, 0x60, 0x94, 0xeb, 0x75, 0xcb}}},
// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
  {"AvidAttrEffect", NULLMXFKEY,
{0xc1be981a, 0x0037, 0x11d4,{0xa0, 0x85, 0x00, 0x60, 0x94, 0xeb, 0x75, 0xcb}}},
// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
  {"AvidCCParamObject", NULLMXFKEY,
{0xd0541d8c, 0x00ec, 0x11d4,{0xa0, 0x86, 0x00, 0x60, 0x94, 0xeb, 0x75, 0xcb}}},
// {13E0A981-0412-11d4-9FF9-0004AC969F50}
  {"AvidTKMNTrackerData", NULLMXFKEY,
{0x13e0a981, 0x0412, 0x11d4,{0x9f, 0xf9, 0x00, 0x04, 0xac, 0x96, 0x9f, 0x50}}},
// {30A42454-069E-11d4-9FFB-0004AC969F50}
  {"AvidTKMNTrackedParam", NULLMXFKEY,
{0x30a42454, 0x069e, 0x11d4,{0x9f, 0xfb, 0x00, 0x04, 0xac, 0x96, 0x9f, 0x50}}},
// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
  {"AvidSourceFileDescriptor", NULLMXFKEY,
{0xa4d2a4a6, 0x04bc, 0x11d4,{0xa0, 0x87, 0x00, 0x60, 0x94, 0xeb, 0x75, 0xcb}}},
  // Sentinel
  {"bogus", NULLMXFKEY,
// {00000000-0000-0000-0000-000000000000}
{0x00000000, 0x0000, 0x0000,{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}}
};

size_t aafKeyTableSize = (sizeof(aafKeyTable)/sizeof(aafKeyTable[0])) - 1;

int compareKey(const void* k1, const void* k2);

int compareKey(const void* k1, const void* k2)
{
  const char* n1 = (*(Key**)k1)->_name;
  const char* n2 = (*(Key**)k2)->_name;
  return strcmp(n1, n2);
}

void dumpKeyTable(void);

void dumpKeyTable(void)
{
    size_t i;
    Key** t = new Key*[mxfKeyTableSize];
    for (i = 0; i < mxfKeyTableSize; i++) {
      t[i] = &mxfKeyTable[i];
    }
    qsort(t, mxfKeyTableSize, sizeof(Key*), compareKey);
    for (i = 0; i < mxfKeyTableSize; i++) {
      fprintf(stdout, "%s\n", t[i]->_name);
      fprintf(stdout, "  ");
      printMxfKey(*t[i]->_key, stdout);
      fprintf(stdout, "\n");
    }
    delete [] t;
}

void checkKeyTable(void);

void checkKeyTable(void)
{
  size_t i;
  size_t j;
  for (i = 0; i < mxfKeyTableSize; i++) {
    for (j = 0; j < mxfKeyTableSize; j++) {
      if (i != j) {
        if (memcmp(mxfKeyTable[i]._key,
                   mxfKeyTable[j]._key,
                   sizeof(mxfKey)) == 0) {
          error("MXF key is not unique - %s has the same key as %s.\n",
                mxfKeyTable[i]._name,
                mxfKeyTable[j]._name);
        }
      }
    }
  }
}

void checkAAFKeyTable(void);

void checkAAFKeyTable(void)
{
  size_t i;
  size_t j;
  for (i = 0; i < aafKeyTableSize; i++) {
    for (j = 0; j < aafKeyTableSize; j++) {
      if (i != j) {
        if (memcmp(&aafKeyTable[i]._aafKey,
                   &aafKeyTable[j]._aafKey,
                   sizeof(aafUID)) == 0) {
          error("AAF key is not unique - %s has the same key as %s.\n",
                aafKeyTable[i]._name,
                aafKeyTable[j]._name);
        }
      }
    }
  }
}

int compareAAFKey(const void* k1, const void* k2);

int compareAAFKey(const void* k1, const void* k2)
{
  const char* n1 = (*(AAFKey**)k1)->_name;
  const char* n2 = (*(AAFKey**)k2)->_name;
  return strcmp(n1, n2);
}

void dumpAAFKeyTable(void);

void dumpAAFKeyTable(void)
{
    size_t i;
    AAFKey** t = new AAFKey*[aafKeyTableSize];
    for (i = 0; i < aafKeyTableSize; i++) {
      t[i] = &aafKeyTable[i];
    }
    qsort(t, aafKeyTableSize, sizeof(AAFKey*), compareAAFKey);
    for (i = 0; i < aafKeyTableSize; i++) {
      fprintf(stdout, "%s\n", t[i]->_name);
      fprintf(stdout, "  ");
      printMxfKey(t[i]->_key, stdout);
      fprintf(stdout, "\n");
    }
    delete [] t;
}

void aafUIDToMxfKey(mxfKey& key, aafUID& aafID);

void aafUIDToMxfKey(mxfKey& key, aafUID& aafID)
{
  // Bottom half of key <- top half of aafID
  //
  key.octet0  = aafID.Data4[0];
  key.octet1  = aafID.Data4[1];
  key.octet2  = aafID.Data4[2];
  key.octet3  = aafID.Data4[3];
  key.octet4  = aafID.Data4[4];
  key.octet5  = aafID.Data4[5];
  key.octet6  = aafID.Data4[6];
  key.octet7  = aafID.Data4[7];

  // Top half of key <- bottom half of aafID
  //
  key.octet8  = (aafID.Data1 & 0xff000000) >> 24;
  key.octet9  = (aafID.Data1 & 0x00ff0000) >> 16;
  key.octet10 = (aafID.Data1 & 0x0000ff00) >>  8;
  key.octet11 = (aafID.Data1 & 0x000000ff);

  key.octet12 = (aafID.Data2 & 0xff00) >> 8;
  key.octet13 = (aafID.Data2 & 0x00ff);

  key.octet14 = (aafID.Data3 & 0xff00) >> 8;
  key.octet15 = (aafID.Data3 & 0x00ff);

  // If aafID is an AAF class AUID, map it to a SMPTE 336M local set key
  //
  mxfByte classIdPrefix[] = {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x06};
  if (memcmp(&key, &classIdPrefix, sizeof(classIdPrefix)) == 0) {
    key.octet5 = 0x53;
  }
}

void initAAFKeyTable(void);

void initAAFKeyTable(void)
{
  for (size_t i = 0; i < aafKeyTableSize; i++) {
    aafUIDToMxfKey(aafKeyTable[i]._key, aafKeyTable[i]._aafKey);
  }
}

bool lookupAAFKey(mxfKey& k, size_t& index)
{
  bool result = false;
  for (size_t i = 0; i < aafKeyTableSize; i++) {
    if (memcmp(&k, &aafKeyTable[i]._key, sizeof(mxfKey)) == 0) {
      index = i;
      result = true;
      break;
    }
  }
  return result;
}

bool aafKeysAsSets = true;
bool bogusKeysAsSets = true;
bool darkKeysAsSets = false;

bool findAAFKey(mxfKey& k, size_t& index, char** flag)
{
  bool found = lookupAAFKey(k, index);
  if (found) {
    *flag = const_cast<char*>(""); // A valid SMPTE label
  } else {
    if (aafKeysAsSets) {
      // This could be an AUID/GUID that cannot be mapped to a SMPTE label.
      // Force the mapping and try again.
      mxfKey x;
      memcpy(&x, &k, sizeof(x));
      x.octet5 = 0x53;
      found = lookupAAFKey(x, index);
      if (found) {
        *flag = const_cast<char*>(" +"); // A valid key but not a SMPTE label
      } else {
        if (bogusKeysAsSets) {
          // This could be a bogus key (Intel byte order GUID)
          // Fix it up and try again.
          aafUID a;
          memcpy(&a, &k, sizeof(a));
          if (hostByteOrder() == 'B') {
            reorder(a);
          }
          mxfKey b;
          aafUIDToMxfKey(b, a);
          found = lookupAAFKey(b, index);
          if (found) {
            *flag = const_cast<char*>(" -"); // A bogus key
          }
        }
      }
    }
  }
  return found;
}

bool isMxfKey(mxfKey& k);

bool isMxfKey(mxfKey& k)
{
  size_t index;
  bool result = lookupMXFKey(k, index);
  return result;
}

bool isAAFKey(mxfKey& k);

bool isAAFKey(mxfKey& k)
{
  size_t index;
  char* flag;
  bool found = findAAFKey(k, index, &flag);
  return found;
}

const mxfLocalKey nullMxfLocalKey = 0x00;

void checkSizes(void);

void checkSizes(void)
{
  if (sizeof(mxfLength) != 8) {
    fatalError("Wrong sizeof(mxfLength).\n");
  }
  if (sizeof(mxfKey) != 16) {
    fatalError("Wrong sizeof(mxfKey).\n");
  }
  if (sizeof(mxfByte) != 1) {
    fatalError("Wrong sizeof(mxfByte).\n");
  }
}

char map(int c);
void init(mxfUInt64 start, int base);
void flush(void);
void dumpByte(mxfByte byte);

// Interpret values 0x00 - 0x7f as ASCII characters.
//
static const char table[128] = {
'.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',
'.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',
'.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',
'.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',
' ',  '!',  '"',  '#',  '$',  '%',  '&', '\'',
'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
'X',  'Y',  'Z',  '[', '\\',  ']',  '^',  '_',
'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
'x',  'y',  'z',  '{',  '|',  '}',  '~',  '.'};

char map(int c)
{
  char result;
  if (c < 0x80) {
    result = table[c & 0x7f];
  } else {
    result = '.';
  }
  return result;
}

unsigned char buffer[16];
size_t bufferIndex;
size_t bufferStart;
mxfUInt32 align;
mxfUInt32 address;
size_t addressBase;

void init(mxfUInt64 start, int base)
{
  bufferIndex = 0;
  bufferStart = bufferIndex;
  address = static_cast<mxfUInt32>(start);
  align = address % 16;
  addressBase = 10;
  if (base == 16) {
    addressBase = 16;
  }
}

void flush(void)
{
  if (bufferIndex > 0) {
    if (addressBase == 10) {
      printDecField(stdout, address);
    } else {
      printHexField(stdout, address);
    }
    fprintf(stdout, "  ");
    for (size_t x = 0; x < bufferStart; x++) {
      fprintf(stdout, "   ");
    }
    for (size_t i = bufferStart; i < bufferIndex; i++) {
      fprintf(stdout, "%02x ", buffer[i]);
    }
    fprintf(stdout, "   ");
    for (size_t j = bufferStart; j < 16 - bufferIndex; j++) {
      fprintf(stdout, "   ");
    }
    for (size_t z = 0; z < bufferStart; z++) {
      fprintf(stdout, " ");
    }
    for (size_t k = bufferStart; k < bufferIndex; k++) {
      char c = map(buffer[k]);
      fprintf(stdout, "%c", c);
    }
    fprintf(stdout, "\n");
  }
}

void dumpByte(mxfByte byte)
{
  if (bufferIndex == 16) {
    flush();
    bufferIndex = 0;
    bufferStart = bufferIndex;
    address = address + 16;
  } else if (bufferIndex == 0) {
    if (align != 0) {
      // align
      bufferIndex = align;
      bufferStart = bufferIndex;
      address = address - align;
    }
  }
  buffer[bufferIndex++] = byte;
}

void printCommonOptions(void);

void printCommonOptions(void)
{
  fprintf(stderr, "  --search-run-in <n> = ");
  fprintf(stderr, "search <n> bytes of run-in");
  fprintf(stderr, "[default, n = 64k]\n");

  fprintf(stderr, "  --verbose           = ");
  fprintf(stderr, "print more detailed information (-v)\n");

  fprintf(stderr, "  --debug             = ");
  fprintf(stderr, "print debuging information (-d)\n");
  fprintf(stderr, "\n");

  fprintf(stderr, "--help                = ");
  fprintf(stderr, "print this message and exit (-h)\n");
  fprintf(stderr, "\n");
}

void printCommonDumpOptions(void);

void printCommonDumpOptions(void)
{
  fprintf(stderr, "  --key-addresses     = ");
  fprintf(stderr, "print key addresses ");
  fprintf(stderr, "- always absolute (-j) [default]\n");

  fprintf(stderr, "  --no-key-addresses  = ");
  fprintf(stderr, "don't print key addresses\n");

  fprintf(stderr, "  --relative          = ");
  fprintf(stderr, "relative addresses ");
  fprintf(stderr, "- value start = 0 (-r) [default]\n");

  fprintf(stderr, "  --absolute          = ");
  fprintf(stderr, "absolute addresses ");
  fprintf(stderr, "- file start = 0 (-b)\n");

  fprintf(stderr, "  --decimal           = ");
  fprintf(stderr, "print addresses in ");
  fprintf(stderr, "decimal (-t)\n");

  fprintf(stderr, "  --hexadecimal       = ");
  fprintf(stderr, "print addresses in ");
  fprintf(stderr, "hexadecimal (-x) [default]\n");

  fprintf(stderr, "  --symbolic          = ");
  fprintf(stderr, "dump the names of keys if known (-y) [default]\n");

  fprintf(stderr, "  --no-symbolic       = ");
  fprintf(stderr, "don't dump the names of keys (-n)\n");
  fprintf(stderr, "\n");

  fprintf(stderr, "  --statistics        = ");
  fprintf(stderr, "print statistics\n");
  fprintf(stderr, "\n");
}

void printCommonValidateOptions(void);

void printCommonValidateOptions(void)
{
}

void printFormatOptions(void);

void printFormatOptions(void)
{
  fprintf(stderr, "  --limit-bytes <n>   = ");
  fprintf(stderr, "truncate essence ");
  fprintf(stderr, "to <n> bytes (-l) [default, n = 0]\n");

  fprintf(stderr, "  --no-limit-bytes    = ");
  fprintf(stderr, "do not truncate ");
  fprintf(stderr, "essence (-e)\n");

  fprintf(stderr, "  --frames            = ");
  fprintf(stderr, "assume essence is ");
  fprintf(stderr, "wrapped frames (-p)\n");

  fprintf(stderr, "  --limit-frames <n>  = ");
  fprintf(stderr, "truncate frame-wrapped essence ");
  fprintf(stderr, "to <n> frames\n");
  fprintf(stderr, "                        ");
  fprintf(stderr, "[default, n = 0]\n");

  fprintf(stderr, "  --no-limit-frames   = ");
  fprintf(stderr, "do not truncate ");
  fprintf(stderr, "frame-wrapped essence\n");

  fprintf(stderr, "  --limit-entries <n> = ");
  fprintf(stderr, "print only the first <n> ");
  fprintf(stderr, "index table entries (-c)\n");
  fprintf(stderr, "                        ");
  fprintf(stderr, "[default, n = 0]\n");

  fprintf(stderr, "  --no-limit-entries  = ");
  fprintf(stderr, "print all ");
  fprintf(stderr, "index table entries\n");

  fprintf(stderr, "  --show-fill         = ");
  fprintf(stderr, "dump fill bytes (-f)\n");

  fprintf(stderr, "  --show-dark         = ");
  fprintf(stderr, "dump dark data (-w)\n");

  fprintf(stderr, "  --show-run-in       = ");
  fprintf(stderr, "dump run-in\n");

  fprintf(stderr, "  --diff-friendly     = ");
  fprintf(stderr, "less readable but more useful when diffing mxf files\n");

  fprintf(stderr, "  --show-anc          = ");
  fprintf(stderr, "dump contents of ANC Frame Elements\n");

  fprintf(stderr, "  --ignore-primer     = ");
  fprintf(stderr, "do not use Primer Pack for mapping from local keys\n");
  fprintf(stderr, "                        ");
  fprintf(stderr, "to their respective UIDs\n");
}

void printRawOptions(void);

void printRawOptions(void)
{
  fprintf(stderr, "  --limit-bytes <n>   = ");
  fprintf(stderr, "truncate values ");
  fprintf(stderr, "to <n> bytes (-l)\n");

  fprintf(stderr, "  --no-limit-bytes    = ");
  fprintf(stderr, "do not truncate ");
  fprintf(stderr, "values (-e) [default]\n");
}

void printAAFOptions(void);

void printAAFOptions(void)
{
  printFormatOptions();
}

void printMXFOptions(void);

void printMXFOptions(void)
{
  printFormatOptions();
}

void printSetOptions(void);

void printSetOptions(void)
{
  printFormatOptions();
}

void printKLVOptions(void);

void printKLVOptions(void)
{
  printRawOptions();
}

void printKLVValidateOptions(void);

void printKLVValidateOptions(void)
{
}

void printSetValidateOptions(void);

void printSetValidateOptions(void)
{
}

void printMXFValidateOptions(void);

void printMXFValidateOptions(void)
{
}

void printAAFValidateOptions(void);

void printAAFValidateOptions(void)
{
}

void printUsage(void);

void printUsage(void)
{
  fprintf(stderr,
          "%s : Usage : %s OPTIONS <file>\n",
          programName(),
          programName());

  fprintf(stderr, "--aaf-dump            = ");
  fprintf(stderr, "dump AAF (-a)\n");

  fprintf(stderr, "--mxf-dump            = ");
  fprintf(stderr, "dump MXF (-m) [default]\n");

  fprintf(stderr, "--set-dump            = ");
  fprintf(stderr, "dump local sets (-s)\n");

  fprintf(stderr, "--klv-dump            = ");
  fprintf(stderr, "dump raw KLV (-k)\n");

  fprintf(stderr, "--aaf-validate        = ");
  fprintf(stderr, "validate AAF\n");

  fprintf(stderr, "--mxf-validate        = ");
  fprintf(stderr, "validate MXF\n");

  fprintf(stderr, "--set-validate        = ");
  fprintf(stderr, "validate local sets\n");

  fprintf(stderr, "--klv-validate        = ");
  fprintf(stderr, "validate KLV\n");

  fprintf(stderr, "--help                = ");
  fprintf(stderr, "print detailed help (-h)\n");
}

void printAAFUsage(void);

void printAAFUsage(void)
{
  fprintf(stderr, "--aaf-dump            = ");
  fprintf(stderr, "dump AAF (-a)\n");
  printAAFOptions();
  fprintf(stderr, "\n");
}

void printMXFUsage(void);

void printMXFUsage(void)
{
  fprintf(stderr, "--mxf-dump            = ");
  fprintf(stderr, "dump MXF (-m) [default]\n");
  printMXFOptions();
  fprintf(stderr, "\n");
}

void printSetUsage(void);

void printSetUsage(void)
{
  fprintf(stderr, "--set-dump            = ");
  fprintf(stderr, "dump local sets (-s)\n");
  printSetOptions();
  fprintf(stderr, "\n");
}

void printKLVUsage(void);

void printKLVUsage(void)
{
  fprintf(stderr, "--klv-dump            = ");
  fprintf(stderr, "dump raw KLV (-k)\n");
  printKLVOptions();
  fprintf(stderr, "\n");
}

void printKLVValidateUsage(void);

void printKLVValidateUsage(void)
{
  fprintf(stderr, "--klv-validate        = ");
  fprintf(stderr, "validate KLV\n");
  printKLVValidateOptions();
  fprintf(stderr, "\n");
}

void printSetValidateUsage(void);

void printSetValidateUsage(void)
{
  fprintf(stderr, "--set-validate        = ");
  fprintf(stderr, "validate local sets\n");
  printSetValidateOptions();
  fprintf(stderr, "\n");
}

void printMXFValidateUsage(void);

void printMXFValidateUsage(void)
{
  fprintf(stderr, "--mxf-validate        = ");
  fprintf(stderr, "validate MXF\n");
  printMXFValidateOptions();
  fprintf(stderr, "\n");
}

void printAAFValidateUsage(void);

void printAAFValidateUsage(void)
{
  fprintf(stderr, "--aaf-validate        = ");
  fprintf(stderr, "validate AAF\n");
  printAAFValidateOptions();
  fprintf(stderr, "\n");
}

void printFullUsage(void);

void printFullUsage(void)
{
  fprintf(stderr,
          "%s : Usage : %s OPTIONS <file>\n",
          programName(),
          programName());

  printAAFUsage();
  printMXFUsage();
  printSetUsage();
  printKLVUsage();

  printCommonDumpOptions();

  printAAFValidateUsage();
  printMXFValidateUsage();
  printSetValidateUsage();
  printKLVValidateUsage();

  printCommonOptions();
}

void printHelp(void);

void printHelp(void)
{
  if (mode == klvMode) {
    printKLVUsage();
    printCommonDumpOptions();
    printCommonOptions();
  } else if (mode == localSetMode) {
    printSetUsage();
    printCommonDumpOptions();
    printCommonOptions();
  } else if (mode == mxfMode) {
    printMXFUsage();
    printCommonDumpOptions();
    printCommonOptions();
  } else if (mode == aafMode) {
    printAAFUsage();
    printCommonDumpOptions();
    printCommonOptions();
  } else if (mode == klvValidateMode) {
    printKLVValidateUsage();
    printCommonValidateOptions();
    printCommonOptions();
  } else if (mode == setValidateMode) {
    printSetValidateUsage();
    printCommonValidateOptions();
    printCommonOptions();
  } else if (mode == mxfValidateMode) {
    printMXFValidateUsage();
    printCommonValidateOptions();
    printCommonOptions();
  } else if (mode == aafValidateMode) {
    printAAFValidateUsage();
    printCommonValidateOptions();
    printCommonOptions();
  } else {
    printFullUsage();
  }
}

const char* baseName(const char* fullName)
{
  const char* result;
#if defined(MXF_OS_WINDOWS)
  const int delimiter = '\\';
#elif defined(MXF_OS_MACOS)
  const int delimiter = ':';
#else
  const int delimiter = '/';
#endif
  result = strrchr(fullName, delimiter);
  if (result == 0) {
    result = fullName;
  } else if (strlen(result) == 0) {
    result = fullName;
  } else {
    result++;
  }

  return result;
}

#define AAF_PROPERTY(name, id, tag, type, mandatory, uid, container) \
{#name, tag, id, NULLMXFKEY},

struct AAFLocalKey {
  const char* _name;
  mxfUInt16 _localKey;
  aafUID _aafKey;
  mxfKey _key;
} aafLocalKeyTable [] = {
#include "AAFMetaDictionary.h"
  // local keys not in AAFMetaDictionary.h
  // Root
  {"MetaDictionary",       0x0001, NULLAAFUID, NULLMXFKEY},
  {"Header",               0x0002, NULLAAFUID, NULLMXFKEY},
  {"ObjectDirectory",      0x0021, NULLAAFUID, NULLMXFKEY},
  {"FormatVersion",        0x0022, NULLAAFUID, NULLMXFKEY},
  // Invalid legacy Root local keys; conflict with SMPTE standard local keys.
  {"ObjectDirectory",      0x7f03, NULLAAFUID, NULLMXFKEY}, // 0x0003
  {"FormatVersion",        0x7f04, NULLAAFUID, NULLMXFKEY}, // 0x0004
  // All objects
  {"InstanceUID",          0x3c0a,
{0x01011502, 0x0000, 0x0000, {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01}},
  NULLMXFKEY},
  // Preface
  {"Primary Package",      0x3b08,
{0x06010104, 0x0108, 0x0000, {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x04}},
  NULLMXFKEY},
  // Index Table
  {"Edit Unit Byte Count", 0x3f05, NULLAAFUID, NULLMXFKEY},
  {"IndexSID",             0x3f06,
{0x01030405, 0x0000, 0x0000, {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x04}},
  NULLMXFKEY},
  {"BodySID",              0x3f07,
{0x01030404, 0x0000, 0x0000, {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x04}},
  NULLMXFKEY},
  {"Slice Count",          0x3f08, NULLAAFUID, NULLMXFKEY},
  {"Delta Entry Array",    0x3f09, NULLAAFUID, NULLMXFKEY},
  {"Index Entry Array",    0x3f0a, NULLAAFUID, NULLMXFKEY},
  {"Index Edit Rate",      0x3f0b, NULLAAFUID, NULLMXFKEY},
  {"Index Start Position", 0x3f0c, NULLAAFUID, NULLMXFKEY},
  {"Index Duration",       0x3f0d, NULLAAFUID, NULLMXFKEY},
  {"PosTableCount",        0x3f0e, NULLAAFUID, NULLMXFKEY},
  //
  // Sentinel
  {"bogus",                0x0000, NULLAAFUID, NULLMXFKEY}
};

#undef NULLMXFKEY

size_t aafLocalKeyTableSize =
                    (sizeof(aafLocalKeyTable)/sizeof(aafLocalKeyTable[0])) - 1;

void initAAFLocalKeyTable(void)
{
  for (size_t i = 0; i < aafLocalKeyTableSize; i++) {
    aafUIDToMxfKey(aafLocalKeyTable[i]._key, aafLocalKeyTable[i]._aafKey);
  }
}

bool lookupAAFLocalKey(mxfLocalKey& k, size_t& index)
{
  bool result = false;
  for (size_t i = 0; i < aafLocalKeyTableSize; i++) {
    if (aafLocalKeyTable[i]._localKey == k) {
      index = i;
      result = true;
      break;
    }
  }
  return result;
}

bool lookupAAFPropertyKey(const mxfKey& k, size_t& index)
{
  bool result = false;
  for (size_t i = 0; i < aafLocalKeyTableSize; i++) {
    if (memcmp(&aafLocalKeyTable[i]._key, &k, sizeof(mxfKey)) == 0) {
      index = i;
      result = true;
      break;
    }
  }
  return result;
}

void updateAAFLocalKey(const mxfKey& key, const mxfLocalKey localKey)
{
  size_t index;
  bool found = lookupAAFPropertyKey(key, index);
  if (found) {
    if (aafLocalKeyTable[index]._localKey != localKey) {
      if (aafLocalKeyTable[index]._localKey == 0x0000) {
        // Set dynamically allocated local key
        aafLocalKeyTable[index]._localKey = localKey;
      } else {
        mxfWarning("Cannot remap static local key as specified by Primer Pack "
                   "(property \"%s\" has local key %04" MXFPRIx16 " in the AAF "
                   "dictionary and %04" MXFPRIx16 " in the Primer)\n",
                   aafLocalKeyTable[index]._name,
                   aafLocalKeyTable[index]._localKey,
                   localKey );
      }
    }
  }
}

void checkAAFLocalKeyTable(void)
{
  size_t i;
  size_t j;
  for (i = 0; i < aafLocalKeyTableSize; i++) {
    for (j = 0; j < aafLocalKeyTableSize; j++) {
      if (i != j) {
        if (aafLocalKeyTable[i]._localKey == aafLocalKeyTable[j]._localKey) {
          error("AAF local key is not unique - %s has the same key as %s.\n",
                aafLocalKeyTable[i]._name,
                aafLocalKeyTable[j]._name);
        }
      }
    }
  }
}

class mxfKeyLess {
public:
  bool operator()(const mxfKey& k1, const mxfKey& k2) const;
};

bool mxfKeyLess::operator()(const mxfKey& k1, const mxfKey& k2) const
{
  bool result = false;
  if (memcmp(&k1, &k2, sizeof(mxfKey)) < 0) {
    result = true;
  }
  return result;
}

typedef std::set<mxfKey, mxfKeyLess> ObjectSet;
ObjectSet objects;

typedef std::map<mxfUInt16, mxfKey> PrimerMap;
PrimerMap primer;

typedef std::set<mxfUInt16> IdSet;
IdSet badids;

bool symbolic = true;

void printMxfLocalKeySymbol(mxfLocalKey& k, mxfKey& enclosing);

void printMxfLocalKeySymbol(mxfLocalKey& k, mxfKey& enclosing)
{
  if (isMxfKey(enclosing)) {
    const char* name = mxfLocalKeyName(k);
    fprintf(stdout, "%s\n", name);
  } else {
    fprintf(stdout, "Dark\n");
  }
}

void printAAFLocalKeySymbol(mxfLocalKey& k, mxfKey& enclosing);

void printAAFLocalKeySymbol(mxfLocalKey& k, mxfKey& enclosing)
{
  if (isAAFKey(enclosing)) {
    mxfLocalKey key = k;
    const char* ename = aafKeyName(enclosing);
    if (strcmp(ename, "Root") == 0) {
      if (key == 0x0003) {
        key = 0x7f03;
      } else if (key == 0x0004) {
        key = 0x7f04;
      }
    }
    const char* name = aafLocalKeyName(key);
    fprintf(stdout, "%s\n", name);
  } else {
    fprintf(stdout, "Dark\n");
  }
}

void printMxfLocalKeySymbol(mxfLocalKey& k, mxfKey& enclosing, FILE* f);

void printMxfLocalKeySymbol(mxfLocalKey& k, mxfKey& enclosing, FILE* f)
{
  if (symbolic) {
    if (mode == aafMode) {
      printAAFLocalKeySymbol(k, enclosing);
    } else if (mode == mxfMode) {
      printMxfLocalKeySymbol(k, enclosing);
    } else if (mode == klvMode) {
      printMxfLocalKeySymbol(k, enclosing);
    } else if (mode == localSetMode) {
      printMxfLocalKeySymbol(k, enclosing);
    } else {
      fprintf(f, "Unknown\n");
    }
  } else {
    fprintf(f, "\n");
  }
  fprintf(f, "  ");
  printMxfLocalKey(k, f);
}

bool lookupMXFKey(mxfKey& k, size_t& index)
{
  bool result = false;
  for (size_t i = 0; i < mxfKeyTableSize; i++) {
    if (memcmp(&k, mxfKeyTable[i]._key, sizeof(mxfKey)) == 0) {
      index = i;
      result = true;
      break;
    }
  }
  return result;
}

bool keyAddresses = true;  // Print addresses of keys
bool relative = true;      // Print relative (not absolute) addresses
int base = 16;             // Base for key addreses

void printKeyAddress(FILE* f, mxfUInt64 keyAddress);

void printKeyAddress(FILE* f, mxfUInt64 keyAddress)
{
  fprintf(f, "( ");
  if (base == 10) {
    printDecField(f, keyAddress);
  } else {
    printHexFieldPad(f, keyAddress);
  }
  fprintf(f, " )");
}

void printMxfKeySymbol(mxfKey& k);

void printMxfKeySymbol(mxfKey& k)
{
  size_t i;
  bool found = lookupMXFKey(k, i);
  if (found) {
    fprintf(stdout, "%s", mxfKeyTable[i]._name);
  } else if (isEssenceElement(k)) {
    fprintf(stdout, "Essence Element");
  } else {
    fprintf(stdout, "Dark");
  }
  if (keyAddresses) {
    fprintf(stdout, " ");
    printKeyAddress(stdout, keyPosition);
  }
  fprintf(stdout, "\n");
}

void printAAFKeySymbol(mxfKey& k);

void printAAFKeySymbol(mxfKey& k)
{
  size_t i;
  char* flag;
  bool found = findAAFKey(k, i, &flag);
  if (found) {
    fprintf(stdout, "%s%s", aafKeyTable[i]._name, flag);
  } else {
    found = lookupMXFKey(k, i);
    if (found) {
      fprintf(stdout, "%s", mxfKeyTable[i]._name);
    } else if (isEssenceElement(k)) {
      fprintf(stdout, "Essence Element");
    } else {
      fprintf(stdout, "Dark");
    }
  }
  if (keyAddresses) {
    fprintf(stdout, " ");
    printKeyAddress(stdout, keyPosition);
  }
  fprintf(stdout, "\n");
}

void printMxfKeySymbol(mxfKey& k, FILE* f);

void printMxfKeySymbol(mxfKey& k, FILE* f)
{
  if (symbolic) {
    if (mode == aafMode) {
      printAAFKeySymbol(k);
    } else if (mode == mxfMode) {
      printMxfKeySymbol(k);
    } else if (mode == klvMode) {
      printMxfKeySymbol(k);
    } else if (mode == localSetMode) {
      printMxfKeySymbol(k);
    } else {
      fprintf(f, "Unknown\n");
    }
  } else {
    fprintf(f, "\n");
  }
  printMxfKey(k, f);
}

const char* mxfKeyName(const mxfKey& k)
{
  const char* result = "Unknown";
  size_t i;
  bool found = lookupMXFKey(const_cast<mxfKey&>(k), i);
  if (found) {
    result = mxfKeyTable[i]._name;
  }
  return result;
}

const char* aafKeyName(const mxfKey& k)
{
  const char* result = "Unknown";
  size_t i;
  bool found = lookupAAFKey(const_cast<mxfKey&>(k), i);
  if (found) {
    result = aafKeyTable[i]._name;
  }
  return result;
}

const char* mxfLocalKeyName(mxfLocalKey& k)
{
  const char* result = "Dark";
  size_t index;
  bool found = lookupMXFLocalKey(k, index);
  if (found) {
    result = mxfLocalKeyTable[index]._name;
  }
  return result;
}

const char* aafLocalKeyName(mxfLocalKey& k)
{
  const char* result = "Dark";
  size_t index;
  bool found = lookupAAFLocalKey(k, index);
  if (found) {
    result = aafLocalKeyTable[index]._name;
  }
  return result;
}

bool printStats = false;

mxfUInt32 objectCount = 0;
mxfUInt64 objectBytes = 0;

void printObjectCount(const mxfKey& key);

void printObjectCount(const mxfKey& key)
{
  if (printStats) {
    if (objectCount > 0) {
      fprintf(stdout, "\n");
      fprintf(stdout,
              "[ %" MXFPRIu32 " objects (%" MXFPRIu64 " bytes)"
              " found in %s partition ]\n",
              objectCount,
              objectBytes,
              keyName(key));
      objectCount = 0;
      objectBytes = 0;
    }
  }
}

mxfUInt32 darkItems = 0;

void printDarkItems(void);

void printDarkItems(void)
{
  if (darkItems > 0) {
    fprintf(stdout, "\n");
    fprintf(stdout, "[ Omitted %" MXFPRIu32 " dark KLV triplets ]\n", darkItems);
    darkItems = 0;
  }
}

void printKL(mxfKey& k, mxfLength& l, mxfUInt08& llen);

void printKL(mxfKey& k, mxfLength& l, mxfUInt08& llen)
{
  printDarkItems();
  fprintf(stdout, "\n");
  fprintf(stdout, "[ K = ");
  printMxfKeySymbol(k, stdout);
  fprintf(stdout, ", ");
  fprintf(stdout, "L = ");
  printMxfLength(l, stdout);
  fprintf(stdout, ", ");
  fprintf(stdout, "LL = %d", llen);
  fprintf(stdout, " ]\n");
}

void printV(mxfLength& length,
            bool limitBytes,
            mxfUInt32 limit,
            mxfFile infile);

void printV(mxfLength& length,
            bool limitBytes,
            mxfUInt32 limit,
            mxfFile infile)
{
  mxfUInt64 start = position(infile);
  if (relative) {
    start = 0;
  }
  init(start, base);

  mxfLength count = 0;
  for (mxfLength i = 0; i < length; i++) {
    mxfByte b;
    readMxfUInt08(b, infile);
    if (limitBytes && (i == limit)) {
      break;
    }
    dumpByte(b);
    count = count + 1;
  }
  flush();
  if (count < length) {
    fprintf(stdout, "[ Value truncated from ");
    printDecField(stdout, length);
    fprintf(stdout, " bytes to ");
    printDecField(stdout, limit);
    fprintf(stdout, " bytes ]\n");

    mxfUInt64 skipLength = (length - count) - 1;
    skipV(skipLength, infile);
  }
}

void printRunIn(mxfUInt64& headerPosition,
                bool limitBytes,
                mxfUInt32 limit,
                mxfFile infile);

void printRunIn(mxfUInt64& headerPosition,
                bool limitBytes,
                mxfUInt32 limit,
                mxfFile infile)
{
  setPosition(infile, 0);
  fprintf(stdout, "\n");
  fprintf(stdout, "[ %" MXFPRIu64 " bytes of run-in ]\n", headerPosition);
  if (lFlag) {
    printV(headerPosition, limitBytes, limit, infile);
  } else {
    printV(headerPosition, false, 64, infile);
  }
}

const char* itemTypeName(mxfByte itemTypeId);

const char* itemTypeName(mxfByte itemTypeId)
{
  const char* result;
  switch (itemTypeId) {
  case 0x05:
    result = "CP Picture";
    break;
  case 0x06:
    result = "CP Sound";
    break;
  case 0x07:
    result = "CP Data";
    break;
  case 0x15:
    result = "GC Picture";
    break;
  case 0x16:
    result = "GC Sound";
    break;
  case 0x17:
    result = "GC Data";
    break;
  case 0x18:
    result = "GC Compound";
    break;
  default:
    result = "Unknown";
    break;
  }
  return result;
}

const char* CPPictureElementTypeName(mxfByte type);

const char* CPPictureElementTypeName(mxfByte type)
{
  const char* result = "Unknown Picture";
  if (type == 0x01) {
    result = "MPEG-2 (D10)";
  } else if (type == 0x41) {
    result = "JFIF";
  } else if (type == 0x42) {
    result = "TIFF";
  } else if ((type < 0x01) || (type > 0x7f)) {
    result = "Illegal";
  }
  return result;
}

const char* CPSoundElementTypeName(mxfByte type);

const char* CPSoundElementTypeName(mxfByte type)
{
  const char* result = "Unknown Sound";
  if (type == 0x10) {
    result = "AES3";
  } else if (type == 0x40) {
    result = "WAVE";
  } else if ((type < 0x01) || (type > 0x7f)) {
    result = "Illegal";
  }
  return result;
}

const char* CPDataElementTypeName(mxfByte type);

const char* CPDataElementTypeName(mxfByte type)
{
  const char* result = "Unknown Data";
  if ((type < 0x01) || (type > 0x7f)) {
    result = "Illegal";
  }
  return result;
}

const char* GCPictureElementTypeName(mxfByte type);

const char* GCPictureElementTypeName(mxfByte type)
{
  const char* result = "Unknown Picture";
  if (type == 0x01) {
    result = "Compressed HD (D11)";
  } else if (type == 0x02) {
    result = "Uncompressed (Frame Wrapped)";
  } else if (type == 0x03) {
    result = "Uncompressed (Clip Wrapped)";
  } else if (type == 0x04) {
    result = "Uncompressed (Line Wrapped)";
  } else if (type == 0x05) {
    result = "Frame Wrapped";
  } else if (type == 0x06) {
    result = "Clip Wrapped";
  } else if (type == 0x07) {
    result = "Custom Wrapped";
  } else if (type == 0x08) {
    result = "Frame-wrapped JPEG 2000 Picture Element";
  } else if (type == 0x09) {
    result = "Clip-wrapped JPEG 2000 Picture Element";
  } else if (type == 0x0a) {
    result = "VC-1 Picture Element (Frame Wrapped)";
  } else if (type == 0x0b) {
    result = "VC-1 Picture Element (Clip Wrapped)";
  } else if (type == 0x0c) {
    result = "VC-3 Picture Element (Frame Wrapped)";
  } else if (type == 0x0d) {
    result = "VC-3 Picture Element (Clip Wrapped)";
  } else if ((type < 0x01) || (type > 0x7f)) {
    result = "Illegal";
  }
  return result;
}

const char* avidPictureElementTypeName(mxfByte type);

const char* avidPictureElementTypeName(mxfByte type)
{
  const char* result = "Unknown Picture";
  if (type == 0x01) {
    result = "Avid JFIF";
  } else if (type == 0x02) {
    result = "Avid DV";
  } else if (type == 0x03) {
    result = "Avid MPEG";
  } else if (type == 0x04) {
    result = "Avid Uncompressed SD";
  } else if (type == 0x05) {
    result = "Avid Uncompressed HD";
  } else if (type == 0x06) {
    result = "Avid Compressed HD";
  } else if (type == 0x07) {
    result = "Avid Packed 10-bit";
  } else if (type == 0x08) {
    result = "Avid RGBA";
  } else if (type == 0x09) {
    result = "Avid RLE";
  }
  return result;
}

const char* GCSoundElementTypeName(mxfByte type);

const char* GCSoundElementTypeName(mxfByte type)
{
  const char* result = "Unknown Sound";
  if (type == 0x01) {
    result = "Broadcast Wave (Frame Wrapped)";
  } else if (type == 0x02) {
    result = "Broadcast Wave (Clip Wrapped)";
  } else if (type == 0x03) {
    result = "AES3( Frame Wrapped)";
  } else if (type == 0x04) {
    result = "AES3 (Clip Wrapped)";
  } else if ((type < 0x01) || (type > 0x7f)) {
    result = "Illegal";
  }
  return result;
}

const char* avidSoundElementTypeName(mxfByte type);

const char* avidSoundElementTypeName(mxfByte type)
{
  const char* result = "Unknown Sound";
  if (type == 0x20) {
    result = "Avid Sound";
  }
  return result;
}

const char* GCDataElementTypeName(mxfByte type);

const char* GCDataElementTypeName(mxfByte type)
{
  const char* result = "Unknown Data";
  if (type == 0x01) {
    result = "VBI Data Element (Frame Wrapped)";
  } else if (type == 0x02) {
    result = "ANC Data Element (Frame Wrapped)";
  } else if ((type < 0x01) || (type > 0x7f)) {
    result = "Illegal";
  }
  return result;
}

const char* GCCompoundElementTypeName(mxfByte type);

const char* GCCompoundElementTypeName(mxfByte type)
{
  const char* result = "Unknown Compound";
  if (type == 0x01) {
    result = "DV-DIF (Frame Wrapped)";
  } else if (type == 0x02) {
    result = "DV-DIF (Clip Wrapped)";
  } else if ((type < 0x01) || (type > 0x7f)) {
    result = "Illegal";
  }
  return result;
}

const char* elementTypeName(mxfByte itemTypeId, mxfByte type);

const char* elementTypeName(mxfByte itemTypeId, mxfByte type)
{
  const char* result = "Unknown";
  switch (itemTypeId) {
  case 0x05: // "CP Picture"
    result = CPPictureElementTypeName(type);
    break;
  case 0x06: // "CP Sound"
    result = CPSoundElementTypeName(type);
    break;
  case 0x07: // "CP Data"
    result = CPDataElementTypeName(type);
    break;
  case 0x15: // "GC Picture"
    result = GCPictureElementTypeName(type);
    break;
  case 0x16: // "GC Sound"
    result = GCSoundElementTypeName(type);
    break;
  case 0x17: // "GC Data"
    result = GCDataElementTypeName(type);
    break;
  case 0x18: // "GC Compound"
    result = GCCompoundElementTypeName(type);
    break;
  default:
    break;
  }
  return result;
}

const char* avidElementTypeName(mxfByte itemTypeId, mxfByte type);

const char* avidElementTypeName(mxfByte itemTypeId, mxfByte type)
{
  const char* result = "Unknown";
  switch (itemTypeId) {
  case 0x15: // "GC Picture"
    result = avidPictureElementTypeName(type);
    break;
  case 0x16: // "GC Sound"
    result = avidSoundElementTypeName(type);
    break;
  default:
    break;
  }
  return result;
}

bool isSystemElement(mxfKey& k);

bool isSystemElement(mxfKey& k)
{
  bool result;
  if (memcmp(&k, &SystemMetadata, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isDataElement(mxfKey& k);

bool isDataElement(mxfKey& /* k */)
{
  return false;
}

bool isPredefinedEssenceElement(mxfKey& k);

bool isPredefinedEssenceElement(mxfKey& k)
{
  // Prefix for MXF predefined MXF essence element labels
  mxfKey pe = {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
               0x0d, 0x01, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00};
  bool result;
  if (memcmp(&k, &pe, 12) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isLegacyPredefinedEssenceElement(mxfKey& k);

bool isLegacyPredefinedEssenceElement(mxfKey& k)
{
  // Legacy Prefix for MXF predefined MXF essence element labels
  mxfKey lpe = {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
               0x0d, 0x01, 0x03, 0x01, 0xff, 0xff, 0xff, 0xff};
  bool result;
  if (memcmp(&k, &lpe, 12) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isAvidEssenceElement(mxfKey& k);

bool isAvidEssenceElement(mxfKey& k)
{
  // Prefix for Avid defined essence element labels
  mxfKey ae = {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
               0x0e, 0x04, 0x03, 0x01, 0xff, 0xff, 0xff, 0xff};
  bool result;
  if (memcmp(&k, &ae, 12) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isEssenceElement(mxfKey& k)
{
  bool result;
  if (isPredefinedEssenceElement(k)) {
    result = true;
  } else if (isLegacyPredefinedEssenceElement(k)) {
    result = true;
  } else if (isAvidEssenceElement(k)) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isANCFrameElement(mxfKey& k)
{
  bool result;
  if (isEssenceElement(k) && k.octet12 == 0x17 && k.octet14 == 0x02) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isIndexSegment(mxfKey& k)
{
  bool result;
  if (memcmp(&IndexTableSegment, &k, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&V10IndexTableSegment, &k, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool is8bitANCSampleCoding(mxfUInt08 payloadSampleCoding);

bool is8bitANCSampleCoding(mxfUInt08 payloadSampleCoding)
{
  bool result = false;
  switch (payloadSampleCoding) {
  case 4:
  case 5:
  case 6:
  case 10:
  case 11:
  case 12:
    result = true;
    break;
  default:
    break;
  }
  return result;
}

void printANCPayloadByteArray(mxfUInt32 entryCount,
                              mxfUInt32 entrySize,
                              mxfUInt08 payloadSampleCoding,
                              bool limitBytes,
                              mxfUInt32 limit,
                              mxfFile infile);

void printANCPayloadByteArray(mxfUInt32 entryCount,
                              mxfUInt32 entrySize,
                              mxfUInt08 payloadSampleCoding,
                              bool limitBytes,
                              mxfUInt32 limit,
                              mxfFile infile)
{
  fprintf(stdout, "        [ Payload ");

  // Print out DID, SDID/DBN and Data Count words of the payload.
  // Only 8-bit samples are currently supported.
  mxfLength remaining = entryCount * entrySize;
  if (is8bitANCSampleCoding(payloadSampleCoding) && remaining > 3) {
    // Peek at the first 3 bytes
    const mxfUInt64 payloadStart = position(infile);
    mxfUInt08 did;
    mxfUInt08 sdid;
    mxfUInt08 dataCount;
    readMxfUInt08(did, infile);
    readMxfUInt08(sdid, infile);
    readMxfUInt08(dataCount, infile);
    setPosition(infile, payloadStart);
  
    fprintf(stdout, "DID = ");
    printHexFieldPad(stdout, did);
    fprintf(stdout, ", SDID/DBN = ");
    printHexFieldPad(stdout, sdid);
    fprintf(stdout, ", Data Count = ");
    printHexFieldPad(stdout, dataCount);
  }

  fprintf(stdout, " ]\n");

  // Print out payload bytes
  printV(remaining, limitBytes, limit, infile);
}

void printANCFrameElement(mxfLength& length,
                         bool limitBytes,
                         mxfUInt32 limit,
                         mxfFile infile);

void printANCFrameElement(mxfLength& length,
                         bool limitBytes,
                         mxfUInt32 limit,
                         mxfFile infile)
{
  mxfLength remainder = length;
  while (remainder > 0) {
    mxfUInt16 numberOfANCPackets;
    readMxfUInt16(numberOfANCPackets, infile);
    remainder = remainder - 2;
    fprintf(stdout, "    Number of ANC Packets = ");
    printHexFieldPad(stdout, numberOfANCPackets);
    fprintf(stdout, "\n");

    for (mxfUInt16 i = 0; i < numberOfANCPackets; i++) {

      fprintf(stdout, "    [");
      printDecField(stdout, i);
      fprintf(stdout, " : ANC Packet]\n");

      mxfUInt16 lineNumber;
      readMxfUInt16(lineNumber, infile);
      remainder = remainder - 2;
      printMxfUInt16(stdout, "Line Number", lineNumber);

      mxfUInt08 wrappingType;
      readMxfUInt08(wrappingType, infile);
      remainder = remainder - 1;
      printMxfUInt08(stdout, "Wrapping Type", wrappingType);

      mxfUInt08 payloadSampleCoding;
      readMxfUInt08(payloadSampleCoding, infile);
      remainder = remainder - 1;
      printMxfUInt08(stdout, "Sample Coding", payloadSampleCoding);

      mxfUInt16 payloadSampleCount;
      readMxfUInt16(payloadSampleCount, infile);
      remainder = remainder - 2;
      printMxfUInt16(stdout, "Sample Count", payloadSampleCount);

      mxfUInt32 ancPayloadEntryCount;
      mxfUInt32 ancPayloadEntrySize;
      readMxfUInt32(ancPayloadEntryCount, infile);
      readMxfUInt32(ancPayloadEntrySize, infile);
      remainder = remainder - 8;
      printANCPayloadByteArray(ancPayloadEntryCount,
                               ancPayloadEntrySize,
                               payloadSampleCoding,
                               limitBytes,
                               limit,
                               infile);
      remainder = remainder - (ancPayloadEntryCount * ancPayloadEntrySize);
    }
  }
}

void printEssenceKL(mxfKey& k, mxfLength& len);

void printEssenceKL(mxfKey& k, mxfLength& /* len */)
{
  mxfByte itemTypeId = k.octet12;
  mxfByte elementTypeId = k.octet14;

  const char* itemTypeIdName = itemTypeName(itemTypeId);
  const char* elementTypeIdName;
  if (isAvidEssenceElement(k)) {
    elementTypeIdName = avidElementTypeName(itemTypeId, elementTypeId);
  } else {
    elementTypeIdName = elementTypeName(itemTypeId, elementTypeId);
  }

  int elementCount = k.octet13;
  int elementNumber = k.octet15;


  fprintf(stdout,
          "  Track          = %02x.%02x.%02x.%02x\n",
          k.octet12,
          k.octet13,
          k.octet14,
          k.octet15);
  fprintf(stdout,
          "  Item Type      = \"%s\" (%02x)\n",
          itemTypeIdName,
          itemTypeId);
  fprintf(stdout,
          "  Element Type   = \"%s\" (%02x)\n",
          elementTypeIdName,
          elementTypeId);
  fprintf(stdout,
          "  Element Count  = %d (%02x)\n",
          elementCount,
          elementCount);
  fprintf(stdout,
          "  Element Number = %d (%02x)\n",
          elementNumber,
          elementNumber);
}

void printEssenceFrameFill(mxfKey& k,
                           mxfLength& length,
                           mxfUInt32 frameCount,
                           mxfFile infile);

void printEssenceFrameFill(mxfKey& k,
                           mxfLength& length,
                           mxfUInt32 frameCount,
                           mxfFile infile)
{
  if ((frameCount < maxFrames) || !iFlag) {
    printFill(k, length, infile);
  } else {
    skipV(length, infile);
  }
}

void printEssenceFrameValue(mxfKey& k,
                            mxfLength& length,
                            mxfUInt08& llen,
                            mxfUInt32 frameCount,
                            bool limitBytes,
                            mxfUInt32 limit,
                            mxfFile infile);

void printEssenceFrameValue(mxfKey& k,
                            mxfLength& length,
                            mxfUInt08& llen,
                            mxfUInt32 frameCount,
                            bool limitBytes,
                            mxfUInt32 limit,
                            mxfFile infile)
{
  if ((frameCount < maxFrames) || !iFlag) {
    fprintf(stdout, "\n");
    fprintf(stdout, "[ Frame = ");
    printDecField(stdout, frameCount);
    fprintf(stdout, "]");

    printKL(k, length, llen);
    printV(length, limitBytes, limit, infile);
  } else {
    skipV(length, infile);
  }
}

void printEssenceFrames(mxfKey& k,
                        mxfLength& length,
                        bool limitBytes,
                        mxfUInt32 limit,
                        mxfFile infile);

void printEssenceFrames(mxfKey& k,
                        mxfLength& length,
                        bool limitBytes,
                        mxfUInt32 limit,
                        mxfFile infile)
{
  mxfUInt32 frameCount = 0;
  mxfLength total = 0;

  printEssenceKL(k, length);
  while (total < length) {
    mxfKey k;
    readMxfKey(k, infile);
    mxfLength len;
    mxfUInt08 llen;
    int lengthLen = readMxfLength(len, llen, infile);
    total = total + lengthLen;

    fprintf(stdout, "[ llen = %d ]", llen);
    
    if (isFill(k)) {
      printEssenceFrameFill(k, len, frameCount, infile);
    } else {
      printEssenceFrameValue(k, len, llen, frameCount, limitBytes, limit, infile);
      frameCount = frameCount + 1;
    }
    total = total + len;
  }
  if (iFlag && (maxFrames < frameCount)) {
    fprintf(stdout, "[ Essence frames truncated from ");
    printDecField(stdout, frameCount);
    fprintf(stdout, " frames to ");
    printDecField(stdout, maxFrames);
    fprintf(stdout, " frames ]\n");
  }
}

void printEssence(mxfKey& k,
                  mxfLength& length,
                  bool limitBytes,
                  mxfUInt32 limit,
                  mxfFile infile);

void printEssence(mxfKey& k,
                  mxfLength& length,
                  bool limitBytes,
                  mxfUInt32 limit,
                  mxfFile infile)
{
  printEssenceKL(k, length);
  if (dumpANC && isANCFrameElement(k)) { // move to printEssenceElement?
    printANCFrameElement(length, limitBytes, limit, infile);
  } else {
    printV(length, limitBytes, limit, infile);
  }
}

void printEssenceElement(mxfKey& k,
                         mxfLength& length,
                         bool limitBytes,
                         mxfUInt32 limit,
                         mxfFile infile);

void printEssenceElement(mxfKey& k,
                         mxfLength& length,
                         bool limitBytes,
                         mxfUInt32 limit,
                         mxfFile infile)
{
  if (frames) {
    printEssenceFrames(k, length, limitBytes, limit, infile);
  } else {
    printEssence(k, length, limitBytes, limit, infile);
  }
}

void skipV(mxfLength length, mxfFile infile)
{
  // Seek past (length - 1) bytes then read a byte
  if (length != 0) {
    skipBytes(length - 1, infile);
    mxfUInt08 x;
    readMxfUInt08(x, infile);
  }
}

void printLocalKL(mxfLocalKey& k, mxfUInt16& l, mxfKey& enclosing);

void printLocalKL(mxfLocalKey& k, mxfUInt16& l, mxfKey& enclosing)
{
  fprintf(stdout, "  [ k = ");
  printMxfLocalKeySymbol(k, enclosing, stdout);
  fprintf(stdout,
          ", l = %5d (%04x) ]\n",
          l,
          l);
}

void printLocalV(mxfUInt16& length,
                 mxfLength& remainder,
                 mxfFile infile);

void checkLocalKey(mxfLocalKey& k);

void checkLocalKey(mxfLocalKey& k)
{
  if (k == nullMxfLocalKey) {
    mxfError(currentKey, keyPosition, "Null local key");
  }
}

mxfUInt64 checkLength(mxfUInt64 length,
                      mxfUInt64 fileSize,
                      mxfUInt64 position);

mxfUInt64 checkLength(mxfUInt64 length,
                      mxfUInt64 fileSize,
                      mxfUInt64 position)
{
  mxfUInt64 result;
  mxfUInt64 remainder = fileSize - position;
  if (length > remainder) {
    mxfError(currentKey, keyPosition, "Length points beyond end of file");
    result = remainder;
  } else {
    result = length;
  }
  return result;
}

void checkKey(mxfKey& k)
{
  if (isNullKey(k)) {
    mxfError("Null key at offset 0x%" MXFPRIx64 ".\n",
             keyPosition);
  }
}

bool isDark(mxfKey& k, Mode mode)
{
  char* flag;
  bool result = false;
  size_t x;
  switch (mode) {
  case klvMode:
    result = false;
    break;
  case localSetMode:
    result = false;
    break;
  case mxfMode:
    result = !lookupMXFKey(k, x);
    break;
  case aafMode: {
      bool found = findAAFKey(k, x, &flag);
      if (!found) {
        found = lookupMXFKey(k, x);
      }
      result = !found;
    }
    break;
  default:
    result = false;
    break;
  }
  return result;
}

bool dumpFill = false;

bool isFill(mxfKey& k)
{
  bool result;
  if (memcmp(&KLVFill, &k, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&V1KLVFill, &k, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&BogusFill, &k, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

void printFill(mxfKey& k, mxfLength& len, mxfFile infile)
{
  checkFill(k, keyPosition, previousKey);
  if (dumpFill) {
    printV(len, false, 0, infile);
  } else {
    skipV(len, infile);
  }
}

bool isNullKey(const mxfKey& k)
{
  bool result;
  if (memcmp(&nullMxfKey, &k, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isLocalSet(mxfKey& k);

bool isLocalSet(mxfKey& k)
{
  char* flag;
  bool result;
  if (k.octet5 == 0x53) {
    result = true;
  } else {
    if (aafKeysAsSets) {
      size_t index;
      bool found = findAAFKey(k, index, &flag);
      if (found) {
        result = true;
      } else {
        result = false;
      }
    } else {
      result = false;
    }
  }
  return result;
}

void skipBogusBytes(const mxfUInt64 byteCount, mxfFile infile);

void skipBogusBytes(const mxfUInt64 byteCount, mxfFile infile)
{
  mxfUInt64 length = byteCount;
  fprintf(stdout, "[ * Error * %" MXFPRIu64 " superfluous bytes ]\n", length);
  printV(length, false, 0, infile);
}

void validateLocalKey(mxfLocalKey& identifier,
                      mxfLength& remainder,
                      mxfFile infile);

void validateLocalKey(mxfLocalKey& identifier,
                      mxfLength& remainder,
                      mxfFile infile)
{
  if (remainder > 2) {
    readMxfLocalKey(identifier, infile);
    checkLocalKey(identifier);
    remainder = remainder - 2;
  } else {
    mxfError(currentKey,
             keyPosition,
             "Local set KLV parse error -"
             " set exhausted looking for local key");
    remainder = 0;
  }
}

void validateLocalLength(mxfUInt16& length,
                         mxfLength& remainder,
                         mxfFile infile);

void validateLocalLength(mxfUInt16& length,
                         mxfLength& remainder,
                         mxfFile infile)
{
  if (remainder > 2) {
    readMxfUInt16(length, infile);
    remainder = remainder - 2;
  } else {
    mxfError(currentKey,
             keyPosition,
             "Local set KLV parse error -"
             " set exhausted looking for local length");
    remainder = 0;
  }
}

mxfUInt16 validateLocalV(mxfUInt16& length,
                         mxfLength& remainder,
                         mxfFile infile);

mxfUInt16 validateLocalV(mxfUInt16& length,
                         mxfLength& remainder,
                         mxfFile /* infile */)
{
  mxfUInt16 vLength;
  if (length < remainder) {
    vLength = length;
  } else {
    vLength = static_cast<mxfUInt16>(remainder);
  }
  remainder = remainder - vLength;
  if (vLength < length) {
    mxfError(currentKey,
             keyPosition,
             "Local set KLV parse error - length points beyond end of set");
  }
  return vLength;
}

void printLocalV(mxfUInt16& length,
                 mxfLength& remainder,
                 mxfFile infile)
{
  mxfLength vLength = validateLocalV(length, remainder, infile);
  printV(vLength, false, 0, infile);
}

void printLocalSet(mxfKey& k, mxfLength& len, mxfFile infile);

void printLocalSet(mxfKey& k, mxfLength& len, mxfFile infile)
{
  mxfLength remainder = len;
  while (remainder > 0) {
    // Key (local identifier)
    mxfLocalKey identifier;
    mxfLength excess = remainder;
    validateLocalKey(identifier, remainder, infile);
    if (remainder == 0) {
      skipBogusBytes(excess, infile);
      break;
    }

    // Length
    mxfUInt16 length;
    excess = remainder;
    validateLocalLength(length, remainder, infile);
    if (remainder == 0) {
      skipBogusBytes(excess, infile);
      break;
    }

    // Value
    printLocalKL(identifier, length, k);
    printLocalV(length, remainder, infile);
  }
}

void checkElementSize(mxfUInt32 expectedSize,
                      mxfUInt32 actualSize,
                      mxfUInt32 elementCount);

void checkElementSize(mxfUInt32 expectedSize,
                      mxfUInt32 actualSize,
                      mxfUInt32 elementCount)
{
  if (actualSize != expectedSize) {
    if ((elementCount == 0) && (actualSize == 0)) {
      mxfWarning(currentKey,
                 keyPosition,
                 "Incorrect element size"
                 " - expected %" MXFPRIu32 ", found %" MXFPRIu32 "",
                 expectedSize,
                 actualSize);
    } else {
      mxfError(currentKey,
               keyPosition,
               "Incorrect element size"
               " - expected %" MXFPRIu32 ", found %" MXFPRIu32 "",
               expectedSize,
               actualSize);
    }
  }
}

mxfUInt32 checkElementCount(mxfUInt32 elementCount,
                            mxfUInt32 elementSize,
                            mxfUInt64 length,
                            mxfUInt64 fixedSize);

mxfUInt32 checkElementCount(mxfUInt32 elementCount,
                            mxfUInt32 elementSize,
                            mxfUInt64 length,
                            mxfUInt64 fixedSize)
{
  mxfUInt32 result = elementCount;
  if (length >= fixedSize) {
    mxfUInt64 remaining = length - fixedSize;
    mxfUInt64 elementBytes = elementSize * elementCount;
    if (remaining < elementBytes) {
      mxfError(currentKey,
               keyPosition,
               "Element count too big for remaining bytes"
               " - %" MXFPRIu32 " elements require %" MXFPRIu64 " bytes,"
               " %" MXFPRIu64 " bytes remaining",
               elementCount,
               elementBytes,
               remaining);
      result = static_cast<mxfUInt32>(remaining / elementSize);
    } else if (remaining > elementBytes) {
      mxfError(currentKey,
               keyPosition,
               "Element count too small for remaining bytes"
               " - %" MXFPRIu32 " elements require %" MXFPRIu64 " bytes,"
               " %" MXFPRIu64 " bytes remaining",
               elementCount,
               elementBytes,
               remaining);
      result = static_cast<mxfUInt32>(remaining / elementSize);
    } // else correct
  } // else reported elsewhere
  return result;
}

struct Segment {
  mxfUInt64 _start;
  mxfUInt64 _size; // allocated
  mxfUInt64 _free; // free within allocated
  mxfUInt64 _origin;
  mxfKey _label;
};

typedef std::list<Segment*> SegmentList;

// In-memory representaton of a partition
struct mxfPartition {
  // As read from the file
  mxfKey _key;
  mxfUInt16 _majorVersion;
  mxfUInt16 _minorVersion;
  mxfUInt32 _KAGSize;
  mxfUInt64 _thisPartition;
  mxfUInt64 _previousPartition;
  mxfUInt64 _footerPartition;
  mxfUInt64 _headerByteCount;
  mxfUInt64 _indexByteCount;
  mxfUInt32 _indexSID;
  mxfUInt64 _bodyOffset;
  mxfUInt32 _bodySID;
  mxfKey _operationalPattern;
  mxfUInt32 _elementCount;
  mxfUInt32 _elementSize;
  // As computed
  mxfUInt64 _address; // Address of partition relative to header
  mxfUInt64 _length;
  mxfUInt64 _metadataSize;
  mxfUInt64 _indexSize;
  SegmentList _segments; // in this partition
};

typedef std::list<mxfPartition*> PartitionList;

mxfPartition* currentPartition = 0;
mxfUInt64 headerPosition = 0;

void markMetadataStart(mxfUInt64 primerKeyPosition)
{
  primerPosition = primerKeyPosition;
}

void markMetadataEnd(mxfUInt64 endKeyPosition)
{
  if (primerPosition != 0) {
    mxfUInt64 headerByteCount = endKeyPosition - primerPosition;
    currentPartition->_metadataSize = headerByteCount;
    primerPosition = 0;
  }
}

void markIndexStart(mxfUInt32 sid, mxfUInt64 indexKeyPosition)
{
  if (!inIndex) {
    inIndex = true;
    indexSID = sid;
    memcpy(&indexLabel, &currentKey, sizeof(mxfKey));
    indexPosition = indexKeyPosition;
  }
}

void markIndexEnd(mxfUInt64 endKeyPosition)
{
  if (inIndex) {
    mxfUInt64 free = 0;
    if (endKeyPosition == fillEnd) {
      free = fillEnd - fillStart;
    }
    mxfUInt64 indexByteCount = endKeyPosition - indexPosition;
    currentPartition->_indexSize = indexByteCount;
    newIndexSegment(indexSID,
                    indexLabel,
                    indexPosition,
                    endKeyPosition - indexPosition,
                    free);
    indexPosition = 0;
    inIndex = false;
    indexSID = 0;
  }
}

void markEssenceSegmentStart(mxfUInt32 sid, mxfUInt64 essenceKeyPosition)
{
  if (!inEssence) {
    inEssence = true;
    essenceSID = sid;
    memcpy(&essenceLabel, &currentKey, sizeof(mxfKey));
    essencePosition = essenceKeyPosition;
  }
}

void markEssenceSegmentEnd(mxfUInt64 endKeyPosition)
{
  if (inEssence) {
    mxfUInt64 free = 0;
    if (endKeyPosition == fillEnd) {
      free = fillEnd - fillStart;
    }
    newEssenceSegment(essenceSID,
                      essenceLabel,
                      essencePosition,
                      endKeyPosition - essencePosition,
                      free);
    inEssence = false;
    essenceSID = 0;
  }
}

void markFill(mxfUInt64 fillKeyPosition,
              mxfUInt64 fillEndPosition)
{
  fillStart = fillKeyPosition;
  fillEnd = fillEndPosition;
}

struct Stream {
  SegmentList _segments;
  mxfUInt64 _size; // allocated
  mxfUInt64 _used; // in use
  mxfUInt32 _sid;
  bool _isEssence; // _sid is body sid
};

typedef std::map<mxfUInt32, Stream*> StreamSet;
StreamSet streams;

void printSegment(Segment* seg);

void printSegment(Segment* seg)
{
  fprintf(stdout,
          "      %016" MXFPRIx64 " : %016" MXFPRIx64 "",
          seg->_origin,
          seg->_origin + seg->_size);
  fprintf(stdout,
          " [%016" MXFPRIx64 " : %016" MXFPRIx64 "]\n",
          seg->_start,
          seg->_start + seg->_size);
}

void printSegments(SegmentList& segments);

void printSegments(SegmentList& segments)
{
  if (!segments.empty()) {
    fprintf(stdout, "    Segments\n");
    fprintf(stdout, "      File address range");
    fprintf(stdout, "                  [Stream address range]\n");
    SegmentList::const_iterator it;
    for (it = segments.begin(); it != segments.end(); it++) {
      Segment* seg = *it;
      printSegment(seg);
    }
  }
}

void printStream(Stream* s);

void printStream(Stream* s)
{
  fprintf(stdout, "  SID = %08" MXFPRIu32 ",", s->_sid);

  fprintf(stdout, " Size = %016" MXFPRIu64 ",", s->_size);

  fprintf(stdout, " Used = %016" MXFPRIu64 ",", s->_used);

  if (s->_isEssence) {
    fprintf(stdout, " [ essence ]\n");
  } else {
    fprintf(stdout, " [ index ]\n");
  }
  printSegments(s->_segments);
}

void printStreams(StreamSet& streams);

void printStreams(StreamSet& streams)
{
  fprintf(stdout, "Streams\n");
  StreamSet::const_iterator it;
  for (it = streams.begin(); it != streams.end(); it++) {
    printStream(it->second);
  }
}

void destroyStream(Stream* s);

void destroyStream(Stream* s)
{
  SegmentList::const_iterator it;
  for (it = s->_segments.begin(); it != s->_segments.end(); it++) {
    Segment* seg = *it;
    delete seg;
  }
}

void destroyStreams(StreamSet& streams);

void destroyStreams(StreamSet& streams)
{
  StreamSet::const_iterator it;
  for (it = streams.begin(); it != streams.end(); it++) {
    destroyStream(it->second);
    delete it->second;
  }
}

void newSegment(bool isEssence,
                mxfUInt32 sid,
                mxfKey& label,
                mxfUInt64 start,
                mxfUInt64 size,
                mxfUInt64 free)
{
  Stream* s;
  StreamSet::const_iterator it = streams.find(sid);
  if (it == streams.end()) {
    s = new Stream();
    s->_size = 0;
    s->_used = 0;
    s->_sid = sid;
    s->_isEssence = isEssence;

    streams.insert(StreamSet::value_type(sid, s));
  } else {
    s = it->second;
    if (isEssence && !s->_isEssence) {
      mxfWarning(currentPartition->_key,
                 currentPartition->_address,
                 "Essence has the same SID (%" MXFPRIu32 ") as index"
                 " (possibly in another partition),",
                 sid);
    } else if (!isEssence && s->_isEssence) {
      mxfWarning(currentPartition->_key,
                 currentPartition->_address,
                 "Index has the same SID (%" MXFPRIu32 ") as essence"
                 " (possibly in another partition),",
                 sid);
    }
  }
  Segment* seg = new Segment;
  seg->_start = s->_size;
  seg->_size = size;
  seg->_free = free;
  seg->_origin = start;
  memcpy(&seg->_label, &label, sizeof(mxfKey));
  s->_segments.push_back(seg);
  s->_size = s->_size + seg->_size;
  s->_used = s->_used + (seg->_size - seg->_free);

  currentPartition->_segments.push_back(seg);
}

void newEssenceSegment(mxfUInt32 sid,
                       mxfKey& label,
                       mxfUInt64 start,
                       mxfUInt64 size,
                       mxfUInt64 free)
{
  newSegment(true, sid, label, start, size, free);
}

void newIndexSegment(mxfUInt32 sid,
                     mxfKey& label,
                     mxfUInt64 start,
                     mxfUInt64 size,
                     mxfUInt64 free)
{
  newSegment(false, sid, label, start, size, free);
}

void readPartition(PartitionList& partitions,
                   mxfUInt64 length,
                   mxfFile infile);

void readPartition(PartitionList& partitions,
                   mxfUInt64 length,
                   mxfFile infile)
{
  mxfPartition* p = new mxfPartition;

  memcpy(&p->_key, &currentKey, sizeof(mxfKey));
  p->_address = keyPosition - headerPosition;
  p->_length = length;
  p->_metadataSize = 0;
  p->_indexSize = 0;

  readMxfUInt16(p->_majorVersion, infile);
  readMxfUInt16(p->_minorVersion, infile);
  readMxfUInt32(p->_KAGSize, infile);
  readMxfUInt64(p->_thisPartition, infile);
  readMxfUInt64(p->_previousPartition, infile);
  readMxfUInt64(p->_footerPartition, infile);
  readMxfUInt64(p->_headerByteCount, infile);
  readMxfUInt64(p->_indexByteCount, infile);
  readMxfUInt32(p->_indexSID, infile);
  readMxfUInt64(p->_bodyOffset, infile);
  readMxfUInt32(p->_bodySID, infile);
  readMxfLabel(p->_operationalPattern, infile);

  readMxfUInt32(p->_elementCount, infile);
  readMxfUInt32(p->_elementSize, infile);

  mxfUInt64 remaining = length - partitionFixedSize;
  skipBytes(remaining, infile);

  partitions.push_back(p);
  currentPartition = p;
}

void printPartitions(PartitionList& partitions);

void printPartitions(PartitionList& partitions)
{
  fprintf(stdout, "Partitions\n");
  PartitionList::const_iterator it;
  for (it = partitions.begin(); it != partitions.end(); ++it) {
    mxfPartition* p = *it;
    fprintf(stdout,
            "  Address = %016" MXFPRIx64 " [%s]\n",
            p->_address,
            mxfKeyName(p->_key));
    printSegments(p->_segments);
  }
}

Segment* findEssenceSegment(mxfPartition* p);

Segment* findEssenceSegment(mxfPartition* p)
{
  Segment* result = 0;
  if (!p->_segments.empty()) {
    SegmentList::const_iterator it;
    for (it = p->_segments.begin(); it != p->_segments.end(); it++) {
      Segment* seg = *it;
      if (isEssenceElement(seg->_label) || isSystemElement(seg->_label)) {
        result = seg;
        break;
      }
    }
  }
  return result;
}

bool hasEssence(mxfPartition* p);

bool hasEssence(mxfPartition* p)
{
  bool result = false;
  if (findEssenceSegment(p)) {
    result = true;
  }
  return result;
}

Segment* findIndexSegment(mxfPartition* p);

Segment* findIndexSegment(mxfPartition* p)
{
  Segment* result = 0;
  if (!p->_segments.empty()) {
    SegmentList::const_iterator it;
    for (it = p->_segments.begin(); it != p->_segments.end(); it++) {
      Segment* seg = *it;
      if (isIndexSegment(seg->_label)) {
        result = seg;
        break;
      }
    }
  }
  return result;
}

bool hasIndex(mxfPartition* p);

bool hasIndex(mxfPartition* p)
{
  bool result = false;
  if (findIndexSegment(p)) {
    result = true;
  }
  return result;
}

void checkField(mxfUInt64 expected,
                mxfUInt64 actual,
                const mxfKey& key,
                mxfUInt64 keyAddress,
                const char* label);

void checkField(mxfUInt64 expected,
                mxfUInt64 actual,
                const mxfKey& key,
                mxfUInt64 keyAddress,
                const char* label)
{
  if (actual != expected) {
    if (actual == 0) {
      mxfWarning(key,
                 keyAddress,
                 "%s not defined"
                 " - expected 0x%" MXFPRIx64 ",",
                 label,
                 expected);
    } else {
      mxfError(key,
               keyAddress,
               "Incorrect value for %s"
               " - expected 0x%" MXFPRIx64 ", found 0x%" MXFPRIx64 "",
               label,
               expected,
               actual);
    }
  }
}

void checkSID(Segment* seg,
              mxfUInt32 actual,
              const mxfKey& key,
              mxfUInt64 keyAddress,
              const char* label,
              const char* kind);

void checkSID(Segment* seg,
              mxfUInt32 actual,
              const mxfKey& key,
              mxfUInt64 keyAddress,
              const char* label,
              const char* kind)
{
  if (seg != 0) {
    if (actual == 0) {
      mxfError(key,
               keyAddress,
               "Incorrect value for %s"
               " - expected %s, found 0x%" MXFPRIx32 ""
               " - partition contains %" MXFPRIu64 " bytes of %s,",
               label,
               "non-zero",
               actual,
               seg->_size,
               kind);
    }
  } else {
    if (actual != 0) {
      mxfError(key,
               keyAddress,
               "Incorrect value for %s"
               " - expected 0x%" MXFPRIx32 ", found 0x%" MXFPRIx32 ""
               " - partition does not contain %s,",
               label,
               0,
               actual,
               kind);
    }
  }
}

void checkOperationalPattern(mxfPartition* p);

void checkOperationalPattern(mxfPartition* p)
{
  if (!isOperationalPattern(p->_operationalPattern)) {
    mxfError(p->_key,
             p->_address + headerPosition,
             "Invalid operational pattern");
  }
}

void checkFill(const mxfKey& key,
               mxfUInt64 keyPosition,
               const mxfKey& previousKey)
{
  if (memcmp(&KLVFill, &previousKey, sizeof(mxfKey)) == 0) {
    mxfError(key, keyPosition, "Consecutive fill items");
  }
}

void checkEssenceContainers(mxfPartition* p);

void checkEssenceContainers(mxfPartition* p)
{
  checkElementSize(sizeof(mxfKey), p->_elementSize, p->_elementCount);
  checkElementCount(p->_elementCount,
                    sizeof(mxfKey),
                    p->_length,
                    partitionFixedSize);
  if (hasEssence(p)) {
    if (p->_elementCount == 0) {
      mxfError(p->_key,
               p->_address,
               "Partition contains essence "
               "but there are no essence container labels.");
    }
  }
}

void checkPartition(mxfPartition* p, mxfUInt64 previous, mxfPartition* footer);

void checkPartition(mxfPartition* p, mxfUInt64 previous, mxfPartition* footer)
{
  // Major Version
  checkField(1,
             p->_majorVersion,
             p->_key,
             p->_address + headerPosition,
             "MajorVersion");
  // Minor Version
  checkField(2,
             p->_minorVersion,
             p->_key,
             p->_address + headerPosition,
             "MinorVersion");
  // KAGSize
  // ThisPartition
  checkField(p->_address,
             p->_thisPartition,
             p->_key,
             p->_address + headerPosition,
             "ThisPartition");
  // PreviousPartition
  checkField(previous,
             p->_previousPartition,
             p->_key,
             p->_address + headerPosition,
             "PreviousPartition");
  // FooterPartition
  if (footer != 0) {
    checkField(footer->_address,
               p->_footerPartition,
               p->_key,
               p->_address + headerPosition,
               "FooterPartition");
  }
  // HeaderByteCount
  checkField(p->_metadataSize,
             p->_headerByteCount,
             p->_key,
             p->_address,
             "HeaderByteCount");
  // IndexByteCount
  checkField(p->_indexSize,
             p->_indexByteCount,
             p->_key,
             p->_address,
             "IndexByteCount");
  // IndexSID
  Segment* iseg = findIndexSegment(p);
  checkSID(iseg, p->_indexSID, p->_key, p->_address, "IndexSID", "index");

  // BodyOffset
  mxfUInt64 bodyOffset = 0;
  Segment* seg = findEssenceSegment(p);
  if (seg != 0) {
    bodyOffset = seg->_start;
  }
  checkField(bodyOffset,
             p->_bodyOffset,
             p->_key,
             p->_address,
             "BodyOffset");
  // BodySID
  checkSID(seg, p->_bodySID, p->_key, p->_address, "BodySID", "essence");

  // Operational Pattern
  checkOperationalPattern(p);
  // EssenceContainers
  checkEssenceContainers(p);

  if ((!isClosed(p->_key)) && (!isComplete(p->_key))) {
    mxfWarning(p->_key,
               p->_address,
               "Partition is open and incomplete");
  } else if (!isClosed(p->_key)) {
    mxfWarning(p->_key,
               p->_address,
               "Partition is open");
  } else if (!isComplete(p->_key)) {
    mxfWarning(p->_key,
               p->_address,
               "Partition is incomplete");
  }
}

void checkPartitions(PartitionList& partitions, mxfPartition* footer);

void checkPartitions(PartitionList& partitions, mxfPartition* footer)
{
  mxfUInt64 previous = 0;
  PartitionList::const_iterator it;
  for (it = partitions.begin(); it != partitions.end(); ++it) {
    mxfPartition* p = *it;
    checkPartition(p, previous, footer);
    previous = p->_address;
  }
}

void destroyPartitions(PartitionList& partitions);

void destroyPartitions(PartitionList& partitions)
{
  PartitionList::const_iterator it;
  for (it = partitions.begin(); it != partitions.end(); ++it) {
    mxfPartition* p = *it;
    delete p;
  }
}

// Note on partition keys -
//
// We choose to name the partition packs according to -
//
//  [Open][Incomplete]{Header|Body|Footer}
//
// That is, a partition is assumed to be Closed and Complete unless
// explicitlt stated.
//
//                              0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
// OpenIncompleteHeader        06 0e 2b 34 02 05 01 01 0d 01 02 01 01 02 01 00
// IncompleteHeader            06 0e 2b 34 02 05 01 01 0d 01 02 01 01 02 02 00
// OpenHeader                  06 0e 2b 34 02 05 01 01 0d 01 02 01 01 02 03 00
// Header                      06 0e 2b 34 02 05 01 01 0d 01 02 01 01 02 04 00
//
// OpenIncompleteBody          06 0e 2b 34 02 05 01 01 0d 01 02 01 01 03 01 00
// IncompleteBody              06 0e 2b 34 02 05 01 01 0d 01 02 01 01 03 02 00
// OpenBody                    06 0e 2b 34 02 05 01 01 0d 01 02 01 01 03 03 00
// Body                        06 0e 2b 34 02 05 01 01 0d 01 02 01 01 03 04 00
//
// GenericStream               06 0e 2b 34 02 05 01 01 0d 01 02 01 01 03 11 00
//
// IncompleteFooter            06 0e 2b 34 02 05 01 01 0d 01 02 01 01 04 02 00
// Footer                      06 0e 2b 34 02 05 01 01 0d 01 02 01 01 04 04 00
//
//
// 1) The Footer may not be Open, so the following are illegal -
//
//    OpenFooter           = illegal
//    OpenIncompleteFooter = illegal
//
// 2) The values are coded such that there aren't separate bits that indicate
//    Open/Closed and Complete/Incomplete
//

bool isPartition(mxfKey& key)
{
  bool result;
  if (memcmp(&OpenIncompleteHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&IncompleteHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&OpenHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Header, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&OpenIncompleteBody, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&IncompleteBody, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&OpenBody, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Body, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&GenericStream, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&IncompleteFooter, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Footer, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isHeader(mxfKey& key)
{
  bool result;
  if (memcmp(&OpenIncompleteHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&IncompleteHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&OpenHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Header, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isFooter(mxfKey& key)
{
  bool result;
  if (memcmp(&IncompleteFooter, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Footer, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isClosed(const mxfKey& key)
{
  bool result;
  if (memcmp(&IncompleteHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Header, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&IncompleteBody, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Body, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&GenericStream, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&IncompleteFooter, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Footer, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isComplete(const mxfKey& key)
{
  bool result;
  if (memcmp(&OpenHeader, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Header, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&OpenBody, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Body, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&Footer, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

bool isRandomIndex(const mxfKey& key)
{
  bool result;
  if (memcmp(&RandomIndexMetadata, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else if (memcmp(&V10RandomIndexMetadata, &key, sizeof(mxfKey)) == 0) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

const char* keyName(const mxfKey& key)
{
  const char* result = "Dark";
  size_t x;
  bool found;

  switch (mode) {
  case klvMode:
  case localSetMode:
  case mxfMode:
  case klvValidateMode:
  case setValidateMode:
  case mxfValidateMode:
    found = lookupMXFKey(const_cast<mxfKey&>(key), x);
    if (found) {
      result = mxfKeyTable[x]._name;
    } else if (isEssenceElement(const_cast<mxfKey&>(key))) {
      result = "Essence Element";
    } else if (isNullKey(key)) {
      result = "Null";
    } else {
      result = "Dark";
    }
    break;
  case aafMode:
  case aafValidateMode:
    found = lookupMXFKey(const_cast<mxfKey&>(key), x);
    if (found) {
      result = mxfKeyTable[x]._name;
    } else {
      found = lookupAAFKey(const_cast<mxfKey&>(key), x);
      if (found) {
        result = aafKeyTable[x]._name;
      } else if (isEssenceElement(const_cast<mxfKey&>(key))) {
        result = "Essence Element";
      } else if (isNullKey(key)) {
        result = "Null";
      } else {
        result = "Dark";
      }
    }
    break;
  default:
    /* Invalid mode ? */
    break;
  }
  return result;
}

struct mxfRIPEntry {
  mxfUInt32 _sid;
  mxfUInt64 _offset;
};

typedef std::list<mxfRIPEntry> RandomIndex;

struct mxfRandomIndex {
  mxfKey _key;
  RandomIndex _index;
};

void readRandomIndex(mxfRandomIndex& rip,
                     const mxfKey& key,
                     mxfLength length,
                     mxfUInt32& overallLength,
                     mxfFile infile);

void readRandomIndex(mxfRandomIndex& rip,
                     const mxfKey& key,
                     mxfLength length,
                     mxfUInt32& overallLength,
                     mxfFile infile)
{
  rip._key = key;
  mxfUInt64 entryCount = length / (sizeof(mxfUInt32) + sizeof(mxfUInt64));
  for (mxfUInt32 i = 0; i < entryCount; i++) {
    mxfRIPEntry e;
    readMxfUInt32(e._sid, infile);
    readMxfUInt64(e._offset, infile);
    rip._index.push_back(e);
  }
  readMxfUInt32(overallLength, infile);
}

void printRandomIndex(mxfRandomIndex& rip);

void printRandomIndex(mxfRandomIndex& rip)
{
  fprintf(stdout, "Random index\n");
  fprintf(stdout, "  SID      : Address\n");
  RandomIndex::const_iterator it;
  for (it = rip._index.begin(); it != rip._index.end(); ++it) {
    mxfRIPEntry e = *it;
    fprintf(stdout, "  %08" MXFPRIx32 " : %016" MXFPRIx64 "\n", e._sid, e._offset);
  }
}

void checkRandomIndex(mxfUInt64 keyPosition,
                      mxfUInt64 endPosition,
                      mxfUInt64 length,
                      mxfUInt32 overallLength,
                      mxfUInt64 fileSize);

void checkRandomIndex(mxfUInt64 keyPosition,
                      mxfUInt64 endPosition,
                      mxfUInt64 length,
                      mxfUInt32 overallLength,
                      mxfUInt64 fileSize)
{
  // length must be (n * (8 + 4)) + 4
  //
  mxfUInt32 entrySize = sizeof(mxfUInt32) + sizeof(mxfUInt64);
  mxfUInt64 entries = length / entrySize;
  if (((entries * entrySize) + sizeof(mxfUInt32)) != length) {
    mxfError(currentKey,
             keyPosition,
             "Incorrect value for random index length"
             " - expected (N * 12) + 4, found %" MXFPRIu64 "",
             length);
  }

  // overall length must be (endPosition - keyPosition)
  //
  if ((endPosition - keyPosition) != overallLength) {
    mxfError(currentKey,
             keyPosition,
             "Incorrect value for random index overall length"
             " - expected %" MXFPRIu64 ", found %" MXFPRIu32 "",
             (endPosition - keyPosition),
             overallLength);
  }

  // Nothing should follow the random index
  if (endPosition < fileSize) {
    mxfError(currentKey,
             keyPosition,
             "Found %" MXFPRIu64 " excess bytes following random index",
             (fileSize - endPosition));
  }
}

void checkRandomIndex(mxfRandomIndex& rip, PartitionList& partitions);

void checkRandomIndex(mxfRandomIndex& rip, PartitionList& partitions)
{
  // Check that we have the correct number of partitions.
  //
  size_t expectedPartitions = partitions.size();
  size_t actualPartitions = rip._index.size();
  if (actualPartitions != expectedPartitions) {
    mxfError("Invalid random index - incorrect partition count"
             " (partitions in file = %" MXFPRIu32 ","
             " partitions in random index = %" MXFPRIu32 ").\n",
             expectedPartitions,
             actualPartitions);
  }

  // Check that the partitions in the random index are in the correct order.
  //
  RandomIndex::const_iterator rit;
  mxfUInt64 previous = 0;
  for (rit = rip._index.begin(); rit != rip._index.end(); ++rit) {
    if ((rit != rip._index.begin()) && (rit->_offset <= previous)) {
      mxfError("Invalid random index - partitions out of order"
               " (partition address = 0x%" MXFPRIx64 ","
               " address of previous partition = 0x%" MXFPRIx64 ").\n",
               rit->_offset,
               previous);
    }
    previous = rit->_offset;
 }

  // Check that each entry in the random index corresponds to a
  // partition in the file.
  //
  for (rit = rip._index.begin(); rit != rip._index.end(); ++rit) {
    bool found = false;
    mxfRIPEntry e = *rit;
    PartitionList::const_iterator pit;
    for (pit = partitions.begin(); pit != partitions.end(); ++pit) {
      mxfPartition* p = *pit;
      if (e._offset == p->_address) {
        found = true;
        break;
      }
    }
    if (!found) {
      mxfError("Invalid random index entry - no such partition"
               " (SID = %" MXFPRIu32 ", address = 0x%" MXFPRIx64 ").\n",
               e._sid,
               e._offset);
    }
  }

  // Check that each partition in the file has an entry in the random index.
  //
  PartitionList::const_iterator pit;
  for (pit = partitions.begin(); pit != partitions.end(); ++pit) {
    bool found = false;
    mxfPartition* p = *pit;
    RandomIndex::const_iterator rit;
    for (rit = rip._index.begin(); rit != rip._index.end(); ++rit) {
      mxfRIPEntry e = *rit;
      if (p->_address == e._offset) {
         found = true;
        break;
      }
    }
    if (!found) {
      mxfError("Invalid random index - missing partition"
               " (partition address = 0x%" MXFPRIx64 ").\n",
               p->_address);
    }
  }
  if (memcmp(&V10RandomIndexMetadata, &rip._key, sizeof(mxfKey)) == 0) {
    mxfError("Random index key at offset 0x%" MXFPRIx64 " is obsolete.\n",
             keyPosition);
  }
}

mxfPartition* checkFooterPartition(mxfPartition* footer,
                                   mxfPartition* partition);

mxfPartition* checkFooterPartition(mxfPartition* footer,
                                   mxfPartition* partition)
{
  if (footer != 0) {
    mxfError(currentKey, keyPosition, "More than one footer");
  }
  return partition;
}

void checkPartitionLength(mxfUInt64& length)
{
  mxfUInt64 entrySize = sizeof(mxfKey);     // Essence container label
  if (length < partitionFixedSize) {
    // Valid length >= size of fixed portion
    mxfError(currentKey,
             keyPosition,
             "Invalid partition length - length too small");
  } else {
    mxfUInt64 labelBytes = length - partitionFixedSize;
    mxfUInt64 labels = labelBytes / entrySize;
    if ((labels * entrySize ) != labelBytes) {
      mxfError(currentKey,
               keyPosition,
               "Invalid partition length -"
               " space for essence container labels "
               "(%" MXFPRIu64 " - %" MXFPRIu64 " = %" MXFPRIu64 ")"
               " not a multiple of %" MXFPRIu64 " bytes.",
               length,
               partitionFixedSize,
               labelBytes,
               entrySize);
    }
  }
}

void printPartition(mxfKey& k, mxfLength& len, mxfFile infile);

void printPartition(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  checkPartitionLength(len);
  dumpMxfUInt16("Major Version", infile);
  dumpMxfUInt16("Minor Version", infile);
  dumpMxfUInt32("KAGSize", infile);
  dumpMxfUInt64("ThisPartition", infile);
  dumpMxfUInt64("PreviousPartition", infile);
  dumpMxfUInt64("FooterPartition", infile);
  dumpMxfUInt64("HeaderByteCount", infile);
  dumpMxfUInt64("IndexByteCount", infile);
  dumpMxfUInt32("IndexSID", infile);
  dumpMxfUInt64("BodyOffset", infile);
  dumpMxfUInt32("BodySID", infile);
  dumpMxfOperationalPattern("Operational Pattern", infile);
  fprintf(stdout, "%20s = ", "EssenceContainers");
  mxfUInt32 elementCount;
  readMxfUInt32(elementCount, infile);
  fprintf(stdout, "[ count = ");
  printDecField(stdout, elementCount);
  fprintf(stdout, " ]\n");
  mxfUInt32 elementSize;
  readMxfUInt32(elementSize, infile);
  checkElementSize(sizeof(mxfKey), elementSize, elementCount);
  mxfUInt32 count = checkElementCount(elementCount,
                                      sizeof(mxfKey),
                                      len,
                                      partitionFixedSize);
  for (mxfUInt32 i = 0; i < count; i++) {
    mxfKey essence;
    readMxfLabel(essence, infile);
    printEssenceContainerLabel(essence, i, stdout);
  }
}

void printHeaderPartition(mxfKey& k, mxfLength& len, mxfFile infile);

void printHeaderPartition(mxfKey& k, mxfLength& len, mxfFile infile)
{
  printPartition(k, len, infile);
}

void printBodyPartition(mxfKey& k, mxfLength& len, mxfFile infile);

void printBodyPartition(mxfKey& k, mxfLength& len, mxfFile infile)
{
  printPartition(k, len, infile);
}

void printGenericStreamPartition(mxfKey& k, mxfLength& len, mxfFile infile);

void printGenericStreamPartition(mxfKey& k, mxfLength& len, mxfFile infile)
{
  printPartition(k, len, infile);
}

void printFooterPartition(mxfKey& k, mxfLength& len, mxfFile infile);

void printFooterPartition(mxfKey& k, mxfLength& len, mxfFile infile)
{
  printPartition(k, len, infile);
}

struct mxfIndexSegment {
  mxfPartition* _partition;

  mxfKey _instanceUID; // Req
  bool _hasInstanceUID;

  mxfRational _indexEditRate; //  Req
  bool _hasIndexEditRate;

  mxfUInt64 _indexStartPosition; // Req
  bool _hasIndexStartPosition;

  mxfUInt64 _indexDuration; // Req
  bool _hasIndexDuration;

  mxfUInt32 _editUnitByteCount; // D/req
  bool _hasEditUnitByteCount;

  mxfUInt32 _indexSID; // D/req
  bool _hasIndexSID;

  mxfUInt32 _bodySID; // Req
  bool _hasBodySID;

  mxfUInt08 _sliceCount; // D/req
  bool _hasSliceCount;

  mxfUInt08 _posTableCount; // Opt
  bool _hasPosTableCount;

  // Delta entry array // Opt
  mxfUInt32 _deltaEntryCount;
  mxfUInt32 _deltaEntrySize;
  mxfUInt64 _deltaEntryArrayPosition;
  bool _hasDeltaEntryArray;

  // Index Entry Array // D/req
  mxfUInt32 _indexEntryCount;
  mxfUInt32 _indexEntrySize;
  mxfUInt64 _indexEntryArrayPosition;
  bool _hasIndexEntryArray;

  bool _isV10Index;
};

void initializeIndexSegment(mxfIndexSegment* index, mxfKey& key);

void initializeIndexSegment(mxfIndexSegment* index, mxfKey& key)
{
  index->_partition = 0;

  memcpy(&index->_instanceUID, &nullMxfKey, sizeof(mxfKey));
  index->_hasInstanceUID = false;

  index->_indexEditRate.numerator = 0;
  index->_indexEditRate.denominator = 0;
  index->_hasIndexEditRate = false;

  index->_indexStartPosition = 0;
  index->_hasIndexStartPosition = false;

  index->_indexDuration = 0;
  index->_hasIndexDuration = false;

  index->_editUnitByteCount = 0;
  index->_hasEditUnitByteCount = false;

  index->_indexSID = 0;
  index->_hasIndexSID = false;

  index->_bodySID = 0;
  index->_hasBodySID = false;

  index->_sliceCount = 0;
  index->_hasSliceCount = false;

  index->_posTableCount = 0;
  index->_hasPosTableCount = false;

  index->_deltaEntryCount = 0;
  index->_deltaEntrySize = 0;
  index->_deltaEntryArrayPosition = 0;
  index->_hasDeltaEntryArray = false;

  index->_indexEntryCount = 0;
  index->_indexEntrySize = 0;
  index->_indexEntryArrayPosition = 0;
  index->_hasIndexEntryArray = false;

  if (memcmp(&key, &V10IndexTableSegment, sizeof(mxfKey)) == 0) {
    index->_isV10Index = true;
  } else {
    index->_isV10Index = false;
  }
}

void readIndexSegment(mxfIndexSegment* index, mxfLength& len, mxfFile infile);

void readIndexSegment(mxfIndexSegment* index, mxfLength& len, mxfFile infile)
{
  index->_partition = currentPartition;
  mxfLength remainder = len;
  while (remainder > 0) {
    mxfLocalKey identifier;
    readMxfLocalKey(identifier, infile);
    mxfUInt16 length;
    readMxfUInt16(length, infile);
    remainder = remainder - 4;

    if (identifier == 0x3c0a) {
      // InstanceUID
      readMxfLabel(index->_instanceUID, infile);
      remainder = remainder - 16;
      index->_hasInstanceUID = true;
    } else if (identifier == 0x3f05) {
      // Edit Unit Byte Count
      readMxfUInt32(index->_editUnitByteCount, infile);
      remainder = remainder - 4;
      index->_hasEditUnitByteCount = true;
    } else if (identifier == 0x3f06) {
      // IndexSID
      readMxfUInt32(index->_indexSID, infile);
      remainder = remainder - 4;
      index->_hasIndexSID = true;
    } else if (identifier == 0x3f07) {
      // BodySID
      readMxfUInt32(index->_bodySID, infile);
      remainder = remainder - 4;
      index->_hasBodySID = true;
    } else if (identifier == 0x3f08) {
      // Slice Count
      readMxfUInt08(index->_sliceCount, infile);
      remainder = remainder - 1;
      index->_hasSliceCount = true;
    } else if (identifier == 0x3f0a) {
      // Entry array
      readMxfUInt32(index->_indexEntryCount, infile);
      readMxfUInt32(index->_indexEntrySize, infile);
      remainder = remainder - 8;
      index->_indexEntryArrayPosition = position(infile);
      mxfUInt64 size = index->_indexEntryCount * index->_indexEntrySize;
      skipBytes(size, infile);
      remainder = remainder - size;
      index->_hasIndexEntryArray = true;
    } else if (identifier == 0x3f0b) {
      // Index Edit Rate
      readMxfRational(index->_indexEditRate, infile);
      remainder = remainder - 8;
      index->_hasIndexEditRate = true;
    } else if (identifier == 0x3f0c) {
      // Index Start Position
      readMxfUInt64(index->_indexStartPosition, infile);
      remainder = remainder - 8;
      index->_hasIndexStartPosition = true;
    } else if (identifier == 0x3f0d) {
      // Index Duration
      readMxfUInt64(index->_indexDuration, infile);
      remainder = remainder - 8;
      index->_hasIndexDuration = true;
    } else if (identifier == 0x3f0e) {
      // Pos Table Count
      readMxfUInt08(index->_posTableCount, infile);
      remainder = remainder - 1;
      index->_hasPosTableCount = true;
    } else if (identifier == 0x3f09) {
      // Delta Entry Array
      readMxfUInt32(index->_deltaEntryCount, infile);
      readMxfUInt32(index->_deltaEntrySize, infile);
      remainder = remainder - 8;
      index->_deltaEntryArrayPosition = position(infile);
      mxfUInt64 size = index->_deltaEntryCount * index->_deltaEntrySize;
      skipBytes(size, infile);
      remainder = remainder - size;
      index->_hasDeltaEntryArray = true;
	} else if ((identifier == 0x0000) && (length == 0x0000)) {
      mxfError(currentKey,
               keyPosition,
               "Local key and length are zero, skipping %" MXFPRIu64 " bytes.",
               remainder);
      skipBytes(remainder, infile);
      remainder = 0;
    } else {
      mxfError(currentKey,
               keyPosition,
               "Local key (%04" MXFPRIx16 ") not recognized.",
               identifier);
      mxfUInt16 vLength = validateLocalV(length, remainder, infile);
      skipBytes(vLength, infile);
    }
  }
}

void printIndexSegment(mxfIndexSegment* index);

void printIndexSegment(mxfIndexSegment* index)
{
  // InstanceUID
  if (index->_hasInstanceUID) {
    printMxfKey(stdout, "InstanceUID", index->_instanceUID);
  }

  // Index Edit Rate
  if (index->_hasIndexEditRate) {
    printMxfRational(stdout, "Index Edit Rate", index->_indexEditRate);
  }

  // Index Start Position
  if (index->_hasIndexStartPosition) {
    printMxfUInt64(stdout, "Index Start Position", index->_indexStartPosition);
  }

  // Index Duration
  if (index->_hasIndexDuration) {
    printMxfUInt64(stdout, "Index Duration", index->_indexDuration);
  }

  // Edit Unit Byte Count
  if (index->_hasEditUnitByteCount) {
    printMxfUInt32(stdout, "Edit Unit Byte Count", index->_editUnitByteCount);
  }

  // IndexSID
  if (index->_hasIndexSID) {
    printMxfUInt32(stdout, "IndexSID", index->_indexSID);
  }

  // BodySID
  if (index->_hasBodySID) {
    printMxfUInt32(stdout, "BodySID", index->_bodySID);
  }

  // Slice Count
  if (index->_hasSliceCount) {
    printMxfUInt08(stdout, "SliceCount", index->_sliceCount);
  }

  // Pos Table Count
  if (index->_hasPosTableCount) {
    printMxfUInt08(stdout, "Pos Table Count", index->_posTableCount);
  }
}

void validateArray(mxfUInt32 defaultSize,
                   mxfUInt32 expectedSize,
                   mxfUInt32 actualSize,
                   mxfUInt32 elementCount,
                   const char* arrayName);

void validateArray(mxfUInt32 defaultSize,
                   mxfUInt32 expectedSize,
                   mxfUInt32 actualSize,
                   mxfUInt32 elementCount,
                   const char* arrayName)
{
  if (expectedSize == 0) {
    expectedSize = defaultSize;
  }
  checkElementSize(expectedSize,
                   actualSize,
                   elementCount);
  // (16-bits - (4-byte element count + 4-byte element size)) / element size
  mxfUInt64 maximumElementCount = (65535 - 8) / expectedSize;
  if (elementCount > maximumElementCount) {
    mxfError(currentKey,
             keyPosition,
             "Invalid element count for %s -"
             " element count (%" MXFPRIu32 ") exceeds maximum allowed"
             " (%" MXFPRIu64 " at %" MXFPRIu32 " bytes each) in a local set.",
             arrayName,
             elementCount,
             maximumElementCount,
             expectedSize);
  }
}

void validateIndexSegment(mxfIndexSegment* index);

void validateIndexSegment(mxfIndexSegment* index)
{

  // Check that all required properties are present.
  //
  if (!index->_hasInstanceUID) {
    missingProperty(currentKey, keyPosition, "InstanceUID");
  }

  if (!index->_hasIndexEditRate) {
    missingProperty(currentKey, keyPosition, "Index Edit Rate");
  }

  if (!index->_hasIndexStartPosition) {
    missingProperty(currentKey, keyPosition, "Index Start Position");
  }

  if (!index->_hasIndexDuration) {
    missingProperty(currentKey, keyPosition, "Index Duration");
  }

  if (!index->_hasBodySID) {
    missingProperty(currentKey, keyPosition, "BodySID");
  }

  // Other checks.
  //
  if (index->_hasIndexEntryArray && index->_indexEntryCount != 0) {
    if (index->_hasEditUnitByteCount && index->_editUnitByteCount != 0) {
      mxfWarning(currentKey,
                 keyPosition,
                 "Index segment has index entries (%" MXFPRIu32 ")"
                 " and edit unit byte count (%" MXFPRIu32 ")",
                 index->_indexEntryCount,
                 index->_editUnitByteCount);
    }
    // else VBR index
  } else {
    if (!index->_hasEditUnitByteCount || index->_editUnitByteCount == 0) {
      mxfWarning(currentKey,
                 keyPosition,
                 "Index segment has no index entries"
                 " and no edit unit byte count");
    }
    // else CBR index
  }

  if (index->_hasDeltaEntryArray) {
    validateArray(6,
                  index->_deltaEntrySize,
                  index->_deltaEntrySize,
                  index->_deltaEntryCount,
                  "Delta Entry Array");
  }

  if (index->_hasIndexEntryArray) {
    validateArray(11,
                  index->_indexEntrySize,
                  index->_indexEntrySize,
                  index->_indexEntryCount,
                  "Index Entry Array");
  }

  if (index->_hasIndexDuration) {
    if (index->_indexEntryCount > index->_indexDuration) {
      mxfWarning(currentKey,
                 keyPosition,
                 "Index entry count (%" MXFPRIu32 ") greater than"
                 " index duration (%" MXFPRIu64 ")",
                 index->_indexEntryCount,
                 index->_indexDuration);
    }
  }

  mxfPartition* partition = index->_partition;
  if (partition != 0) {
    // we're in validation mode
    if (index->_hasIndexSID) {
      if (partition->_indexSID != index->_indexSID) {
        mxfError(currentKey,
                 keyPosition,
                 "Incorrect value for IndexSID -"
                 " partition has IndexSID = %" MXFPRIu32 ","
                 " index segment has IndexSID = %" MXFPRIu32 ".",
                 partition->_indexSID,
                 index->_indexSID);
      }
    }
  }
  if (index->_isV10Index) {
    mxfError("Index table segment key at offset 0x%" MXFPRIx64 " is obsolete.\n",
             keyPosition);
  }
}

void dumpFlags(mxfUInt08 flags);

void dumpFlags(mxfUInt08 flags)
{
  char symbols[] = "IbPB";
  mxfUInt08 prediction = (flags & 0x30) >> 4;
  char ch = symbols[prediction];
  fprintf(stdout, " %c ", ch);
}

void dumpIndexEntryArray(mxfUInt32 entryCount,
                         mxfUInt32 entrySize,
                         mxfFile& infile);

void dumpIndexEntryArray(mxfUInt32 entryCount,
                         mxfUInt32 entrySize,
                         mxfFile& infile)
{
  mxfUInt32 sliceCount = (entrySize - 11) / 4;
  fprintf(stdout, "     IndexEntryArray = ");
  fprintf(stdout, "[ Number of entries = ");
  printDecField(stdout, entryCount);
  fprintf(stdout, "\n");
  fprintf(stdout, "                       ");
  fprintf(stdout, "  Entry size        = ");
  printDecField(stdout, entrySize);
  fprintf(stdout, " ]\n");

  if (entryCount > 0) {
    if (sliceCount > 0) {
      fprintf(stdout, "         ");
      fprintf(stdout, "        Temporal   Anchor  Flags        Stream        Slice\n");
      fprintf(stdout, "         ");
      fprintf(stdout, "          Offset   Offset               Offset      Offsets\n");
    } else {
      fprintf(stdout, "         ");
      fprintf(stdout, "        Temporal   Anchor  Flags        Stream\n");
      fprintf(stdout, "         ");
      fprintf(stdout, "          Offset   Offset               Offset\n");
    }
  }

  mxfUInt32 count = 0;
  for (mxfUInt32 i = 0; i < entryCount; i++) {
    mxfUInt08 temporalOffset; // signed
    readMxfUInt08(temporalOffset, infile);
    mxfUInt08 anchorOffset; // signed
    readMxfUInt08(anchorOffset, infile);
    mxfUInt08 flags;
    readMxfUInt08(flags, infile);
    mxfUInt64 streamOffset;
    readMxfUInt64(streamOffset, infile);

    mxfUInt32* sliceOffsets = new mxfUInt32[sliceCount];
    for (mxfUInt32 s = 0; s < sliceCount; s++) {
      mxfUInt32 sliceOffset;
      readMxfUInt32(sliceOffset, infile);
      sliceOffsets[s] = sliceOffset;
    }

    if (!cFlag || (i < maxIndex)) {
      fprintf(stdout, "    ");
      printDecField(stdout, i);
      fprintf(stdout, " :");
      fprintf(stdout, "    ");
      printSignedDecField(stdout, temporalOffset);
      fprintf(stdout, "    ");
      printSignedDecField(stdout, anchorOffset);
      fprintf(stdout, "     ");
      printHexFieldPad(stdout, flags);
      fprintf(stdout, " ");
      dumpFlags(flags);
      printDecField(stdout, streamOffset);
      fprintf(stdout, "\n");

      // Slice offsets
      if (sliceCount > 0) {
        // The first slice offset is on the same line as
        // the other of the index entry fields.
        fprintf(stdout, "     ");
        printDecField(stdout, sliceOffsets[0]);
        fprintf(stdout, "\n");

        // The rest of the slice offsets
        for (mxfUInt32 s = 1; s < sliceCount; s++) {
          fprintf(stdout,
                 "                                                          ");
          printDecField(stdout, sliceOffsets[s]);
          fprintf(stdout, "\n");
        }
      } else {
        fprintf(stdout, "\n");
      }

      delete [] sliceOffsets;
      sliceOffsets = 0;

      count = count + 1;
    }
  }
  if (cFlag && (count < entryCount)) {
    fprintf(stdout, "[ Index table truncated from ");
    printDecField(stdout, entryCount);
    fprintf(stdout, " entries to ");
    printDecField(stdout, maxIndex);
    fprintf(stdout, " entries ]\n");
  }
}

void dumpDeltaEntryArray(mxfUInt32 entryCount,
                         mxfUInt32 entrySize,
                         mxfFile& infile);

void dumpDeltaEntryArray(mxfUInt32 entryCount,
                         mxfUInt32 entrySize,
                         mxfFile& infile)
{
  fprintf(stdout, "     DeltaEntryArray = ");
  fprintf(stdout, "[ Number of entries = ");
  printDecField(stdout, entryCount);
  fprintf(stdout, "\n");
  fprintf(stdout, "                       ");
  fprintf(stdout, "  Entry size        = ");
  printDecField(stdout, entrySize);
  fprintf(stdout, " ]\n");

  if (entryCount > 0) {
    fprintf(stdout, "         ");
    fprintf(stdout, "        Pos Table  Slice     Element\n");
    fprintf(stdout, "         ");
    fprintf(stdout, "            Index              Delta\n");
  }

  for (mxfUInt32 i = 0; i < entryCount; i++) {
    mxfUInt08 posTableIndex; // signed
    readMxfUInt08(posTableIndex, infile);
    mxfUInt08 slice;
    readMxfUInt08(slice, infile);
    mxfUInt32 elementDelta;
    readMxfUInt32(elementDelta, infile);

    fprintf(stdout, "    ");
    printDecField(stdout, i);
    fprintf(stdout, " :");
    fprintf(stdout, "    ");
    printDecField(stdout, posTableIndex);
    fprintf(stdout, "    ");
    printDecField(stdout, slice);
    fprintf(stdout, "     ");
    printDecField(stdout, elementDelta);
    fprintf(stdout, "\n");
  }
}

void printIndexTable(mxfKey& k, mxfLength& len, mxfFile infile);

void printIndexTable(mxfKey& k, mxfLength& len, mxfFile infile)
{
  mxfUInt64 startPosition = position(infile);
  mxfIndexSegment index;
  initializeIndexSegment(&index, k);
  readIndexSegment(&index, len, infile);
  validateIndexSegment(&index);
  printIndexSegment(&index);
  if (index._hasDeltaEntryArray) {
    setPosition(infile, index._deltaEntryArrayPosition);
    dumpDeltaEntryArray(index._deltaEntryCount, index._deltaEntrySize, infile);
  }
  if (index._hasIndexEntryArray) {
    setPosition(infile, index._indexEntryArrayPosition);
    dumpIndexEntryArray(index._indexEntryCount, index._indexEntrySize, infile);
  }
  setPosition(infile, startPosition + len);
}

void validateIndexSegment(mxfKey& k, mxfLength& len, mxfFile infile);

void validateIndexSegment(mxfKey& k, mxfLength& len, mxfFile infile)
{
  mxfUInt64 startPosition = position(infile);
  mxfIndexSegment index;
  initializeIndexSegment(&index, k);
  readIndexSegment(&index, len, infile);
  validateIndexSegment(&index);
  setPosition(infile, startPosition + len);
}

// Size of the fixed portion of a primer
mxfUInt64 primerFixedSize =
  sizeof(mxfUInt32) + // Element count
  sizeof(mxfUInt32);  // Element size

void checkPrimerLength(mxfUInt64& length);

void checkPrimerLength(mxfUInt64& length)
{
  // Size of a primer entry
  mxfUInt64 entrySize = sizeof(mxfLocalKey) + // Local key
                        sizeof(mxfKey);       // Global key

  if (length < primerFixedSize) {
    // Valid length >= size of fixed portion
    mxfError(currentKey,
             keyPosition,
             "Invalid primer length - length is too small");
  } else {
    // Valid length == 8 + (N * 18)
    mxfUInt64 entryBytes = length - primerFixedSize;
    mxfUInt64 entries = entryBytes / entrySize;
    if ((entries * entrySize) != entryBytes) {
      mxfError(currentKey,
               keyPosition,
               "Invalid primer length -"
               " %" MXFPRIu64 " != %" MXFPRIu64 " + (N * %" MXFPRIu64 ")",
               length,
               primerFixedSize,
               entrySize);
    }
  }
}

bool usePrimer = true;
bool diffFriendly = false;

void printPrimer(mxfKey& k, mxfLength& len, mxfFile infile);

void printPrimer(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  checkPrimerLength(len);
  mxfUInt32 elementCount;
  readMxfUInt32(elementCount, infile);
  mxfUInt32 elementSize;
  readMxfUInt32(elementSize, infile);
  checkElementSize(sizeof(mxfUInt16) + sizeof(mxfKey),
                   elementSize,
                   elementCount);
  mxfUInt32 count = checkElementCount(elementCount,
                                      sizeof(mxfUInt16) + sizeof(mxfKey),
                                      len,
                                      primerFixedSize);

  fprintf(stdout, "  [ Number of entries = ");
  printDecField(stdout, elementCount);
  if (diffFriendly) {
    fprintf(stdout, ",\n    Entry size        = ");
  } else {
    fprintf(stdout, ", Entry size        = ");
  }
  printDecField(stdout, elementSize);
  fprintf(stdout, " ]\n");

  if (elementCount > 0) {
    if (diffFriendly) {
      fprintf(stdout, "  Local Tag\n  UID\n");
    } else {
      fprintf(stdout, "  Local Tag      UID\n");
    }
  }

  for (mxfUInt32 j = 0; j < count; j++) {
    mxfLocalKey identifier;
    readMxfLocalKey(identifier, infile);
    mxfKey longIdentifier;
    readMxfLabel(longIdentifier, infile);
    if (usePrimer) {
      updateMXFLocalKey(longIdentifier, identifier);
      updateAAFLocalKey(longIdentifier, identifier);
    }
    fprintf(stdout, "  ");
    printMxfLocalKey(identifier, stdout);
    if (diffFriendly) {
      fprintf(stdout, "     :\n    ");
    } else {
      fprintf(stdout, "     :    ");
    }
    printMxfKey(longIdentifier, stdout);
    fprintf(stdout, "\n");
  }
}

void printSystemMetadata(mxfKey& k, mxfLength& len, mxfFile infile);

void printSystemMetadata(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  if (len == 57) {
    dumpMxfUInt08("Bitmap", infile);
    dumpMxfUInt08("Rate", infile);
    dumpMxfUInt08("Type", infile);
    dumpMxfUInt16("Channel Handle", infile);
    dumpMxfUInt16("Continuity Count", infile);
    dumpMxfLabel("Label", infile);
    dumpMxfUInt08Array("Creation Date", 17, infile);
    dumpMxfUInt08Array("User Date", 17, infile);
  } else {
    printV(len, false, 0, infile);
  }
}

void printObjectDirectory(mxfKey& k, mxfLength& len, mxfFile infile);

void printObjectDirectory(mxfKey& /* k */,
                          mxfLength& /* len */,
                          mxfFile infile)
{
  mxfUInt64 entryCount;
  readMxfUInt64(entryCount, infile);
  mxfUInt08 entrySize;
  readMxfUInt08(entrySize, infile);

  fprintf(stdout, "  [ Number of entries = ");
  printDecField(stdout, entryCount);
  if (diffFriendly) {
    fprintf(stdout, ",\n    Entry size        = ");
  } else {
    fprintf(stdout, ", Entry size        = ");
  }
  printDecField(stdout, entrySize);
  fprintf(stdout, " ]\n");

  if (entryCount > 0) {
    if (diffFriendly) {
      fprintf(stdout, "  Object\n");
      fprintf(stdout, "  Offset\n");
      fprintf(stdout, "  Flags\n");
    } else {
      fprintf(stdout, "  Object");
      fprintf(stdout, "                                           Offset");
      fprintf(stdout, "            Flags\n");
    }
  }

  for (mxfUInt64 i = 0; i < entryCount; i++) {
    mxfKey instance;
    readMxfLabel(instance, infile);
    mxfUInt64 offset;
    readMxfUInt64(offset, infile);
    mxfUInt08 flags;
    readMxfUInt08(flags, infile);

    fprintf(stdout, "  ");
    printMxfKey(instance, stdout);
    if (diffFriendly) {
      fprintf(stdout, "\n  ");
    } else {
      fprintf(stdout, "  ");
    }
    printHexFieldPad(stdout, offset);
    if (diffFriendly) {
      fprintf(stdout, "\n  ");
    } else {
      fprintf(stdout, "  ");
    }
    printHexFieldPad(stdout, flags);
    fprintf(stdout, "\n");
  }
}

void printMetaDictionary(mxfKey& k, mxfLength& len, mxfFile infile);

void printMetaDictionary(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  mxfLength remaining = len;
  while (remaining > 0) {
    mxfUInt16 length;
    readMxfUInt16(length, infile);
    remaining = remaining - sizeof(mxfUInt16);
    mxfByte tag;
    readMxfUInt08(tag, infile);
    switch (tag) {
    case 0x10:
      dumpExtensionClass(infile);
      break;
    case 0x20:
      dumpExtensionProperty(infile);
      break;
    case 0x31:
      dumpExtensionTypeInteger(infile);
      break;
    case 0x32:
      dumpExtensionTypeCharacter(infile);
      break;
    case 0x33:
      dumpExtensionTypeStrongObjectReference(infile);
      break;
    case 0x34:
      dumpExtensionTypeWeakObjectReference(infile);
      break;
    case 0x35:
      dumpExtensionTypeRename(infile);
      break;
    case 0x36:
      dumpExtensionTypeEnumerated(infile);
      break;
    case 0x37:
      dumpExtensionTypeFixedArray(infile);
      break;
    case 0x38:
      dumpExtensionTypeVaryingArray(infile);
      break;
    case 0x39:
      dumpExtensionTypeSet(infile);
      break;
    case 0x3a:
      dumpExtensionTypeRecord(infile);
      break;
    case 0x3b:
      dumpExtensionTypeStream(infile);
      break;
    case 0x3c:
      dumpExtensionTypeString(infile);
      break;
    case 0x3d:
      dumpExtensionTypeExtendibleEnumerated(infile);
      break;
    case 0x3e:
      dumpExtensionTypeIndirect(infile);
      break;
    case 0x3f:
      dumpExtensionTypeOpaque(infile);
      break;
    default:
      mxfError("Invalid definition tag (%" MXFPRIx08 ").\n", tag);
      skipBytes(length - 1, infile);
      break;
    }
    fprintf(stdout, "\n");
    remaining = remaining - length;
  }
}

void printRandomIndex(mxfKey& k, mxfLength& len, mxfFile infile);

void printRandomIndex(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  mxfUInt64 entryCount = len / (sizeof(mxfUInt32) + sizeof(mxfUInt64));
  fprintf(stdout, "  [ Number of entries = ");
  printDecField(stdout, entryCount );
  fprintf(stdout, " ]\n");
  fprintf(stdout, "       Partition           SID        Offset\n");
  for (mxfUInt32 i = 0; i < entryCount; i++) {
    mxfUInt32 sid;
    mxfUInt64 offset;
    readMxfUInt32(sid, infile);
    readMxfUInt64(offset, infile);
    fprintf(stdout, "    ");
    printDecField(stdout, i);
    fprintf(stdout, " :");
    fprintf(stdout, "    ");
    printDecField(stdout, sid);
    fprintf(stdout, "    ");
    if (base == 10) {
      printDecField(stdout, offset);
    } else {
      printHexFieldPad(stdout, offset);
    }
    fprintf(stdout, "\n");
  }
  mxfUInt32 length;
  readMxfUInt32(length, infile);
  fprintf(stdout, "  [ Overall length = ");
  printDecField(stdout, length);
  fprintf(stdout, " ]\n");
}

void printV10RandomIndex(mxfKey& k, mxfLength& len, mxfFile infile);

void printV10RandomIndex(mxfKey& k, mxfLength& len, mxfFile infile)
{
  printRandomIndex(k, len, infile);
}

void klvDumpFile(mxfFile infile)
{
  mxfUInt64 fileSize = size(infile);
  mxfKey k;
  while (readOuterMxfKey(k, infile)) {
    checkKey(k);
    mxfLength len;
    mxfUInt08 llen;
    readMxfLength(len, llen, infile);
    printKL(k, len, llen);
    len = checkLength(len, fileSize, position(infile));
    printV(len, limitBytes, limit, infile);
  }
}

void setDumpFile(mxfFile infile)
{
  mxfUInt64 fileSize = size(infile);
  mxfKey k;
  while (readOuterMxfKey(k, infile)) {
    checkKey(k);
    mxfLength len;
    mxfUInt08 llen;
    readMxfLength(len, llen, infile);
    printKL(k, len, llen);
    len = checkLength(len, fileSize, position(infile));

    if (isLocalSet(k)) {
      printLocalSet(k, len, infile);
    } else if (isFill(k)) {
      printFill(k, len, infile);
    } else if (isEssenceElement(k)) {
      printEssenceElement(k, len, limitBytes, limit, infile);
    } else {
      printV(len, false, 0, infile);
    }
  }
}

bool dumpDark = false;
bool dumpRunIn = false;

void mxfDumpKLV(mxfKey& k, mxfLength& len, mxfFile infile);

void mxfDumpKLV(mxfKey& k, mxfLength& len, mxfFile infile)
{
  if (memcmp(&OpenIncompleteHeader, &k, sizeof(mxfKey)) == 0) {
    printHeaderPartition(k, len, infile);
  } else if (memcmp(&IncompleteHeader, &k, sizeof(mxfKey)) == 0) {
    printHeaderPartition(k, len, infile);
  } else if (memcmp(&OpenHeader, &k, sizeof(mxfKey)) == 0) {
    printHeaderPartition(k, len, infile);
  } else if (memcmp(&Header, &k, sizeof(mxfKey)) == 0) {
    printHeaderPartition(k, len, infile);
  } else if (memcmp(&OpenIncompleteBody, &k, sizeof(mxfKey)) == 0) {
    printBodyPartition(k, len, infile);
  } else if (memcmp(&IncompleteBody, &k, sizeof(mxfKey)) == 0) {
    printBodyPartition(k, len, infile);
  } else if (memcmp(&OpenBody, &k, sizeof(mxfKey)) == 0) {
    printBodyPartition(k, len, infile);
  } else if (memcmp(&Body, &k, sizeof(mxfKey)) == 0) {
    printBodyPartition(k, len, infile);
  } else if (memcmp(&GenericStream, &k, sizeof(mxfKey)) == 0) {
    printGenericStreamPartition(k, len, infile);
  } else if (memcmp(&IncompleteFooter, &k, sizeof(mxfKey)) == 0) {
    printFooterPartition(k, len, infile);
  } else if (memcmp(&Footer, &k, sizeof(mxfKey)) == 0) {
    printFooterPartition(k, len, infile);
  } else if (memcmp(&Primer, &k, sizeof(mxfKey)) == 0) {
    printPrimer(k, len, infile);
    // The following are not yet dealt with explicitly, they are either
    // handled as their AAF equivalents or as generic local sets.
    //
    // Preface
    // Identification
    // ContentStorage
    // EssenceContainerData
    // MaterialPackage
    // SourcePackage
    // Track
    // Sequence
    // SourceClip
    // Timecode12MComponent
    // TimecodeStream12MComponent
    // DMSegment
    // DMSourceClip
    // FileDescriptor
    // GenPictureEssenceDesc
    // CDCIEssenceDescriptor
    // RGBAEssenceDescriptor
    // GenSoundEssenceDesc
    // GenDataEssenceDesc
    // MultipleDescriptor
    // NetworkLocator
    // TextLocator
    //
  } else if (memcmp(&SystemMetadata, &k, sizeof(mxfKey)) == 0) {
    printSystemMetadata(k, len, infile);
  } else if (memcmp(&ObjectDirectory, &k, sizeof(mxfKey)) == 0) {
    printObjectDirectory(k, len, infile);
  } else if (memcmp(&MetaDictionary, &k, sizeof(mxfKey)) == 0) {
    printMetaDictionary(k, len, infile);
  } else if (memcmp(&V10IndexTableSegment, &k, sizeof(mxfKey)) == 0) {
    printIndexTable(k, len, infile);
  } else if (memcmp(&IndexTableSegment, &k, sizeof(mxfKey)) == 0) {
    printIndexTable(k, len, infile);
  } else if (memcmp(&V10RandomIndexMetadata, &k, sizeof(mxfKey)) == 0) {
    printV10RandomIndex(k, len, infile);
  } else if (memcmp(&RandomIndexMetadata, &k, sizeof(mxfKey)) == 0) {
    printRandomIndex(k, len, infile);
  } else if (isFill(k)) {
    printFill(k, len, infile);
  } else if (isEssenceElement(k)) {
    printEssenceElement(k, len, limitBytes, limit, infile);
  } else {
    if ((mode == aafMode) && isAAFKey(k)) {
      printLocalSet(k, len, infile);
    } else if (!isDark(k, mode) || dumpDark) {
      if (isLocalSet(k)) {
        printLocalSet(k, len, infile);
      } else if (darkKeysAsSets) {
        printLocalSet(k, len, infile);
      } else {
        printV(len, false, 0, infile);
      }
    } else {
      darkItems = darkItems + 1;
      skipV(len, infile);
    }
    objectCount = objectCount + 1;
    objectBytes = objectBytes + (position(infile) - keyPosition);
  }
}

void mxfDumpFile(mxfFile infile)
{
  mxfUInt64 fileSize = size(infile);
  mxfKey k;
  while (readOuterMxfKey(k, infile)) {
    if (isPartition(k)) {
      memcpy(&previousPartitionKey, &currentPartitionKey, sizeof(mxfKey));
      memcpy(&currentPartitionKey, &k, sizeof(mxfKey));
      printObjectCount(previousPartitionKey);
    }
    checkKey(k);
    mxfLength len;
    mxfUInt08 llen;
    readMxfLength(len, llen, infile);
    if (isEssenceElement(k) || !isDark(k, mode) || dumpDark) {
      printKL(k, len, llen);
    }
    len = checkLength(len, fileSize, position(infile));
    mxfDumpKLV(k, len, infile);
  }
  printDarkItems();
  printObjectCount(currentPartitionKey);
}

void aafDumpKLV(mxfKey& k, mxfLength& len, mxfFile infile);

void aafDumpKLV(mxfKey& k, mxfLength& len, mxfFile infile)
{
  mxfDumpKLV(k, len, infile);
}

void aafDumpFile(mxfFile infile)
{
  mxfUInt64 fileSize = size(infile);
  mxfKey k;
  while (readOuterMxfKey(k, infile)) {
    if (isPartition(k)) {
      memcpy(&previousPartitionKey, &currentPartitionKey, sizeof(mxfKey));
      memcpy(&currentPartitionKey, &k, sizeof(mxfKey));
      printObjectCount(previousPartitionKey);
    }
    checkKey(k);
    mxfLength len;
    mxfUInt08 llen;
    readMxfLength(len, llen, infile);
    if (isEssenceElement(k) || !isDark(k, mode) || dumpDark) {
      printKL(k, len, llen);
    }
    len = checkLength(len, fileSize, position(infile));
    aafDumpKLV(k, len, infile);
  }
  printDarkItems();
}

void klvValidate(mxfFile infile);

void klvValidate(mxfFile infile)
{
  mxfUInt64 fileSize = size(infile);
  mxfKey k;
  while (readOuterMxfKey(k, infile)) {
    checkKey(k);
    mxfLength len;
    mxfUInt08 llen;
    readMxfLength(len, llen, infile);
    len = checkLength(len, fileSize, position(infile));
    skipV(len, infile);
  }

  fprintf(stderr,
          "%s : KLV validation       - ",
          programName());
  if (errors == 0) {
    fprintf(stderr, "passed.\n");
  } else {
    fprintf(stderr, "failed.\n");
  }
}

void validateLocalSet(mxfKey& k, mxfLength& len, mxfFile infile);

void validateLocalSet(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  mxfLength remainder = len;
  while (remainder > 0) {
    // Key (local identifier)
    mxfLocalKey identifier;
    validateLocalKey(identifier, remainder, infile);
    if (remainder == 0) {
      break;
    }

    // Length
    mxfUInt16 length;
    validateLocalLength(length, remainder, infile);
    if (remainder == 0) {
      break;
    }

    // Value
    mxfUInt16 vLength = validateLocalV(length, remainder, infile);
    skipBytes(vLength, infile);
  }
}

void validateObject(mxfKey& k, mxfLength& len, mxfFile infile);

void validateObject(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  mxfLength remainder = len;
  while (remainder > 0) {
    // Key (local identifier)
    mxfLocalKey identifier;
    mxfLength excess = remainder;
    validateLocalKey(identifier, remainder, infile);
    if (remainder == 0) {
      skipBytes(excess, infile);
      break;
    }
    // Check that this id is in the primer
    PrimerMap::const_iterator piter = primer.find(identifier);
    if (piter == primer.end()) {
      IdSet::const_iterator iditer = badids.find(identifier);
      if (iditer == badids.end()) {
        badids.insert(identifier);
        mxfError(currentKey,
                 keyPosition,
                 "Local key %04" MXFPRIx16 " not found in primer, "
                 "reporting first instance only",
                 identifier);
      }
    }

    // Length
    mxfUInt16 length;
    excess = remainder;
    validateLocalLength(length, remainder, infile);
    if (remainder == 0) {
      skipBytes(excess, infile);
      break;
    }

    // Value
    mxfUInt16 vLength = validateLocalV(length, remainder, infile);
    if (identifier == 0x3c0a) {
      if (vLength == sizeof(mxfKey)) {
        mxfKey oid;
        readMxfLabel(oid, infile);
        std::pair<ObjectSet::const_iterator, bool> r = objects.insert(oid);
        if (!r.second) {
          mxfError(currentKey, keyPosition, "Instance uid is not unique");
        }
      } else {
        mxfError(currentKey, keyPosition, "Incorrect length for instance uid");
        skipBytes(vLength, infile);
      }
    } else {
      skipBytes(vLength, infile);
    }
  }
}

void validatePrimer(mxfKey& /* k */, mxfLength& len, mxfFile infile)
{
  checkPrimerLength(len);
  mxfUInt32 elementCount;
  readMxfUInt32(elementCount, infile);
  mxfUInt32 elementSize;
  readMxfUInt32(elementSize, infile);
  checkElementSize(sizeof(mxfUInt16) + sizeof(mxfKey),
                   elementSize,
                   elementCount);
  mxfUInt32 count = checkElementCount(elementCount,
                                      sizeof(mxfUInt16) + sizeof(mxfKey),
                                      len,
                                      primerFixedSize);

  ObjectSet propertyIds;
  for (mxfUInt32 j = 0; j < count; j++) {
    mxfLocalKey identifier;
    readMxfLocalKey(identifier, infile);
    mxfKey longIdentifier;
    readMxfLabel(longIdentifier, infile);
    std::pair<PrimerMap::iterator, bool> r = primer.insert(
                                        PrimerMap::value_type(identifier,
                                                              longIdentifier));
    if (!r.second) {
      mxfError(currentKey,
               keyPosition,
               "Local key %04" MXFPRIx16 " is not unique",
               identifier);
    }

    std::pair<ObjectSet::iterator, bool> p = propertyIds.insert(
                                                               longIdentifier);
    if (!p.second) {
      mxfError("Key ");
      printMxfKey(longIdentifier, stderr);
      fprintf(stderr, " is not unique");
      fprintf(stderr,
              " (following %s key at offset 0x%" MXFPRIx64 ").\n",
              "Primer",
              keyPosition);
    }
    
    if (identifier < 0x8000) { // static key 
      size_t index;
      bool found = lookupAAFLocalKey(identifier, index);
      if (found) {
        if (memcmp(&aafLocalKeyTable[index]._key,
            &nullMxfKey,
            sizeof(mxfKey)) == 0) {
//          warning("Missing key for \"%s\"\n", aafLocalKeyTable[index]._name);
        } else if (memcmp(&aafLocalKeyTable[index]._key,
                   &longIdentifier,
                   sizeof(mxfKey)) != 0) {
          mxfError("Primer remaps static local key %04" MXFPRIx16 ""
                   " (\"%s\") from ",
                   identifier,
                   aafLocalKeyTable[index]._name);
          printMxfKey(aafLocalKeyTable[index]._key, stderr);
          fprintf(stderr, " to ");
          printMxfKey(longIdentifier, stderr);
          fprintf(stderr, "\n");
        }
      }
    }
  }
}

void setValidate(mxfFile infile);

void setValidate(mxfFile infile)
{
  mxfUInt64 fileSize = size(infile);
  mxfKey k;
  while (readOuterMxfKey(k, infile)) {
    checkKey(k);
    mxfLength len;
    mxfUInt08 llen;
    readMxfLength(len, llen, infile);
    len = checkLength(len, fileSize, position(infile));
    if (isLocalSet(k)) {
      validateLocalSet(k, len, infile);
    } else {
      skipV(len, infile);
    }
  }

  fprintf(stderr,
          "%s : local set validation - ",
          programName());
  if (errors == 0) {
    fprintf(stderr, "passed.\n");
  } else {
    fprintf(stderr, "failed.\n");
  }
}

void mxfValidate(mxfFile infile);

void mxfValidate(mxfFile infile)
{
  PartitionList p;
  mxfRandomIndex rip;
  mxfPartition* footer = 0;
  mxfUInt64 fileSize = size(infile);
  mxfKey k;
  while (readOuterMxfKey(k, infile)) {
    checkKey(k);
    mxfLength len;
    mxfUInt08 llen;
    readMxfLength(len, llen, infile);
    mxfLength length = len;
    len = checkLength(len, fileSize, position(infile));
    if (memcmp(&Primer, &k, sizeof(mxfKey)) == 0) {
      markMetadataStart(keyPosition);
      checkPrimerLength(length);
      skipV(len, infile);
      validatePrimer(k, len, infile);
    } else if (isPartition(k)) {
      objects.clear();
      primer.clear();
      badids.clear();
      markMetadataEnd(keyPosition);
      markIndexEnd(keyPosition);
      markEssenceSegmentEnd(keyPosition);
      checkPartitionLength(length);
      readPartition(p, len, infile);
      if (isFooter(k)) {
        footer = checkFooterPartition(footer, currentPartition);
      }
    } else if (isRandomIndex(k)) {
      objects.clear();
      primer.clear();
      badids.clear();
      markMetadataEnd(keyPosition);
      markIndexEnd(keyPosition);
      mxfUInt32 overall;
      readRandomIndex(rip, k, len, overall, infile);
      checkRandomIndex(keyPosition, position(infile), len, overall, fileSize);
    } else if (isEssenceElement(k) || isSystemElement(k)) {
      objects.clear();
      markMetadataEnd(keyPosition);
      markIndexEnd(keyPosition);
      markEssenceSegmentStart(currentPartition->_bodySID, keyPosition);
      skipV(len, infile);
    } else if (isIndexSegment(k)) {
      objects.clear();
      primer.clear();
      badids.clear();
      markMetadataEnd(keyPosition);
      markIndexStart(currentPartition->_indexSID, keyPosition);
      validateIndexSegment(k, len, infile);
    } else if (isFill(k)) {
      skipV(len, infile);
      markFill(keyPosition, position(infile));
      checkFill(k, keyPosition, previousKey);
    } else if (isLocalSet(k)) {
      validateObject(k, len, infile);
    } else {
      skipV(len, infile);
    }
  }
  objects.clear();
  primer.clear();
  badids.clear();
  markMetadataEnd(fileSize);
  markIndexEnd(fileSize);

  if (footer == 0) {
    mxfError("No footer found.\n");
  }
  checkPartitions(p, footer);

  if (!rip._index.empty()) {
    checkRandomIndex(rip, p);
    if (debug) {
      printRandomIndex(rip);
    }
  }

  if (debug) {
    printPartitions(p);
    printStreams(streams);
  }
  destroyPartitions(p);
  destroyStreams(streams);

  fprintf(stderr,
          "%s : MXF validation       - ",
          programName());
  if (errors == 0) {
    fprintf(stderr, "passed.\n");
  } else {
    fprintf(stderr, "failed.\n");
  }
}

void aafValidate(mxfFile infile);

void aafValidate(mxfFile /* infile */)
{
  fprintf(stderr,
          "%s : AAF validation       - not yet implemented.\n",
          programName());
}

void mxfValidateFile(Mode mode, mxfFile infile);

void mxfValidateFile(Mode mode, mxfFile infile)
{
  switch (mode) {
  case aafValidateMode:
    klvValidate(infile);
    setPosition(infile, headerPosition);
    setValidate(infile);
    setPosition(infile, headerPosition);
    mxfValidate(infile);
    setPosition(infile, headerPosition);
    aafValidate(infile);
    break;
  case mxfValidateMode:
    klvValidate(infile);
    setPosition(infile, headerPosition);
    setValidate(infile);
    setPosition(infile, headerPosition);
    mxfValidate(infile);
    break;
  case setValidateMode:
    klvValidate(infile);
    setPosition(infile, headerPosition);
    setValidate(infile);
    break;
  case klvValidateMode:
    klvValidate(infile);
    break;
  default:
    /* Invalid mode ? */
    break;
  }
}

void setMode(Mode m);

void setMode(Mode m)
{
  if (mode != unspecifiedMode) {
    error("Specify only one of "
          "--klv-dump, --set-dump, --mxf-dump, --aaf-dump, "
          "--aaf-validate, --mxf-validate, --set-validate, --klv-validate.\n");
    printUsage();
    exit(EXIT_FAILURE);
  }
  mode = m;
}

bool isDumpMode(Mode mode);

bool isDumpMode(Mode mode)
{
  bool result = false;
  if ((mode == klvMode) ||
      (mode == localSetMode) ||
      (mode == mxfMode) ||
      (mode == aafMode)) {
    result = true;
  }
  return result;
}

void checkDumpMode(const char* option);

void checkDumpMode(const char* option)
{
  if (mode == unspecifiedMode) {
    mode = mxfMode;
  }
  if (!isDumpMode(mode)) {
    error("%s not valid with "
          "--aaf-validate, --mxf-validate, --set-validate, --klv-validate.\n",
          option);
    printUsage();
    exit(EXIT_FAILURE);
  }
}

bool getInteger(int& i, char* s);

bool getInteger(int& i, char* s)
{
  bool result;
  char* expectedEnd = &s[strlen(s)];
  char* end;
  long b = strtoul(s, &end, 10);
  if (end != expectedEnd || b > INT_MAX || b < INT_MIN) {
    result = false;
  } else {
    i = (int)b;
    result = true;
  }
  return result;
}

int getIntegerOption(int currentArgument,
                     int argumentCount,
                     char* argumentVector[],
                     const char* label);

int getIntegerOption(int currentArgument,
                     int argumentCount,
                     char* argumentVector[],
                     const char* label)
{
  char* option = argumentVector[currentArgument];
  int result = 0;
  int optArg = currentArgument + 1;
  if ((optArg < argumentCount) && (*argumentVector[optArg] != '-' )) {
    char* value = argumentVector[optArg];
    if (!getInteger(result, value)) {
      error("\"%s\" is not a valid %s.\n", value, label);
      printUsage();
      exit(EXIT_FAILURE);
    }
  } else {
    error("\"%s\" must be followed by a %s.\n", option, label);
    printUsage();
    exit(EXIT_FAILURE);
  }
  return result;
}

void printSummary(void);

void printSummary(void)
{
  if ((errors != 0) && (warnings != 0)) {
    message("Encountered %" MXFPRIu32 " errors and %" MXFPRIu32 " warnings.\n",
             errors,
             warnings);
  } else if (errors != 0) {
    message("Encountered %" MXFPRIu32 " errors.\n",
             errors);
  } else if (warnings != 0) {
    message("Encountered %" MXFPRIu32 " warnings.\n",
             warnings);
  }
}

int mxfExitStatus(int status)
{
  int result;
  if (errors != 0) {
    result = 2;      // Errors, possibly also warnings
  } else if (warnings != 0) {
    result = 1;      // Warnings only
  } else {
    result = status; // No errors or warnings
  }
  return result;
}

} // unnamed namespece

// Summary of options -
//
// -k --klv-dump
// -s --set-dump
// -m --mxf-dump
// -a --aaf-dump
// -v --verbose
// -f --show-fill
// -w --show-dark
//    --show-run-in
//    --search-run-in
// -e --no-limit-bytes
// -l --limit-bytes
// -c --limit-entries
// -p --frames
// -i --limit-frames
//    --no-limit-frames
//    --diff-friendly
// -j --key-addresses
//    --no-key-addresses
// -r --relative
// -b --absolute
// -t --decimal
// -x --hexadecimal
// -y --symbolic
// -n --no-symbolic
// -h --help
// -d --debug
// -u --show-dark-as-sets
//    --klv-validate
//    --set-validate
//    --mxf-validate
//    --aaf-validate
//    --statistics
//    --ignore-primer
//    --show-anc
// Free letters - goqz

int main(int argumentCount, char* argumentVector[])
{
#if defined(MXF_USE_CONSOLE)
  argumentCount = ccommand(&argumentVector);
#endif
  atexit(printSummary);
  setProgramName(argumentVector[0]);
  checkSizes();
  initAAFKeyTable();
  initAAFLocalKeyTable();
  initEssenceContainerLabelMap();
  int fileCount = 0;
  int fileArg = 0;
  char* p = 0;
  for (int i = 1; i < argumentCount; i++) {
    p = argumentVector[i];
    if ((strcmp(p, "--klv-dump") == 0) ||
        (strcmp(p, "-k") == 0)) {
      setMode(klvMode);
    } else if ((strcmp(p, "--set-dump") == 0) ||
               (strcmp(p, "-s") == 0)) {
      setMode(localSetMode);
    } else if ((strcmp(p, "--mxf-dump") == 0) ||
               (strcmp(p, "-m") == 0)) {
      setMode(mxfMode);
    } else if ((strcmp(p, "--aaf-dump") == 0) ||
               (strcmp(p, "-a") == 0)) {
      setMode(aafMode);
    } else if (strcmp(p, "--klv-validate") == 0) {
      setMode(klvValidateMode);
    } else if (strcmp(p, "--set-validate") == 0) {
      setMode(setValidateMode);
    } else if (strcmp(p, "--mxf-validate") == 0) {
      setMode(mxfValidateMode);
    } else if (strcmp(p, "--aaf-validate") == 0) {
      setMode(aafValidateMode);
    } else if ((strcmp(p, "--verbose") == 0) ||
               (strcmp(p, "-v") == 0)) {
      verbose = true;
    } else if ((strcmp(p, "--show-fill") == 0) ||
               (strcmp(p, "-f") == 0)) {
      checkDumpMode(p);
      dumpFill = true;
    } else if ((strcmp(p, "--show-dark") == 0) ||
               (strcmp(p, "-w") == 0)) {
      checkDumpMode(p);
      dumpDark = true;
    } else if (strcmp(p, "--show-run-in") == 0) {
      checkDumpMode(p);
      dumpRunIn = true;
    } else if (strcmp(p, "--show-anc") == 0) {
      checkDumpMode(p);
      dumpANC = true;
    } else if (strcmp(p, "--ignore-primer") == 0) {
      checkDumpMode(p);
      usePrimer = false;
    } else if (strcmp(p, "--search-run-in") == 0) {
      runInLimit = getIntegerOption(i,
                                    argumentCount,
                                    argumentVector,
                                    "byte count");
      i = i + 1;
    } else if ((strcmp(p, "--no-limit-bytes") == 0) ||
               (strcmp(p, "-e") == 0)) {
      checkDumpMode(p);
      lFlag = true;
      limitBytes = false;
    } else if ((strcmp(p, "--limit-bytes") == 0) ||
               (strcmp(p, "-l") == 0)) {
      checkDumpMode(p);
      lFlag = true;
      limitBytes = true;
      limit = getIntegerOption(i, argumentCount, argumentVector, "byte count");

      i = i + 1;
    } else if ((strcmp(p, "--limit-entries") == 0) ||
               (strcmp(p, "-c") == 0)) {
      checkDumpMode(p);
      maxIndex = getIntegerOption(i, argumentCount, argumentVector, "count");
      cFlag = true;
      i = i + 1;
    } else if (strcmp(p, "--no-limit-entries") == 0) {
      checkDumpMode(p);
      cFlag = false;
    } else if ((strcmp(p, "--frames") == 0) ||
               (strcmp(p, "-p") == 0)) {
      checkDumpMode(p);
      frames = true;
    } else if ((strcmp(p, "--limit-frames") == 0) ||
               (strcmp(p, "-i") == 0)) {
      checkDumpMode(p);
      maxFrames = getIntegerOption(i, argumentCount, argumentVector, "count");
      iFlag = true;
      i = i + 1;
    } else if (strcmp(p, "--no-limit-frames") == 0) {
      checkDumpMode(p);
      iFlag = false;
      i = i + 1;
    } else if (strcmp(p, "--diff-friendly") == 0) {
      checkDumpMode(p);
      diffFriendly = true;
    } else if ((strcmp(p, "--key-addresses") == 0) ||
               (strcmp(p, "-j") == 0)) {
      checkDumpMode(p);
      keyAddresses = true;
    } else if (strcmp(p, "--no-key-addresses") == 0) {
      checkDumpMode(p);
      keyAddresses = false;
    } else if ((strcmp(p, "--relative") == 0) ||
               (strcmp(p, "-r") == 0)) {
      checkDumpMode(p);
      relative = true;
    } else if ((strcmp(p, "--absolute") == 0) ||
               (strcmp(p, "-b") == 0)) {
      checkDumpMode(p);
      relative = false;
    } else if ((strcmp(p, "--decimal") == 0) ||
               (strcmp(p, "-t") == 0)) {
      checkDumpMode(p);
      base = 10;
    } else if ((strcmp(p, "--hexadecimal") == 0) ||
               (strcmp(p, "-x") == 0)) {
      checkDumpMode(p);
      base = 16;
    } else if ((strcmp(p, "--symbolic") == 0) ||
               (strcmp(p, "-y") == 0)) {
      checkDumpMode(p);
      symbolic = true;
    } else if ((strcmp(p, "--no-symbolic") == 0) ||
               (strcmp(p, "-n") == 0)) {
      checkDumpMode(p);
      symbolic = false;
    } else if ((strcmp(p, "-u") == 0) ||
               (strcmp(p, "--show-dark-as-sets") == 0)) {
      checkDumpMode(p);
      darkKeysAsSets = true;
      dumpDark = true;
    } else if (strcmp(p, "--statistics") == 0) {
      checkDumpMode(p);
      printStats = true;
    } else if ((strcmp(p, "--help") == 0) ||
               (strcmp(p, "-h") == 0)) {
      hFlag = true;
    } else if ((strcmp(p, "--debug") == 0) ||
               (strcmp(p, "-d") == 0)) {
      debug = true;
    } else if (*p == '-') {
      error("Invalid option \"%s\".\n", p);
      printUsage();
      exit(EXIT_FAILURE);
    } else {
      fileCount = fileCount + 1;
      fileArg = i;
    }
  }
  if (debug) {
    dumpKeyTable();
    dumpAAFKeyTable();

    checkKeyTable();
    checkLocalKeyTable();
    checkAAFKeyTable();
    checkAAFLocalKeyTable();
  }
  if (hFlag) {
    printHelp();
    exit(EXIT_SUCCESS);
  }
  if (mode == unspecifiedMode) {
    mode = mxfMode;
  }

  // Apply --limit-bytes default
  if (!lFlag) {
    if (mode == klvMode) {
      limitBytes = true;
      limit = 0;
    } else if (mode == localSetMode) {
      limitBytes = true;
      limit = 0;
    } else if (mode == mxfMode) {
      limitBytes = true;
      limit = 0;
    } else if (mode == aafMode) {
      limitBytes = true;
      limit = 0;
    }
  }

  if (mode == klvMode) {
    if (dumpFill) {
      error("--show-fill not valid with --klv-dump.\n");
      printUsage();
      exit(EXIT_FAILURE);
    }
  } else if (mode == localSetMode) {
    // No checks here yet
  } else if (mode == mxfMode) {
    // No checks here yet
  } else if (mode == aafMode) {
    // No checks here yet
  }
  int expectedFiles = 1;
  if (fileCount != expectedFiles) {
    error("Wrong number of arguments (%d).\n", fileCount);
    printUsage();
    exit(EXIT_FAILURE);
  }
  char* fileName = argumentVector[fileArg];
  if (debug) {
    fprintf(stdout, "file = %s\n", fileName);
  }
  if (verbose) {
    if (limitBytes) {
      fprintf(stdout,
              "dumping only the first ");
      printDecField(stdout, limit);
      fprintf(stdout,
              " bytes of values.\n");
    }
  }

  mxfFile infile = openExistingRead(fileName);
  if (infile == 0) {
    fatalError("File \"%s\" not found.\n", fileName);
  }

  if (!isMxfFile(infile, headerPosition)) {
    if (runInLimit < 64 * 1024) {
      // We searched less than 64k bytes
      warning("Searched only %" MXFPRIu32 " bytes of run-in.\n", runInLimit);
    }
    if ((mode == klvMode) || (mode == klvValidateMode)) {
      warning("File \"%s\" is not an MXF file.\n", fileName);
    } else {
      fatalError("File \"%s\" is not an MXF file.\n", fileName);
    }
  } else if (headerPosition > 64 * 1024) {
    // We found the header but the run-in was too big
    mxfError("Run-in too large [ %" MXFPRIu64 " bytes ].\n", headerPosition);
  }
  if (isDumpMode(mode)) {
    if (headerPosition != 0) {
      if (dumpRunIn) {
        printRunIn(headerPosition, limitBytes, limit, infile);
      } else {
        fprintf(stdout, "\n");
        fprintf(stdout,
                "[ Omitted %" MXFPRIu64 " bytes of run-in ]\n",
                headerPosition);
      }
    }
  }
  setPosition(infile, headerPosition);

  if (mode == klvMode) {
    klvDumpFile(infile);
  } else if (mode == localSetMode) {
    setDumpFile(infile);
  } else if (mode == mxfMode) {
    mxfDumpFile(infile);
  } else if (mode == aafMode) {
    aafDumpFile(infile);
  } else if (mode == klvValidateMode) {
    mxfValidateFile(mode, infile);
  } else if (mode == setValidateMode) {
    mxfValidateFile(mode, infile);
  } else if (mode == mxfValidateMode) {
    mxfValidateFile(mode, infile);
  } else if (mode == aafValidateMode) {
    mxfValidateFile(mode, infile);
  }
  closeFile(infile);

  return mxfExitStatus(EXIT_SUCCESS);
}
