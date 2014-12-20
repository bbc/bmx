/*
 * Copyright (C) 2014, British Broadcasting Corporation
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

#ifdef HAVE_LIBCURL

#define __STDC_FORMAT_MACROS

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <curl/curl.h>

#include <mxf/mxf.h>

#include <bmx/MXFHTTPFile.h>
#include <bmx/Utils.h>
#include <bmx/Logging.h>
#include <bmx/BMXException.h>

using namespace std;
using namespace bmx;


typedef struct
{
    MXFFile *mxf_file;
} MXFHTTPFile;

struct MXFFileSysData
{
    MXFHTTPFile http_file;
    string url_str;
    CURL *curl;
    int64_t position;
    int eof;
    unsigned char *buffer;
    uint32_t buffer_pos;
    uint32_t buffer_size;
    uint32_t buffer_alloc_size;
    bool disable_response_code_warn;
};

typedef struct
{
    MXFFileSysData *sys_data;
    int64_t range_first;
    int64_t range_last;
    uint8_t *client_data;
    uint32_t client_rem_count;
    uint32_t read_count;
    bool accept_range_recv;
    bool accept_bytes_range;
} CURLReceiveInfo;


static size_t get_http_field_value_pos(const string &header_str, const string &field_name)
{
  size_t fidx = header_str.find(field_name);
  if (fidx != string::npos) {
      fidx += field_name.size();
      while (header_str[fidx] && (header_str[fidx] == ':' || header_str[fidx] == ' '))
          fidx++;
  }
  return fidx;
}

static size_t curl_file_size_header_cb(char *buffer, size_t size, size_t nmemb, void *priv)
{
  int64_t *file_size_out = (int64_t*)priv;

  const string header_str(buffer, size * nmemb);

  size_t fidx = get_http_field_value_pos(header_str, "Content-Length");
  if (fidx != string::npos) {
      int64_t content_length;
      if (sscanf(&header_str.c_str()[fidx], "%"PRId64, &content_length) == 1)
          *file_size_out = content_length;
  }

  return size * nmemb;
}

static size_t curl_header_cb(char *buffer, size_t size, size_t nmemb, void *priv)
{
  CURLReceiveInfo *info = (CURLReceiveInfo*)priv;

  const string header_str(buffer, size * nmemb);

  size_t fidx = get_http_field_value_pos(header_str, "Accept-Ranges");
  if (fidx != string::npos) {
      info->accept_range_recv = true;
      if (header_str.compare(fidx, 5, "bytes"))
          info->accept_bytes_range = true;
  }

  fidx = get_http_field_value_pos(header_str, "Content-Range");
  if (fidx != string::npos) {
      int64_t first;
      if (sscanf(&header_str.c_str()[fidx], "%"PRId64, &first) == 1) {
          if (first != info->range_first) {
              log_warn("HTTP content range start byte at %"PRId64" does not match requested start byte at %"PRId64"\n",
                       first, info->range_first);
          }
      }
  }

  return size * nmemb;
}

static size_t curl_data_cb(void* ptr, size_t size, size_t nmemb, void *priv)
{
  CURLReceiveInfo *info = (CURLReceiveInfo*)priv;

  size_t rec_count = size * nmemb;

  uint32_t client_copy_count = 0;
  if (info->client_rem_count > 0) {
      client_copy_count = info->client_rem_count;
      if (client_copy_count > rec_count)
          client_copy_count = (uint32_t)(rec_count);
      memcpy(&info->client_data[info->read_count], ptr, client_copy_count);
      info->client_rem_count   -= client_copy_count;
      info->sys_data->position += client_copy_count;
      info->read_count         += client_copy_count;
  }

  uint32_t buf_copy_count = 0;
  if (client_copy_count < rec_count) {
      buf_copy_count = info->sys_data->buffer_alloc_size - info->sys_data->buffer_size;
      if (buf_copy_count > rec_count - client_copy_count)
          buf_copy_count = (uint32_t)(rec_count - client_copy_count);
      memcpy(&info->sys_data->buffer[info->sys_data->buffer_size], &((unsigned char*)ptr)[client_copy_count], buf_copy_count);
      info->sys_data->buffer_size += buf_copy_count;
      info->read_count            += buf_copy_count;
  }

  return (size_t)client_copy_count + (size_t)buf_copy_count;
}


static void http_file_close(MXFFileSysData *sys_data)
{
    if (sys_data->curl)
        curl_easy_cleanup(sys_data->curl);
    delete [] sys_data->buffer;
}

static uint32_t http_file_read(MXFFileSysData *sys_data, uint8_t *data, uint32_t count)
{
    uint8_t *data_ptr  = data;
    uint32_t rem_count = count;

    sys_data->eof = 0;

    uint32_t buffer_copy_size = 0;
    if (sys_data->buffer_pos < sys_data->buffer_size) {
        buffer_copy_size = sys_data->buffer_size - sys_data->buffer_pos;
        if (buffer_copy_size > rem_count)
            buffer_copy_size = rem_count;
        if (buffer_copy_size > 0) {
            memcpy(data_ptr, &sys_data->buffer[sys_data->buffer_pos], buffer_copy_size);
            sys_data->buffer_pos += buffer_copy_size;
            sys_data->position   += buffer_copy_size;
            data_ptr             += buffer_copy_size;
            rem_count            -= buffer_copy_size;
        }
    }

    if (rem_count > 0) {
        char error_buf[CURL_ERROR_SIZE];

        sys_data->buffer_pos  = 0;
        sys_data->buffer_size = 0;

        CURLReceiveInfo info;
        memset(&info, 0, sizeof(info));
        info.sys_data         = sys_data;
        info.client_data      = data_ptr;
        info.client_rem_count = rem_count;
        info.range_first      = sys_data->position;
        info.range_last       = sys_data->position;
        if (rem_count > sys_data->buffer_alloc_size)
            info.range_last += rem_count - 1;
        else
            info.range_last += sys_data->buffer_alloc_size - 1;

        char range_buf[64];
        bmx_snprintf(range_buf, sizeof(range_buf), "%"PRId64"-%"PRId64,
                     info.range_first, info.range_last);

        curl_easy_reset(sys_data->curl);
        curl_easy_setopt(sys_data->curl, CURLOPT_ERRORBUFFER, error_buf);
        curl_easy_setopt(sys_data->curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(sys_data->curl, CURLOPT_URL, sys_data->url_str.c_str());
        curl_easy_setopt(sys_data->curl, CURLOPT_RANGE, range_buf);
        curl_easy_setopt(sys_data->curl, CURLOPT_WRITEFUNCTION, curl_data_cb);
        curl_easy_setopt(sys_data->curl, CURLOPT_WRITEDATA, (void*)&info);
        curl_easy_setopt(sys_data->curl, CURLOPT_HEADERFUNCTION, curl_header_cb);
        curl_easy_setopt(sys_data->curl, CURLOPT_HEADERDATA, (void*)&info);
        curl_easy_setopt(sys_data->curl, CURLOPT_FAILONERROR, 1);

        CURLcode result = curl_easy_perform(sys_data->curl);
        rem_count = info.client_rem_count;

        if (result != 0) {
            if (result != CURLE_PARTIAL_FILE) {
                if (result == CURLE_WRITE_ERROR) {
                    if (!info.accept_range_recv || !info.accept_bytes_range) {
                        if (info.range_first != 0)
                            rem_count = count;
                        bmx::log((info.range_first == 0 ? WARN_LOG: ERROR_LOG),
                                 "HTTP server does not support byte range requests\n");
                    } else {
                        log_error("HTTP server returned more data than requested\n");
                    }
                } else {
                    log_error("HTTP request failed: %s (curl result %d)\n", error_buf, result);
                }
            }
        } else {
            long code;
            curl_easy_getinfo(sys_data->curl, CURLINFO_RESPONSE_CODE, &code);
            if (code != 206) { // 206 = partial content
                if (!sys_data->disable_response_code_warn) {
                    if (info.accept_range_recv && !info.accept_bytes_range)
                        log_warn("HTTP server does not support byte range requests\n");
                    else if (!info.accept_range_recv)
                        log_warn("HTTP server does not indicate support for byte range requests\n");
                    else
                        log_warn("Unexpected HTTP response code %ld\n", code);
                    sys_data->disable_response_code_warn = true;
                }
            } else {
                sys_data->disable_response_code_warn = false;
            }
        }
    }

    return count - rem_count;
}

static uint32_t http_file_write(MXFFileSysData *sys_data, const uint8_t *data, uint32_t count)
{
    (void)sys_data;
    (void)data;
    (void)count;
    return EOF;
}

static int http_file_getc(MXFFileSysData *sys_data)
{
    uint8_t data;
    uint32_t num_read = http_file_read(sys_data, &data, 1);
    if (num_read == 0)
        return EOF;

    return data;
}

static int http_file_putc(MXFFileSysData *sys_data, int c)
{
    (void)sys_data;
    (void)c;
    return EOF;
}

static int http_file_eof(MXFFileSysData *sys_data)
{
    return sys_data->eof;
}

static int64_t http_file_tell(MXFFileSysData *sys_data)
{
    return sys_data->position;
}

static int http_file_is_seekable(MXFFileSysData *sys_data)
{
    (void)sys_data;
    return 1;
}

static int64_t http_file_size(MXFFileSysData *sys_data)
{
    int64_t file_size = -1;

    // note: not using CURLINFO_CONTENT_LENGTH_DOWNLOAD because it returns a double

    char error_buf[CURL_ERROR_SIZE];
    curl_easy_reset(sys_data->curl);
    curl_easy_setopt(sys_data->curl, CURLOPT_ERRORBUFFER, error_buf);
    curl_easy_setopt(sys_data->curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(sys_data->curl, CURLOPT_URL, sys_data->url_str.c_str());
    curl_easy_setopt(sys_data->curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(sys_data->curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(sys_data->curl, CURLOPT_WRITEFUNCTION, curl_file_size_header_cb);
    curl_easy_setopt(sys_data->curl, CURLOPT_WRITEDATA, &file_size);
    curl_easy_setopt(sys_data->curl, CURLOPT_FAILONERROR, 1);

    CURLcode result = curl_easy_perform(sys_data->curl);
    if (result != 0)
        log_error("Failed to get file size: %s (curl result = %d)\n", error_buf, result);

    return file_size;
}

static int http_file_seek(MXFFileSysData *sys_data, int64_t offset, int whence)
{
    int64_t new_position = 0;

    if (whence == SEEK_CUR) {
        new_position = sys_data->position + offset;
    } else if (whence == SEEK_SET) {
        new_position = offset;
    } else if (whence == SEEK_END) {
        int64_t file_size = http_file_size(sys_data);
        if (file_size < 0)
            return 0;
        new_position = file_size + offset;
    } else {
        return 0;
    }

    if (sys_data->position != new_position) {
        int64_t pos_diff = new_position - sys_data->position;
        if (sys_data->buffer_size > 0 &&
            pos_diff >= - (int64_t)sys_data->buffer_pos &&
            pos_diff < (int64_t)(sys_data->buffer_size - sys_data->buffer_pos))
        {
            sys_data->buffer_pos = (uint32_t)(sys_data->buffer_pos + pos_diff);
        }
        else
        {
            sys_data->buffer_size = 0;
            sys_data->buffer_pos  = 0;
        }
        sys_data->position = new_position;
    }

    sys_data->eof = false;

    return 1;
}

static void free_http_file(MXFFileSysData *sys_data)
{
    delete sys_data;
}


bool bmx::mxf_http_is_supported()
{
    return true;
}

bool bmx::mxf_http_is_url(const string &url_str)
{
    return url_str.compare(0, 7, "http://") == 0 ||
           url_str.compare(0, 8, "https://") == 0;
}

MXFFile* bmx::mxf_http_file_open_read(const string &url_str, uint32_t min_read_size)
{
    MXFFile *http_file = 0;
    try
    {
        // using malloc() because mxf_file_close will call free()
        BMX_CHECK((http_file = (MXFFile*)malloc(sizeof(MXFFile))) != 0);
        memset(http_file, 0, sizeof(MXFFile));

        http_file->sysData = new MXFFileSysData;
        http_file->sysData->http_file.mxf_file = http_file;
        http_file->sysData->url_str = url_str;
        http_file->sysData->curl = 0;
        http_file->sysData->position = 0;
        http_file->sysData->eof = false;
        http_file->sysData->buffer = 0;
        http_file->sysData->buffer_pos = 0;
        http_file->sysData->buffer_size = 0;
        http_file->sysData->buffer_alloc_size = 0;

        BMX_CHECK((http_file->sysData->curl = curl_easy_init()) != 0);
        if (min_read_size > 0) {
            http_file->sysData->buffer            = new unsigned char[min_read_size];
            http_file->sysData->buffer_alloc_size = min_read_size;
        }

        http_file->close         = http_file_close;
        http_file->read          = http_file_read;
        http_file->write         = http_file_write;
        http_file->get_char      = http_file_getc;
        http_file->put_char      = http_file_putc;
        http_file->eof           = http_file_eof;
        http_file->seek          = http_file_seek;
        http_file->tell          = http_file_tell;
        http_file->is_seekable   = http_file_is_seekable;
        http_file->size          = http_file_size;
        http_file->free_sys_data = free_http_file;

        return http_file;
    }
    catch (...)
    {
        if (http_file)
            mxf_file_close(&http_file);
        throw;
    }
}


#else // ifdef HAVE_LIBCURL


#include <mxf/mxf.h>

#include <bmx/MXFHTTPFile.h>
#include <bmx/Logging.h>
#include <bmx/BMXException.h>

using namespace std;
using namespace bmx;


bool bmx::mxf_http_is_supported()
{
    return false;
}

bool bmx::mxf_http_is_url(const string &url_str)
{
    return url_str.compare(0, 7, "http://") == 0 ||
           url_str.compare(0, 8, "https://") == 0;
}

MXFFile* bmx::mxf_http_file_open_read(const string &url_str, uint32_t min_read_size)
{
    (void)url_str;
    (void)min_read_size;
    BMX_EXCEPTION(("HTTP file access is not supported in this build"));
}


#endif
