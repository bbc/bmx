/*
 * Copyright (C) 2021, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

#include <bmx/essence_parser/FilePatternEssenceSource.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


FilePatternEssenceSource::FilePatternEssenceSource(bool fill_gaps)
: EssenceSource()
{
    mFillGaps = fill_gaps;
    mStartOffset = 0;
    mCurrentOffset = 0;
    mErrno = 0;
    mCurrentNumber = 0;
    mFileBufferOffset = 0;
}

FilePatternEssenceSource::~FilePatternEssenceSource()
{
}

bool FilePatternEssenceSource::Open(const string &pattern, int64_t start_offset)
{
    mStartOffset = start_offset;

    string file_path = get_abs_filename(get_cwd(), pattern);
    mDirname = strip_name(file_path);

    // Add %c to the sscanf format to ensure the whole pattern is checked
    string format = strip_path(file_path) + "%c";

    // Read the directory and store filenames that match the pattern
    DIR *dir = opendir(mDirname.c_str());
    if (!dir) {
        mErrno = errno;
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != 0) {
        // Add a newline that wouldn't appear in a filename and should be
        // returned by sscanf as the last character
        string name_with_newline = entry->d_name;
        name_with_newline += "\n";

        int index;
        char last_char;
        if (sscanf(name_with_newline.c_str(), format.c_str(), &index, &last_char) == 2 && last_char == '\n')
            mFilenames[index] = entry->d_name;
    }

    closedir(dir);

    if (mFilenames.empty())
        log_warn("No files found for file pattern\n");

    return SeekStart();
}

uint32_t FilePatternEssenceSource::Read(unsigned char *data, uint32_t size)
{
    return (uint32_t)ReadOrSkip(data, size, 0);
}

bool FilePatternEssenceSource::SeekStart()
{
    if (mFilenames.empty())
        return true;

    mNextFilenamesIter = mFilenames.begin();
    mCurrentNumber = mNextFilenamesIter->first;
    mCurrentFilename = mNextFilenamesIter->second;

    mNextFilenamesIter++;

    mCurrentOffset = 0;
    mFileBuffer.Clear();
    mFileBufferOffset = 0;

    return Skip(mStartOffset - mCurrentOffset);
}

bool FilePatternEssenceSource::Skip(int64_t offset)
{
    return ReadOrSkip(0, 0, offset) == offset;
}

int64_t FilePatternEssenceSource::ReadOrSkip(unsigned char *data, uint32_t size, int64_t skip_offset)
{
    if (mFilenames.empty() || (data == 0 && skip_offset <= 0))
        return 0;

    bool do_read = (data != 0);
    int64_t rem_size = do_read ? size : skip_offset;
    while (rem_size > 0) {
        if (mFileBuffer.GetSize() == 0) {
            // Buffer or skip the current file
            bool buffer_file = true;
            if (!do_read) {
                // Decide whether to buffer the current file; don't if skipping past it
                int64_t file_size = 0;
                try {
                    file_size = get_file_size(GetCurrentFilePath());
                } catch (const BMXIOException &ex) {
                    mErrno = ex.GetErrno();
                    break;
                }
                if (file_size == 0) {
                    // We're done if the file is empty
                    break;
                }

                if (rem_size >= file_size) {
                    // Skip past the current file
                    buffer_file = false;
                    rem_size -= file_size;
                    if (!NextFile())
                        break;
                }
            }

            if (buffer_file && (!BufferFile() || mFileBuffer.GetSize() == 0))
                break;
        } else if (mFileBufferOffset < mFileBuffer.GetSize()) {
            // Copy or skip from the file buffer
            uint32_t copy_size = mFileBuffer.GetSize() - mFileBufferOffset;
            if (copy_size > rem_size)
                copy_size = (uint32_t)rem_size;

            if (do_read)
                memcpy(&data[size - rem_size], &mFileBuffer.GetBytes()[mFileBufferOffset], copy_size);

            mFileBufferOffset += copy_size;
            mCurrentOffset += copy_size;
            rem_size -= copy_size;
        } else {  // mFileBufferOffset >= mFileBuffer.GetSize()
            // At the end of the current file; move to the start of the next file
            if (!NextFile())
                break;
        }
    }

    if (do_read)
        return size - rem_size;
    else
        return skip_offset - rem_size;
}

string FilePatternEssenceSource::GetStrError() const
{
    return bmx_strerror(mErrno);
}

bool FilePatternEssenceSource::NextFile()
{
    if (mNextFilenamesIter == mFilenames.end())
        return false;

    if (mFillGaps)
        mCurrentNumber++;
    else
        mCurrentNumber = mNextFilenamesIter->first;

    if (mCurrentNumber >= mNextFilenamesIter->first) {
        mCurrentFilename = mNextFilenamesIter->second;
        mFileBuffer.Clear();

        mNextFilenamesIter++;
    }
    mFileBufferOffset = 0;

    return true;
}

bool FilePatternEssenceSource::BufferFile()
{
    mFileBuffer.Clear();
    mFileBufferOffset = 0;

    string filepath = GetCurrentFilePath();
    FILE *file = fopen(filepath.c_str(), "rb");
    if (!file) {
        mErrno = errno;
        return false;
    }

    int64_t file_size;
    try {
        file_size = get_file_size(file);
    } catch (const BMXIOException &ex) {
        mErrno = ex.GetErrno();
        return false;
    }

    BMX_CHECK(file_size <= UINT32_MAX);
    mFileBuffer.Allocate((uint32_t)file_size);

    int64_t rem_read = file_size;
    while (rem_read > 0) {
        size_t next_read = 8192;
        if ((int64_t)next_read > rem_read)
            next_read = (size_t)rem_read;

        size_t num_read = fread(mFileBuffer.GetBytesAvailable(), 1, next_read, file);
        if (num_read != next_read && ferror(file) != 0)
            mErrno = errno;

        mFileBuffer.IncrementSize(num_read);
        rem_read -= num_read;

        if (num_read != next_read)
            break;
    }

    fclose(file);

    return mErrno == 0;
}
