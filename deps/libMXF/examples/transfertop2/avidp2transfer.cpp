/*
 * Reads an Avid AAF composition file and transfers referenced MXF files to P2
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the British Broadcasting Corporation nor the names
 *       of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cwchar>
#include <cstdarg>

#include <AAF.h>
#include <AAFTypes.h>
#include <AAFResult.h>
#include <AAFClassDefUIDs.h>
#include <AAFFileKinds.h>
#include <AAFSmartPointer.h>
#include <AAFDataDefs.h>

#include <vector>
using namespace std;

#include "avidp2transfer.h"

#include <mxf/mxf_logging.h>
#include <mxf/mxf_macros.h>



#define APT_ERROR(message) \
    mxf_log_error(message LOG_LOC_FORMAT, LOG_LOC_PARAMS); \
    throw APTException(message "\n");

#define APT_ERROR_1(message, arg1) \
    mxf_log_error(message LOG_LOC_FORMAT, arg1, LOG_LOC_PARAMS); \
    throw APTException(message "\n", arg1);

#define AAF_CHECK(cmd, message) \
    if (!AAFRESULT_SUCCEEDED(cmd)) \
    { \
        APT_ERROR(message); \
    }

#define AAF_CHECK_1(cmd, message, arg1) \
    if (!AAFRESULT_SUCCEEDED(cmd)) \
    { \
        APT_ERROR_1(message, arg1); \
    }

#define APT_WARNING(message) \
    mxf_log_warn(message LOG_LOC_FORMAT, LOG_LOC_PARAMS);



// These routines rely upon URLs as defined in RFC 1738:
//
// <scheme>:<scheme-specific-part>
// ; the scheme is in lower case; interpreters should use case-ignore
//	scheme		= 1*[ lowalpha | digit | "+" | "-" | "." ]
//
// For file scheme:
//	fileurl		= "file://" [ host | "localhost" ] "/" fpath
//
// NB. ':' is acceptable unescaped in the fpath component

#ifdef _MSC_VER			// MS VC++ dosen't provide POSIX strncasecmp
#define strncasecmp(s1, s2, n) strnicmp(s1, s2, n)
#else
#include <strings.h>	// strncasecmp()
#endif

static char asciiHexToChar (char c)
{
    return  c >= '0' && c <= '9' ?  c - '0'
    	    : c >= 'A' && c <= 'F'? c - 'A' + 10
    	    : c - 'a' + 10;	/* accept lowercase letters too */
}

static void unescapeURI(char *str)
{
    char *p = str;
    char *q = str;

    while (*p)
	{
        if (*p == '%')		// URI hex escape char
		{
			p++;
			if (*p)
				*q = asciiHexToChar(*p++) * 16;
			if (*p)
				*q = *q + asciiHexToChar(*p);
			p++;
			q++;
		}
		else
		    *q++ = *p++;
    }
    *q++ = 0;
}

/************************
 * Function: wcsconvertURLtoFilepath
 *
 *	Converts a file scheme URL into an absolute filepath.  An invalid URL is
 *	returned unmodified.
 *
 * Argument Notes:
 *		filepath must have space allocated by the caller
 *
 * ReturnValue:
 *		void
 *
 * Possible Errors:
 *		none
 */
static void wcsconvertURLtoFilepath(wchar_t *url, wchar_t *filepath)
{
	// Convert to char *for ease of processing.
	// (wcsncasecmp and similiar are not available everywhere)
	//unsigned tlen = wcslen(url);
	size_t tlen = wcslen(url);
	char *tmp = new char[tlen+1];		// +1 includes terminating '\0'
	wcstombs(tmp, url, tlen+1);

	// If no file scheme is found, assume a simple filepath and not a URI.
	// Note that other schemes such as http and ftp are not supported.
	if (strncasecmp(tmp, "file://", strlen("file://")) != 0)
	{
		wcscpy(filepath, url);
		delete [] tmp;
		return;
	}

	// Skip over the file://[host]/ to point to the fpath.
	char *fpath = tmp + strlen("file://");
	while (*fpath && *fpath != '/')
		fpath++;

#ifdef _WIN32
	// WIN32 filepaths must start with a drive letter, so remove the
	// initial '/' from the URL.
	if (*fpath == '/')
		fpath++;
#endif

	unescapeURI(fpath);

	mbstowcs(filepath, fpath, strlen(fpath)+1);		// convert back to wcs
	delete [] tmp;
}


