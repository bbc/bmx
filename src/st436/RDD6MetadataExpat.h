/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#ifndef BMX_RDD6_METADATA_EXPAT_H_
#define BMX_RDD6_METADATA_EXPAT_H_

#include <map>

#include <expat.h>

#include <bmx/st436/RDD6Metadata.h>



namespace bmx
{



class ExpatHandler
{
public:
    ExpatHandler();
    virtual ~ExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts) = 0;
    virtual bool EndElement(const std::string &ns, const std::string &name) = 0;
    virtual void CharacterData(const char *s, int len) = 0;

    std::string GetNamespace() const { return mNamespace; }
    std::string GetName() const      { return mName; }

    bool ElementNSEquals(const std::string &ns, const std::string &left, const std::string &right) const;

    bool HaveAttribute(const std::string &name, const char **atts);
    std::string GetAttribute(const std::string &name, const char **atts);

    virtual std::string GetCData()   { return ""; }

protected:
    std::string mNamespace;
    std::string mName;
};


class IgnoreExpatHandler : public ExpatHandler
{
public:
    IgnoreExpatHandler();
    virtual ~IgnoreExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
    virtual void CharacterData(const char *s, int len);

private:
    int mLevel;
};


class SimpleTextExpatHandler : public ExpatHandler
{
public:
    SimpleTextExpatHandler();
    virtual ~SimpleTextExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
    virtual void CharacterData(const char *s, int len);

    virtual std::string GetCData() { return mCData; }

protected:
    std::string mCData;
};


class SimpleChildrenExpatHandler : public ExpatHandler
{
public:
    SimpleChildrenExpatHandler();
    virtual ~SimpleChildrenExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
    virtual void CharacterData(const char *s, int len);

    bool HaveChild(const std::string &name) const;
    const std::string& GetChildStr(const std::string &name) const;

protected:
    ExpatHandler *mChildHandler;
    std::map<std::string, std::string> mChildren;
};



class DataSegmentHandler
{
public:
    DataSegmentHandler();
    virtual ~DataSegmentHandler();

    void CompletePayload(RDD6ParsedPayload *payload);

    RDD6DataSegment* TakeDataSegment();

protected:
    RDD6DataSegment *mDataSegment;
};


class DolbyECompleteExpatHandler;

class DescriptionTextExpatHandler : public ExpatHandler
{
public:
    DescriptionTextExpatHandler(RDD6DolbyEComplete *payload);
    virtual ~DescriptionTextExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
    virtual void CharacterData(const char *s, int len);

private:
    RDD6DolbyEComplete *mPayload;
    uint8_t mProgramCount;
    uint8_t mNextProgram;
    ExpatHandler *mChildHandler;
};

class DolbyECompleteExpatHandler : public DataSegmentHandler, public ExpatHandler
{
public:
    DolbyECompleteExpatHandler();
    virtual ~DolbyECompleteExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
    virtual void CharacterData(const char *s, int len);

private:
    RDD6DolbyEComplete *mPayload;
    ExpatHandler *mChildHandler;
};


class DolbyEEssentialExpatHandler : public DataSegmentHandler, public SimpleChildrenExpatHandler
{
public:
    DolbyEEssentialExpatHandler();
    virtual ~DolbyEEssentialExpatHandler();

    virtual bool EndElement(const std::string &ns, const std::string &name);

private:
    RDD6DolbyEEssential *mPayload;
};


class DolbyDigitalCompleteExtBSIExpatHandler : public DataSegmentHandler, public SimpleChildrenExpatHandler
{
public:
    DolbyDigitalCompleteExtBSIExpatHandler();
    virtual ~DolbyDigitalCompleteExtBSIExpatHandler();

    virtual bool EndElement(const std::string &ns, const std::string &name);

private:
    RDD6DolbyDigitalCompleteExtBSI *mPayload;
};


class DolbyDigitalEssentialExtBSIExpatHandler : public DataSegmentHandler, public SimpleChildrenExpatHandler
{
public:
    DolbyDigitalEssentialExtBSIExpatHandler();
    virtual ~DolbyDigitalEssentialExtBSIExpatHandler();

    virtual bool EndElement(const std::string &ns, const std::string &name);

private:
    RDD6DolbyDigitalEssentialExtBSI *mPayload;
};


class DolbyDigitalCompleteExpatHandler : public DataSegmentHandler, public SimpleChildrenExpatHandler
{
public:
    DolbyDigitalCompleteExpatHandler();
    virtual ~DolbyDigitalCompleteExpatHandler();

    virtual bool EndElement(const std::string &ns, const std::string &name);

private:
    RDD6DolbyDigitalComplete *mPayload;
};


class DolbyDigitalEssentialExpatHandler : public DataSegmentHandler, public SimpleChildrenExpatHandler
{
public:
    DolbyDigitalEssentialExpatHandler();
    virtual ~DolbyDigitalEssentialExpatHandler();

    virtual bool EndElement(const std::string &ns, const std::string &name);

private:
    RDD6DolbyDigitalEssential *mPayload;
};


class RawDataSegmentExpatHandler : public DataSegmentHandler, public SimpleTextExpatHandler
{
public:
    RawDataSegmentExpatHandler();
    virtual ~RawDataSegmentExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
};


class SyncExpatHandler : public SimpleChildrenExpatHandler
{
public:
    SyncExpatHandler(RDD6SyncSegment *sync_segment);
    virtual ~SyncExpatHandler();

    virtual bool EndElement(const std::string &ns, const std::string &name);

private:
    RDD6SyncSegment *mSyncSegment;
};


class SubFrameExpatHandler : public ExpatHandler
{
public:
    SubFrameExpatHandler(bool is_first);
    virtual ~SubFrameExpatHandler();

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
    virtual void CharacterData(const char *s, int len);

    RDD6MetadataSubFrame* TakeSubFrame();

private:
    RDD6MetadataSubFrame *mSubFrame;
    ExpatHandler *mChildHandler;
};


class RDD6MetadataExpat : public ExpatHandler
{
public:
    RDD6MetadataExpat(RDD6MetadataFrame *rdd6_frame);
    virtual ~RDD6MetadataExpat();

    bool ParseXML(const std::string &filename);

    virtual void StartElement(const std::string &ns, const std::string &name, const char **atts);
    virtual bool EndElement(const std::string &ns, const std::string &name);
    virtual void CharacterData(const char *s, int len);

private:
    RDD6MetadataFrame *mRDD6Frame;
    XML_Parser mParser;
    ExpatHandler *mChildHandler;
};


};



#endif

