/*
 * Functions to create package definitions
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "package_definitions.h"
#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>



static void free_user_comment(UserComment **userComment)
{
    if ((*userComment) == NULL)
    {
        return;
    }

    SAFE_FREE((*userComment)->name);
    SAFE_FREE((*userComment)->value);

    SAFE_FREE(*userComment);
}

static void free_tagged_value_in_list(void *data)
{
    UserComment *userComment;

    if (data == NULL)
    {
        return;
    }

    userComment = (UserComment*)data;
    free_user_comment(&userComment);
}

static void free_locator(Locator **locator)
{
    if ((*locator) == NULL)
    {
        return;
    }

    SAFE_FREE((*locator)->comment);

    SAFE_FREE(*locator);
}

static void free_locator_in_list(void *data)
{
    Locator *locator;

    if (data == NULL)
    {
        return;
    }

    locator = (Locator*)data;
    free_locator(&locator);
}

static int create_user_comment(const char *name, const char *value, UserComment **userComment)
{
    UserComment *newUserComment = NULL;

    CHK_ORET(name != NULL);
    CHK_ORET(value != NULL);

    CHK_MALLOC_ORET(newUserComment, UserComment);
    memset(newUserComment, 0, sizeof(UserComment));

    CHK_OFAIL((newUserComment->name = strdup(name)) != NULL);
    CHK_OFAIL((newUserComment->value = strdup(value)) != NULL);

    *userComment = newUserComment;
    return 1;

fail:
    free_user_comment(&newUserComment);
    return 0;
}

static int create_locator(int64_t position, const char *comment, AvidRGBColor color, Locator **locator)
{
    Locator *newLocator = NULL;

    CHK_MALLOC_ORET(newLocator, Locator);
    memset(newLocator, 0, sizeof(Locator));

    if (comment != NULL)
    {
        CHK_OFAIL((newLocator->comment = strdup(comment)) != NULL);
    }
    newLocator->position = position;
    newLocator->color = color;

    *locator = newLocator;
    return 1;

fail:
    free_locator(&newLocator);
    return 0;
}

static void free_track(Track **track)
{
    if ((*track) == NULL)
    {
        return;
    }

    SAFE_FREE((*track)->name);

    SAFE_FREE(*track);
}

static void free_track_in_list(void *data)
{
    Track *track;

    if (data == NULL)
    {
        return;
    }

    track = (Track*)data;
    free_track(&track);
}

static void free_package(Package **package)
{
    if ((*package) == NULL)
    {
        return;
    }

    SAFE_FREE((*package)->name);
    mxf_clear_list(&(*package)->tracks);
    SAFE_FREE((*package)->filename);

    SAFE_FREE(*package);
}

static void free_package_in_list(void *data)
{
    Package *package;

    if (data == NULL)
    {
        return;
    }

    package = (Package*)data;
    free_package(&package);
}

static int create_package(const mxfUMID *uid, const char *name, const mxfTimestamp *creationDate, Package **package)
{
    Package *newPackage = NULL;

    CHK_MALLOC_ORET(newPackage, Package);
    memset(newPackage, 0, sizeof(Package));
    mxf_initialise_list(&newPackage->tracks, free_track_in_list);

    newPackage->uid = *uid;
    if (name != NULL)
    {
        CHK_OFAIL((newPackage->name = strdup(name)) != NULL);
    }
    else
    {
        newPackage->name = NULL;
    }
    newPackage->creationDate = *creationDate;

    *package = newPackage;
    return 1;

fail:
    free_package(&newPackage);
    return 0;
}


int create_package_definitions(PackageDefinitions **definitions, const mxfRational *locatorEditRate)
{
    PackageDefinitions *newDefinitions;

    CHK_MALLOC_ORET(newDefinitions, PackageDefinitions);
    memset(newDefinitions, 0, sizeof(PackageDefinitions));
    mxf_initialise_list(&newDefinitions->fileSourcePackages, free_package_in_list);
    mxf_initialise_list(&newDefinitions->userComments, free_tagged_value_in_list);
    mxf_initialise_list(&newDefinitions->locators, free_locator_in_list);
    newDefinitions->locatorEditRate = *locatorEditRate;

    *definitions = newDefinitions;
    return 1;
}

void free_package_definitions(PackageDefinitions **definitions)
{
    if (*definitions == NULL)
    {
        return;
    }

    free_package(&(*definitions)->materialPackage);
    mxf_clear_list(&(*definitions)->fileSourcePackages);
    mxf_clear_list(&(*definitions)->userComments);
    mxf_clear_list(&(*definitions)->locators);
    free_package(&(*definitions)->tapeSourcePackage);

    SAFE_FREE(*definitions);
}

void init_essence_info(EssenceInfo *essenceInfo)
{
    memset(essenceInfo, 0, sizeof(*essenceInfo));

    essenceInfo->locked = -1;
    essenceInfo->audioRefLevel = -256;
    essenceInfo->dialNorm = -256;
    essenceInfo->sequenceOffset = -1;
}

int create_material_package(PackageDefinitions *definitions, const mxfUMID *uid, const char *name,
                            const mxfTimestamp *creationDate)
{
    CHK_ORET(create_package(uid, name, creationDate, &definitions->materialPackage));
    return 1;
}

int create_file_source_package(PackageDefinitions *definitions, const mxfUMID *uid, const char *name,
                               const mxfTimestamp *creationDate, const char *filename, EssenceType essenceType,
                               const EssenceInfo *essenceInfo, Package **filePackage)
{
    Package *newFilePackage = NULL;

    if (filename == NULL)
    {
        mxf_log_error("File source package filename is null" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        return 0;
    }

    CHK_ORET(create_package(uid, name, creationDate, &newFilePackage));
    CHK_ORET(mxf_append_list_element(&definitions->fileSourcePackages, newFilePackage));

    CHK_ORET((newFilePackage->filename = strdup(filename)) != NULL);
    newFilePackage->essenceType = essenceType;
    newFilePackage->essenceInfo = *essenceInfo;

    *filePackage = newFilePackage;
    return 1;
}

int create_tape_source_package(PackageDefinitions *definitions, const mxfUMID *uid, const char *name,
                               const mxfTimestamp *creationDate)
{
    CHK_ORET(create_package(uid, name, creationDate, &definitions->tapeSourcePackage));
    return 1;
}

int set_user_comment(PackageDefinitions *definitions, const char *name, const char *value)
{
    UserComment *userComment = NULL;
    MXFListIterator iter;

    CHK_ORET(name != NULL);

    /* modify/remove user comment if it one already exists with given name */
    mxf_initialise_list_iter(&iter, &definitions->userComments);
    while (mxf_next_list_iter_element(&iter))
    {
        userComment = (UserComment*)mxf_get_iter_element(&iter);
        if (strcmp(userComment->name, name) == 0)
        {
            if (value == NULL)
            {
                void *valueData = mxf_remove_list_element_at_index(&definitions->userComments,
                                                                   mxf_get_list_iter_index(&iter));
                mxf_free_list_element_data(&definitions->userComments, valueData);
            }
            else
            {
                SAFE_FREE(userComment->value);
                CHK_ORET((userComment->value = strdup(value)) != NULL);
            }
            return 1;
        }
    }

    /* no existing user comment to remove */
    if (value == NULL)
    {
        return 1;
    }

    /* create a new user comment */
    CHK_ORET(create_user_comment(name, value, &userComment));
    CHK_OFAIL(mxf_append_list_element(&definitions->userComments, userComment));

    return 1;