static string toUTF8(vector<aafCharacter> utf16Str)
{
    string utf8Str;
    utf8Str.reserve(utf16Str.size());

    std::vector<aafCharacter>::const_iterator iter;
    for (iter = utf16Str.begin(); iter != utf16Str.end(); iter++)
    {
        if ((*iter) < 0x80)
        {
            utf8Str += (char)((*iter));
        }
        else if ((*iter) < 0x800)
        {
            utf8Str += (char)(0xC0 | ((*iter) >> 6));
            utf8Str += (char)(0x80 | ((*iter) & 0x3F));
        }
        else if ((*iter) < 0xD800 || (*iter) > 0xDFFF)
        {
            utf8Str += (char)(0xE0 | ((*iter) >> 12));
            utf8Str += (char)(0x80 | (((*iter) >> 6) & 0x3F));
            utf8Str += (char)(0x80 | ((*iter) & 0x3F));
        }
        else if (((*iter) & 0xFC00) == 0xD800)
        {
            aafCharacter aafc1 = *iter;
            iter++;
            if (iter == utf16Str.end())
            {
                APT_ERROR("Invalid UTF16 string");
            }
            aafCharacter aafc2 = *iter;

            if ((aafc2 & 0xFC00) == 0xD900)
            {
                unsigned long int c = (aafc1 & 0x03FF) << 10;
                c |= (aafc2 & 0x03FF);
                c += 0x10000;
                utf8Str += (char)(0xF0 | (c >> 18));
                utf8Str += (char)(0x80 | ((c >> 12) & 0x3F));
                utf8Str += (char)(0x80 | ((c >> 6) & 0x3F));
                utf8Str += (char)(0x80 | (c & 0x3F));
            }
            else
            {
                APT_ERROR("Invalid UTF16 string");
            }
        }
        else
        {
            APT_ERROR("Invalid UTF16 string");
        }
    }

    return utf8Str;
}


APTException::APTException(const char *format, ...)
{
    char message[512];

    va_list p_arg;
    va_start(p_arg, format);
    mxf_vsnprintf(message, 512, format, p_arg);
    va_end(p_arg);

    _message = message;
}


APTException::~APTException()
{}

const char *APTException::getMessage()
{
    return _message.c_str();
}



AvidP2Transfer::AvidP2Transfer(const char *aafFilename, progress_callback progress,
    insert_timecode_callback insertTimecode,
    const char *filepathPrefix, bool omitDriveColon)
: totalStorageSizeEstimate(0), _transfer(NULL), _readyToTransfer(false),
_omitDriveColon(omitDriveColon)
{
    char *inputFilenames[17];
    unsigned int i;

    if (filepathPrefix != NULL)
    {
        _filepathPrefix = filepathPrefix;
    }

    processAvidComposition(aafFilename);
    mxf_log_debug("Number of tracks extracted from '%s' = %d\n", aafFilename, trackInfo.size());

    vector<APTTrackInfo>::const_iterator iter;
    for (i = 0, iter = trackInfo.begin(); iter != trackInfo.end(); iter++, i++)
    {
        inputFilenames[i] = new char[(*iter).mxfFilename.length() + 1];
        strcpy(inputFilenames[i], (*iter).mxfFilename.c_str());
    }

    if (!prepare_transfer(inputFilenames, (int)trackInfo.size(),
        _timecodeStart, _timecodeDrop ? 1 : 0,
        progress, insertTimecode, &_transfer))
    {
        for (i = 0; i < trackInfo.size(); i++)
        {
            delete [] inputFilenames[i];
        }
        throw APTException("Failed to prepare transfer");
    }

    totalStorageSizeEstimate = _transfer->totalStorageSizeEstimate;

    for (i = 0; i < trackInfo.size(); i++)
    {
        delete [] inputFilenames[i];
    }

    _readyToTransfer = true;
}

