/*
 * Test libMXF++
 *
 * Copyright (C) 2008, British Broadcasting Corporation
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

#include <libMXF++/MXF.h>
#include <memory>
#include <cstdio>

using namespace std;
using namespace mxfpp;


static const char TEST_WRITE_FILENAME[] = "write_test.mxf";



static void testWrite()
{
    // open file
    unique_ptr<File> file(File::openNew(TEST_WRITE_FILENAME));
    file->setMinLLen(4);


    // header partition
    Partition& headerPartition = file->createPartition();
    headerPartition.setKey(&MXF_PP_K(ClosedComplete, Header));
    headerPartition.setVersion(1, 2);
    headerPartition.setKagSize(0x100);
    headerPartition.setOperationalPattern(&MXF_OP_L(atom, NTracks_1SourceClip));
    headerPartition.addEssenceContainer(&MXF_EC_L(DVBased_50_625_50_ClipWrapped));

    headerPartition.write(file.get());


    // header metadata
    unique_ptr<DataModel> dataModel(new DataModel());
    unique_ptr<AvidHeaderMetadata> headerMetadata(new AvidHeaderMetadata(dataModel.get()));

    headerMetadata->createDefaultMetaDictionary();

    Preface *preface = new Preface(headerMetadata.get());
    preface->setVersion(MXF_PREFACE_VER(1, 2));
    preface->appendIdentifications(new Identification(headerMetadata.get()));
    preface->getIdentifications()[0]->setCompanyName("a company");
    preface->setEssenceContainers(vector<mxfUL>());
    preface->setContentStorage(new ContentStorage(headerMetadata.get()));
    // etc.

    headerMetadata->createDefaultDictionary(preface);

    headerMetadata->write(file.get(), &headerPartition, 0);


    // body partition
    Partition &bodyPartition = file->createPartition();
    bodyPartition.setKey(&MXF_PP_K(ClosedComplete, Body));

    bodyPartition.write(file.get());

    // essence data


    // footer partition
    Partition &footerPartition = file->createPartition();
    footerPartition.setKey(&MXF_PP_K(ClosedComplete, Footer));

    footerPartition.write(file.get());


    // RIP
    file->writeRIP();

    // update partitions
    file->updatePartitions();
}

static void testRead(string filename)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    unique_ptr<File> file(File::openRead(filename));
    unique_ptr<DataModel> dataModel(new DataModel());
    unique_ptr<AvidHeaderMetadata> headerMetadata(new AvidHeaderMetadata(dataModel.get()));


    // read header partition
    if (!file->readHeaderPartition())
    {
        throw "Could not find header partition";
    }
    Partition &headerPartition = file->getPartition(0);


    // read header metadata
    file->readNextNonFillerKL(&key, &llen, &len);
    if (HeaderMetadata::isHeaderMetadata(&key))
    {
        headerMetadata->read(file.get(), &headerPartition, &key, llen, len);
        printf("Read header metadata\n");
    }
    else
    {
        throw "Could not find header metadata in header partition";
    }

    Preface *preface = headerMetadata->getPreface();
    preface->getInstanceUID();
    if (preface->haveGenerationUID())
    {
        preface->getGenerationUID();
    }
    printf("Preface::Version = %u\n", preface->getVersion());
    printf("size Preface::Identifications = %d\n", (int)preface->getIdentifications().size());
    printf("size Preface::EssenceContainers = %d\n", (int)preface->getEssenceContainers().size());

    if (preface->getIdentifications().size() > 0)
    {
        Identification *identification = *preface->getIdentifications().begin();
        printf("Identification::CompanyName = '%s'\n", identification->getCompanyName().c_str());
    }

    ContentStorage *storage = preface->getContentStorage();
    if (storage->haveEssenceContainerData())
    {
        storage->getEssenceContainerData();
    }

    // read remaining KLVs
    printf("Reading remaining KLVs\n");
    while (1)
    {
        if (file->tell() >= file->size())
            break;

        file->readNextNonFillerKL(&key, &llen, &len);
        printf(".");
        fflush(stdout);
        mxf_print_key(&key);
        file->skip(len);
    }

}



int main(int argc, const char **argv)
{
    try
    {
        printf("Testing writing...\n");
        testWrite();
        printf("Done testing writing\n");

        printf("Testing reading...\n");
        testRead(TEST_WRITE_FILENAME);
        printf("Done testing reading\n");

        remove(TEST_WRITE_FILENAME);
    }
    catch (MXFException &ex)
    {
        fprintf(stderr, "\nFailed:\n%s\n", ex.getMessage().c_str());
        remove(TEST_WRITE_FILENAME);
        return 1;
    }
    catch (const char *&ex)
    {
        fprintf(stderr, "\nFailed:\n%s\n", ex);
        remove(TEST_WRITE_FILENAME);
        return 1;
    }

    if (argc == 2)
    {
        const char *filename = argv[1];

        try
        {
            printf("Testing reading input '%s'...\n", filename);
            testRead(filename);
            printf("Done testing reading input\n");
        }
        catch (MXFException &ex)
        {
            fprintf(stderr, "\nFailed:\n%s\n", ex.getMessage().c_str());
            return 1;
        }
        catch (const char *&ex)
        {
            fprintf(stderr, "\nFailed:\n%s\n", ex);
            return 1;
        }
    }

    return 0;
}