fail:
    free_user_comment(&userComment);
    return 0;
}

void clear_user_comments(PackageDefinitions *definitions)
{
    mxf_clear_list(&definitions->userComments);
}

int add_locator(PackageDefinitions *definitions, int64_t position, const char *comment, AvidRGBColor color)
{
    Locator *newLocator = NULL;
    Locator *locator = NULL;
    MXFListIterator iter;
    int inserted = 0;

    if (mxf_get_list_length(&definitions->locators) >= MAX_LOCATORS)
    {
        /* silently ignore */
        return 1;
    }

    CHK_ORET(create_locator(position, comment, color, &newLocator));

    mxf_initialise_list_iter(&iter, &definitions->locators);
    while (mxf_next_list_iter_element(&iter))
    {
        locator = (Locator*)mxf_get_iter_element(&iter);

        if (locator->position > position)
        {
            CHK_OFAIL(mxf_insert_list_element(&definitions->locators, mxf_get_list_iter_index(&iter),
                1, newLocator));
            inserted = 1;
            break;
        }
    }
    if (!inserted)
    {
        CHK_OFAIL(mxf_append_list_element(&definitions->locators, newLocator));
    }

    return 1;

fail:
    free_locator(&locator);
    return 0;
}

void clear_locators(PackageDefinitions *definitions)
{
    mxf_clear_list(&definitions->locators);
}

int create_track(Package *package, uint32_t id, uint32_t number, const char *name, int isPicture,
                 const mxfRational *editRate, const mxfUMID *sourcePackageUID, uint32_t sourceTrackID,
                 int64_t startPosition, int64_t length, int64_t origin, Track **track)
{
    Track *newTrack = NULL;

    CHK_MALLOC_ORET(newTrack, Track);
    memset(newTrack, 0, sizeof(Track));

    newTrack->id = id;
    if (name != NULL)
    {
        CHK_OFAIL((newTrack->name = strdup(name)) != NULL);
    }
    else
    {
        newTrack->name = NULL;
    }
    newTrack->id = id;
    newTrack->number = number;
    newTrack->isPicture = isPicture;
    newTrack->editRate = *editRate;
    newTrack->sourcePackageUID = *sourcePackageUID;
    newTrack->sourceTrackID = sourceTrackID;
    newTrack->startPosition = startPosition;
    newTrack->length = length;
    newTrack->origin = origin;

    CHK_OFAIL(mxf_append_list_element(&package->tracks, newTrack));

    *track = newTrack;
    return 1;

fail:
    free_track(&newTrack);
    return 0;
}

void get_image_aspect_ratio(PackageDefinitions *definitions, const mxfRational *defaultImageAspectRatio,
                            mxfRational *imageAspectRatio)
{
    Package *filePackage;
    MXFListIterator iter;

    mxf_initialise_list_iter(&iter, &definitions->fileSourcePackages);
    while (mxf_next_list_iter_element(&iter))
    {
        filePackage = (Package*)mxf_get_iter_element(&iter);

        if (filePackage->essenceType != PCM && filePackage->essenceInfo.imageAspectRatio.numerator != 0)
        {
            *imageAspectRatio = filePackage->essenceInfo.imageAspectRatio;
            return;
        }
    }

    *imageAspectRatio = *defaultImageAspectRatio;
}