AvidP2Transfer::~AvidP2Transfer()
{
    free_transfer(&_transfer);
}

bool AvidP2Transfer::transferToP2(const char *p2path)
{
    int isComplete;

    if (!_readyToTransfer)
    {
        throw APTException("Transfer is not in the ready state");
    }

    if (!transfer_avid_mxf_to_p2(p2path, _transfer, &isComplete))
    {
        throw APTException("Failed to transfer");
    }
    clipName = _transfer->clipName;

    return isComplete != 0;
}


void AvidP2Transfer::processAvidComposition(const char *filename)
{
    aafCharacter *inFilename = 0;
    aafUInt32 count;
    aafUInt32 sizeInBytes;
    aafUInt32 sizeInChars;
    aafUInt8* buffer1 = 0;
    aafUInt8* buffer2 = 0;
    char *buffer3 = 0;
    IAAFSmartPointer<IAAFFile> pInput;
    IAAFSmartPointer<IAAFHeader> pHeader;
    IAAFSmartPointer<IEnumAAFMobs> pMobs;
    IAAFSmartPointer<IAAFMob> pMob;
    IAAFSmartPointer<IEnumAAFMobSlots> pSlots;
    IAAFSmartPointer<IAAFMobSlot> pSlot;
    IAAFSmartPointer<IAAFTimelineMobSlot> pTimelineMobSlot;
    IAAFSmartPointer<IAAFDataDef> pDataDef;
    IAAFSmartPointer<IAAFSegment> pSegment;
    IAAFSmartPointer<IAAFDefObject> pDefObject;
    IAAFSmartPointer<IAAFTimecode> pTimecode;
    IAAFSmartPointer<IAAFSourceClip> pSourceClip;
    IAAFSmartPointer<IAAFSequence> pSequence;
    IAAFSmartPointer<IAAFNestedScope> pNestedScope;
    IAAFSmartPointer<IAAFComponent> pComponent;
    IAAFSmartPointer<IAAFMob> pMob2;
    IAAFSmartPointer<IAAFMobSlot> pSlot2;
    IAAFSmartPointer<IAAFSegment> pSegment2;
    IAAFSmartPointer<IAAFSourceClip> pSourceClip2;
    IAAFSmartPointer<IAAFSequence> pSequence2;
    IAAFSmartPointer<IAAFComponent> pComponent2;
    IAAFSmartPointer<IAAFMob> pMob3;
    IAAFSmartPointer<IAAFSourceMob> pSourceMob;
    IAAFSmartPointer<IAAFMobSlot> pSlot3;
    IAAFSmartPointer<IAAFSegment> pSegment3;
    IAAFSmartPointer<IAAFComponent> pComponent3;
    IAAFSmartPointer<IAAFEssenceDescriptor> pEssenceDescriptor;
    IAAFSmartPointer<IAAFLocator> pLocator;
    aafSourceRef_t sourceRef;
    aafSourceRef_t sourceRef2;
    aafLength_t trackLength;
    aafBoolean_t isPictureKind;
    aafBoolean_t isSoundKind;
    aafRational_t editRate;
    aafTimecode_t timecode;
    aafUID_t auid;
    int i;
    bool foundSourceClip;

    _timecodeStart = 0;
    _timecodeDrop = false;
    _timecodeFPS = 0;

    try
    {
        // open Avid AAF file

        inFilename = new aafCharacter[strlen(filename) + 1];
        size_t status = mbstowcs(inFilename, filename, strlen(filename) + 1);
        if (status == (size_t)-1)
        {
            APT_ERROR_1("Failed to convert filename '%s' to a wide string.", filename);
        }
        AAF_CHECK_1(AAFFileOpenExistingRead(inFilename, 0, &pInput), "Failed to open AAF file '%s'\n", filename);
        delete [] inFilename;
        inFilename = 0;
    }
    catch (APTException&)
    {
        if (inFilename != 0)
        {
            delete [] inFilename;
            inFilename = 0;
        }
        throw;
    }
    catch (...)
    {
        if (inFilename != 0)
        {
            delete [] inFilename;
            inFilename = 0;
        }
        throw APTException("Failed to open Avid input file");
    }

    try
    {
        // get the CompositionMob

        AAF_CHECK(pInput->GetHeader(&pHeader), "Failed to get Header");
        aafSearchCrit_t searchCrit;
        searchCrit.searchTag = kAAFByMobKind;
        searchCrit.tags.mobKind = kAAFCompMob;
        AAF_CHECK(pHeader->GetMobs(&searchCrit, &pMobs), "Failed to get CompositionMobs");
        AAF_CHECK(pMobs->NextOne(&pMob), "Failed to get a CompositionMob");


        AAF_CHECK(pMob->GetSlots(&pSlots), "Failed to get CompositionMob MobSlots");
        while (AAFRESULT_SUCCEEDED(pSlots->NextOne(&pSlot)))
        {
            APTTrackInfo singleTrackInfo;

            // get the track type Picture/Sound and process Timecode

            AAF_CHECK(pSlot->GetDataDef(&pDataDef), "Failed to get Segment::DataDef");
            AAF_CHECK(pDataDef->IsPictureKind(&isPictureKind), "Failed to get DataDefinition kind");
            AAF_CHECK(pDataDef->IsSoundKind(&isSoundKind), "Failed to get DataDefinition kind");
            if (isPictureKind == kAAFTrue)
            {
                singleTrackInfo.isPicture = true;
            }
            else if (isSoundKind == kAAFTrue)
            {
                singleTrackInfo.isPicture = false;
            }
            else
            {
                AAF_CHECK(pDataDef->QueryInterface(IID_IAAFDefObject, (void **)&pDefObject),
                    "Failed to get DefObject interface from DataDefinition object");
                AAF_CHECK(pDefObject->GetAUID(&auid), "Failed to get DefObject::Identification");
                if (memcmp(&auid, &kAAFDataDef_Timecode, sizeof(aafUID_t)) == 0 ||
                    memcmp(&auid, &kAAFDataDef_LegacyTimecode, sizeof(aafUID_t)) == 0)
                {
                    // get the Timecode information

                    AAF_CHECK(pSlot->GetSegment(&pSegment), "Failed to get a CompositionMob-Slot-Segment");

                    if (AAFRESULT_SUCCEEDED(pSegment->QueryInterface(IID_IAAFTimecode, (void **)&pTimecode)))
                    {
                        AAF_CHECK(pTimecode->GetTimecode(&timecode), "Failed to get timecode value");
                        _timecodeStart = timecode.startFrame;
                        _timecodeDrop = timecode.drop == kAAFTrue;
                        _timecodeFPS = timecode.fps;
                    }
                    else
                    {
                        APT_WARNING("Expecting a Timecode object referenced directly by the Timecode MobSlot");
                    }
                }

                continue;
            }


            // get the MobSlot::Name

            if (AAFRESULT_SUCCEEDED(pSlot->GetNameBufLen(&sizeInBytes)))
            {
                sizeInChars = sizeInBytes/sizeof(aafCharacter);
                vector<aafCharacter> buffer(sizeInChars);
                if (AAFRESULT_SUCCEEDED(pSlot->GetName(&buffer[0], sizeInBytes)))
                {
                    singleTrackInfo.name = toUTF8(buffer);
                }
                else
                {
                    APT_WARNING("Failed to get MobSlot name after successfully getting the name buffer length");
                }
            }


            // get the CompositionMob track edit rate

            AAF_CHECK(pSlot->QueryInterface(IID_IAAFTimelineMobSlot, (void **)&pTimelineMobSlot),
                "Picture or Sound MobSlot is not a TimelineMobSlot");
            AAF_CHECK(pTimelineMobSlot->GetEditRate(&editRate), "Failed to get TimelineMobSlot::EditRate");
            singleTrackInfo.compositionEditRate.numerator = editRate.numerator;
            singleTrackInfo.compositionEditRate.denominator = editRate.denominator;


            // get the CompositionMob track length

            AAF_CHECK(pSlot->GetSegment(&pSegment), "Failed to get a CompositionMob-Slot-Segment");

            AAF_CHECK(pSegment->QueryInterface(IID_IAAFComponent, (void **)&pComponent),
                "Failed to get Component interface from Segment interface");
            AAF_CHECK(pComponent->GetLength(&trackLength), "Failed to get Component::Length");
            singleTrackInfo.compositionTrackLength = trackLength;


            // get the MasterMob referenced in this Composition MobSlot

            if (AAFRESULT_SUCCEEDED(pSegment->QueryInterface(IID_IAAFSequence, (void **)&pSequence)))
            {
                AAF_CHECK(pSequence->CountComponents(&count), "Failed to get component count in Sequence");
                if (count == 0)
                {
                    APT_ERROR("Empty Sequence");
                }
                else if (count > 1)
                {
                    // get the SourceClip, assuming we have Filler(s) and a SourceClip
                    foundSourceClip = false;
                    for (i = 0; i < (int)count; i++)
                    {
                        AAF_CHECK(pSequence->GetComponentAt(i, &pComponent), "Failed to get sequence component");
                        if (AAFRESULT_SUCCEEDED(pComponent->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip)))
                        {
                            foundSourceClip = true;
                            break;
                        }
                    }
                    if (!foundSourceClip)
                    {
                        APT_WARNING("Multiple components in CompositionMob Slot Sequence, but no SourceClip found");
                        continue;
                    }
                }
                else
                {
                    AAF_CHECK(pSequence->GetComponentAt(0, &pComponent), "Empty sequence");
                    if (!AAFRESULT_SUCCEEDED(pComponent->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip)))
                    {
                        // could be a Filler
                        continue;
                    }
                }
            }
            else if (!AAFRESULT_SUCCEEDED(pSegment->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip)))
            {
                // process simple NestedScope scenario with single source

                if (!AAFRESULT_SUCCEEDED(pSegment->QueryInterface(IID_IAAFNestedScope, (void **)&pNestedScope)))
                {
                    // could be a Filler
                    continue;
                }

                AAF_CHECK(pNestedScope->CountSegments(&count), "Failed to get NestedScope Segment count");
                if (count != 2)
                {
                    APT_WARNING("Found NestedScope segment with <> 2 segments. This scenario is not supported");
                    continue;
                }
                AAF_CHECK(pNestedScope->GetSegmentAt(0, &pSegment), "Failed to get NestedScope Segment");
                if (AAFRESULT_SUCCEEDED(pSegment->QueryInterface(IID_IAAFSequence, (void **)&pSequence)))
                {
                    AAF_CHECK(pSequence->CountComponents(&count), "Failed to get component count in Sequence");
                    if (count == 0)
                    {
                        APT_ERROR("Empty Sequence in NestedScope");
                    }
                    else if (count > 1)
                    {
                        // get the SourceClip, assuming we have Filler(s) and a SourceClip
                        foundSourceClip = false;
                        for (i = 0; i < (int)count; i++)
                        {
                            AAF_CHECK(pSequence->GetComponentAt(i, &pComponent), "Failed to get sequence component");
                            if (AAFRESULT_SUCCEEDED(pComponent->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip)))
                            {
                                foundSourceClip = true;
                                break;
                            }
                        }
                        if (!foundSourceClip)
                        {
                            APT_WARNING("Multiple components in CompositionMob Nested Scope Sequence, but no SourceClip found");
                            continue;
                        }
                    }
                    else
                    {
                        AAF_CHECK(pSequence->GetComponentAt(0, &pComponent), "Empty sequence in Nested Scope");
                        if (!AAFRESULT_SUCCEEDED(pComponent->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip)))
                        {
                            // could be a Filler
                            continue;
                        }
                    }
                }
                else if (!AAFRESULT_SUCCEEDED(pSegment->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip)))
                {
                    // could be a Filler
                    continue;
                }

            }
            AAF_CHECK(pSourceClip->ResolveRef(&pMob2), "Failed to resolve reference to MasterMob");
            AAF_CHECK(pSourceClip->GetSourceReference(&sourceRef), "Failed to get CompositionMob SourceClip SourceReference struct");



            // get the SourceMob referenced in the single MasterMob MobSlot

            AAF_CHECK(pMob2->LookupSlot(sourceRef.sourceSlotID, &pSlot2), "Failed to get SourceMob MobSlot referenced by CompositionMob SourceClip");
            AAF_CHECK(pSlot2->GetSegment(&pSegment2), "Failed to get a MasterMob-Slot-Segment");
            if (AAFRESULT_SUCCEEDED(pSegment2->QueryInterface(IID_IAAFSequence, (void **)&pSequence2)))
            {
                AAF_CHECK(pSequence2->CountComponents(&count), "Failed to get component count in Sequence");
                if (count == 0)
                {
                    APT_ERROR("Empty Sequence");
                }
                else if (count > 1)
                {
                    // get the SourceClip, assuming we have Filler(s) and a SourceClip
                    foundSourceClip = false;
                    for (i = 0; i < (int)count; i++)
                    {
                        AAF_CHECK(pSequence2->GetComponentAt(i, &pComponent2), "Failed to get sequence component");
                        if (AAFRESULT_SUCCEEDED(pComponent2->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip2)))
                        {
                            foundSourceClip = true;
                            break;
                        }
                    }
                    if (!foundSourceClip)
                    {
                        APT_ERROR("Multiple components in MasterMob Slot Sequence, but no SourceClip found");
                    }
                }
                else
                {
                    AAF_CHECK(pSequence2->GetComponentAt(0, &pComponent2), "Empty sequence");
                    AAF_CHECK(pComponent2->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip2),
                        "Expecting a SourceClip referenced by the Sequence");
                }
            }
            else
            {
                AAF_CHECK(pSegment2->QueryInterface(IID_IAAFSourceClip, (void **)&pSourceClip2), "Expecting a SourceClip");
            }
            AAF_CHECK(pSourceClip2->ResolveRef(&pMob3), "Failed to resolve reference to SourceMob");
            AAF_CHECK(pSourceClip2->GetSourceReference(&sourceRef2), "Failed to get MasterMob SourceClip SourceReference struct");
            AAF_CHECK(pMob3->QueryInterface(IID_IAAFSourceMob, (void **)&pSourceMob), "Mob refernced by MasterMob is not a SourceMob");


            // get the SourceMob track edit rate
            AAF_CHECK(pMob3->LookupSlot(sourceRef2.sourceSlotID, &pSlot3), "Failed to get SourceMob MobSlot referenced by MasterMob SourceClip");

            AAF_CHECK(pSlot3->QueryInterface(IID_IAAFTimelineMobSlot, (void **)&pTimelineMobSlot),
                "SourceMob MobSlot is not a TimelineMobSlot");
            AAF_CHECK(pTimelineMobSlot->GetEditRate(&editRate), "Failed to get TimelineMobSlot::EditRate");
            singleTrackInfo.sourceEditRate.numerator = editRate.numerator;
            singleTrackInfo.sourceEditRate.denominator = editRate.denominator;


            // get the SourceMob track length

            AAF_CHECK(pSlot3->GetSegment(&pSegment3), "Failed to get a SourceMob-Slot-Segment");
            AAF_CHECK(pSegment3->QueryInterface(IID_IAAFComponent, (void **)&pComponent3),
                "Failed to get Component interface from Segment interface");
            AAF_CHECK(pComponent3->GetLength(&trackLength), "Failed to get Component::Length");
            singleTrackInfo.sourceTrackLength = trackLength;



            // get the location of the MXF file

            AAF_CHECK(pSourceMob->GetEssenceDescriptor(&pEssenceDescriptor), "Failed to get SourceMob EssenceDescriptor");
            AAF_CHECK(pEssenceDescriptor->GetLocatorAt(0, &pLocator), "Failed to get first Locator in EssenceDescriptor");
            AAF_CHECK(pLocator->GetPathBufLen(&sizeInBytes), "Failed to ge path buffer length in Locator");
            if (sizeInBytes > sizeof(aafCharacter))
            {
                buffer1 = new aafUInt8[sizeInBytes];
                buffer2 = new aafUInt8[sizeInBytes + 128]; // be on safe side
                buffer3 = new char[(sizeInBytes) + 128 / sizeof(aafCharacter)];
                AAF_CHECK(pLocator->GetPath((aafCharacter*)buffer1, sizeInBytes), "Failed to get Locator::Path");
                wcsconvertURLtoFilepath((aafCharacter*)buffer1, (aafCharacter*)buffer2);
                wcstombs(buffer3, (aafCharacter*)buffer2, wcslen((aafCharacter*)buffer2) + 1);
                singleTrackInfo.mxfFilename = rewriteFilepath(buffer3);

                trackInfo.push_back(singleTrackInfo);
                mxf_log_debug("MXF source file located at '%s'\n", singleTrackInfo.mxfFilename.c_str());
                mxf_log_debug("MXF composition track length (%d/%d) %lld\n",
                    singleTrackInfo.compositionEditRate.numerator, singleTrackInfo.compositionEditRate.denominator,
                    singleTrackInfo.compositionTrackLength);
                mxf_log_debug("MXF source track length (%d/%d) %lld\n",
                    singleTrackInfo.sourceEditRate.numerator, singleTrackInfo.sourceEditRate.denominator,
                    singleTrackInfo.sourceTrackLength);

                delete [] buffer1;
                buffer1 = 0;
                delete [] buffer2;
                buffer2 = 0;
                delete [] buffer3;
                buffer3 = 0;
            }
            else
            {
                APT_ERROR("Locator::Path value is empty");
            }
        }

        pInput->Close();

    }
    catch (APTException&)
    {
        pInput->Close();
        if (buffer1 != 0)
        {
            delete [] buffer1;
            buffer1 = 0;
        }
        if (buffer2 != 0)
        {
            delete [] buffer2;
            buffer2 = 0;
        }
        if (buffer3 != 0)
        {
            delete [] buffer3;
            buffer3 = 0;
        }
        throw;
    }
    catch (...)
    {
        pInput->Close();
        if (buffer1 != 0)
        {
            delete [] buffer1;
            buffer1 = 0;
        }
        if (buffer2 != 0)
        {
            delete [] buffer2;
            buffer2 = 0;
        }
        if (buffer3 != 0)
        {
            delete [] buffer3;
            buffer3 = 0;
        }
        throw APTException("Failed to process Avid input file");
    }

}


string AvidP2Transfer::rewriteFilepath(string filepath)
{
    string fp = filepath;
    // colon is omitted in paths starting with '/X:' or 'X:'
    if (_omitDriveColon)
    {
        size_t i;
        for (i = 0; i < fp.length() && i < 3; i++)
        {
            if (fp[i] == '/')
            {
                if (i != 0)
                {
                    break;
                }
            }
            else if ((fp[i] == ':' &&
                i == 2 && fp[0] == '/' && isalpha(fp[i-1])) ||
                (i == 1 && isalpha(fp[i-1])))
            {
                fp.erase(i, 1);
                break;
            }
        }
    }
    if (_filepathPrefix.length() > 0)
    {
        return _filepathPrefix + fp;
    }
    else
    {
        return fp;
    }
}

