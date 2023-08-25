/*
 * VLC MXF demux module
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

/* TODO: handling libMXF messages */
/* TODO: correct pts for variable audio frame sizes,
   eg. 5 frame sequence for 48kHz audio for NTSC. Will audio frame start
   always be <= video frame start? */

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include <vlc/vlc.h>
#include <vlc/input.h>

#include <mxf_reader.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int Open( vlc_object_t * );
static void Close( vlc_object_t * );

vlc_module_begin();
    set_shortname( "MXF" );
    set_description( _("MXF (Material Exchange Format) demuxer") );
    set_capability( "demux2", 2 ); /* TODO: score? */
    set_category( CAT_INPUT );
    set_subcategory( SUBCAT_INPUT_DEMUX );
    set_callbacks( Open, Close );
    add_shortcut( "mxf" );
vlc_module_end();


/*****************************************************************************
 * Constants
 *****************************************************************************/

/* some big number that we would never expect to exceed */
#define MAX_ESSENCE_STREAMS      64


/*****************************************************************************
 * Definitions of structures used by this plugin
 *****************************************************************************/

struct demux_sys_t
{
    /* MXF reader */
    MXFReader *p_mxfReader;

    /* essence streams */
    es_out_id_t *p_es[MAX_ESSENCE_STREAMS];
    int numTracks;

    /* output frame rate used to update the pcr */
    double f_rate;

    /* data block received from the MXF reader */
    block_t *p_block;

    /* if this demux is a slave then it doesn't set the PCR */
    vlc_bool_t b_isSlave;

    /* program clock reference */
    mtime_t i_pcr;
};

struct MXFReaderListenerData
{
    demux_t *p_demux;
};

struct MXFFileSysData
{
    /* wrapped VLC stream */
    stream_t *p_stream;

    /* wrapped byte array */
    byte_t *p_data;
    int64_t i_size;
    int64_t i_pos;
};


/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int accept_frame(MXFReaderListener *listener, int trackIndex);
static int allocate_buffer(MXFReaderListener *listener, int trackIndex, uint8_t **buffer, uint32_t bufferSize);
static void deallocate_buffer(MXFReaderListener *listener, int trackIndex, uint8_t **buffer);
static int receive_frame(MXFReaderListener *listener, int trackIndex, uint8_t *buffer, uint32_t bufferSize);

static int Demux( demux_t * );
static int Control( demux_t *, int i_query, va_list args );

static int get_fourcc( const mxfUL *essenceContainerLabel, vlc_fourcc_t *fourcc, int isVideo );
static int create_mxf_reader( stream_t *p_stream, MXFReader **p_mxfReader );
static int wrap_byte_array( demux_t *p_demux, byte_t *p_data, int64_t i_size, MXFFile **pp_mxfFile );


/*****************************************************************************
 * Open: initializes MXF demux structures
 *****************************************************************************/
static int Open( vlc_object_t * p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = NULL;
    byte_t *p_peek;
    MXFFile *p_mxfFile = NULL;
    MXFReader *p_mxfReader = NULL;
    MXFClip *p_clip;
    MXFTrack *p_track;
    int i;
    es_format_t format;
    vlc_fourcc_t fourcc;
    int64_t i_peekSize;


    /* peek for the maximum MXF run-in length (65536) + a realistic maximum size for a
       header partition pack (356), and check if format is MXF and that the essence
       and operational pattern is supported. Will accept peek size >= 356 */
    if ( ( i_peekSize = stream_Peek( p_demux->s, &p_peek, 65891 ) ) < 356 )
    {
        return VLC_EGENERIC;
    }
    if ( !wrap_byte_array( p_demux, p_peek, i_peekSize, &p_mxfFile ) )
    {
        return VLC_EGENERIC;
    }
    if ( !format_is_supported( p_mxfFile ) )
    {
        mxf_file_close( &p_mxfFile );
        return VLC_EGENERIC;
    }
    mxf_file_close( &p_mxfFile );



    /* format recognized -> create the MXF reader. Note: this could still fail if
       the file is invalid in some way or has an unexpected format */
    if ( !create_mxf_reader( p_demux->s, &p_mxfReader ) )
    {
        msg_Err( p_demux, "failed to create MXF reader" );
        return VLC_EGENERIC;
    }


    /* create the demux sys data */

    if ( ( p_sys = malloc( sizeof( demux_sys_t ) ) ) == NULL )
    {
        msg_Err( p_demux, "Out of memory" );
        goto fail;
    }
    memset( p_sys, 0, sizeof( demux_sys_t ) );

    p_clip = get_mxf_clip(p_mxfReader);
    p_sys->p_mxfReader = p_mxfReader;
    p_sys->i_pcr = 1;
    p_sys->numTracks = get_num_tracks(p_mxfReader);
    p_sys->f_rate = p_clip->frameRate.numerator / (double)p_clip->frameRate.denominator;

    for ( i = 0; i < p_sys->numTracks; i++ )
    {
        if ( i > MAX_ESSENCE_STREAMS )
        {
            msg_Warn( p_demux, "reached maximum essence stream %d - software update required", MAX_ESSENCE_STREAMS );
            break;
        }

        p_sys->p_es[i] = NULL;
        memset(&format, 0, sizeof(es_format_t));

        p_track = get_mxf_track( p_mxfReader, i );
        if ( p_track->isVideo )
        {
            if ( !get_fourcc( &p_track->essenceContainerLabel, &fourcc, 1 ) )
            {
                msg_Err( p_demux, "unknown MXF video essence type" );
                goto fail;
            }

            es_format_Init( &format, VIDEO_ES, fourcc );

            format.video.i_width = p_track->video.frameWidth;
            format.video.i_height= p_track->video.frameHeight;
            format.video.i_x_offset = p_track->video.displayXOffset;
            format.video.i_y_offset = p_track->video.displayYOffset;
            format.video.i_visible_width = p_track->video.displayWidth;
            format.video.i_visible_height = p_track->video.displayHeight;
            /* TODO: check this bits_per_pixel */
            format.video.i_bits_per_pixel = p_track->video.componentDepth *
                ( 1.0 / (double)p_track->video.horizSubsampling +
                2 * 1.0 / (double)p_track->video.vertSubsampling );
            format.video.i_frame_rate = p_track->video.frameRate.numerator;
            format.video.i_frame_rate_base = p_track->video.frameRate.denominator;
            if (p_track->video.aspectRatio.numerator != 0 && p_track->video.aspectRatio.denominator != 0)
            {
                /* TODO: what is VOUT_ASPECT_FACTOR, and why is video not exactly 16:9 for some XDCAM example files? */
                format.video.i_aspect = VOUT_ASPECT_FACTOR *
                    p_track->video.aspectRatio.numerator / p_track->video.aspectRatio.denominator;
            }

            if ( ( p_sys->p_es[i] = es_out_Add( p_demux->out, &format ) ) == NULL )
            {
                msg_Err( p_demux, "failed to add video es" );
                goto fail;
            }
        }
        else
        {
            if ( !get_fourcc( &p_track->essenceContainerLabel, &fourcc, 0 ) )
            {
                msg_Err( p_demux, "unknown MXF audio essence type" );
                goto fail;
            }

            es_format_Init( &format, AUDIO_ES, fourcc );

            format.audio.i_channels = p_track->audio.channelCount;
            format.audio.i_rate = p_track->audio.samplingRate.numerator;
            format.audio.i_blockalign = p_track->audio.blockAlign;
            format.audio.i_bitspersample = 8 * p_track->audio.blockAlign / p_track->audio.channelCount;

            if ( ( p_sys->p_es[i] = es_out_Add( p_demux->out, &format ) ) == NULL )
            {
                msg_Err( p_demux, "failed to add audio es" );
                goto fail;
            }
        }
    }


    /* set the demux properties */

    p_demux->pf_demux = Demux;
    p_demux->pf_control = Control;
    p_demux->p_sys = p_sys;


    return VLC_SUCCESS;


fail:
    close_mxf_reader( &p_mxfReader );
    if ( p_sys != NULL )
    {
        for ( i = 0; i < p_sys->numTracks; i++ )
        {
            if ( p_sys->p_es[i] == NULL )
            {
                break;
            }
            es_out_Del( p_demux->out, p_sys->p_es[i] );
        }
        free( p_sys );
    }

    p_demux->pf_demux = NULL;
    p_demux->pf_control = NULL;
    p_demux->p_sys = NULL;

    return VLC_EGENERIC;
}


/*****************************************************************************
 * Close: frees unused data
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = p_demux->p_sys;

    close_mxf_reader( &p_sys->p_mxfReader );
    free( p_sys );
}


/*****************************************************************************
 * Demux: reads and demuxes data packets
 *****************************************************************************
 * Returns -1 in case of error, 0 in case of EOF, 1 otherwise
 *****************************************************************************/
static int Demux( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    MXFReaderListener listener;
    MXFReaderListenerData data;
    int i_result;

    data.p_demux = p_demux;
    listener.data = &data;
    listener.accept_frame = accept_frame;
    listener.allocate_buffer = allocate_buffer;
    listener.deallocate_buffer = deallocate_buffer;
    listener.receive_frame = receive_frame;

    /* slave demux doesn't set the pace */
    if ( !p_sys->b_isSlave )
    {
        es_out_Control( p_demux->out, ES_OUT_SET_PCR, p_sys->i_pcr );
    }

    if ( ( i_result = read_next_frame( p_sys->p_mxfReader, &listener ) ) != 1 )
    {
        if ( p_sys->p_block != NULL )
        {
            block_Release(  p_sys->p_block );
            p_sys->p_block = NULL;
        }

        if ( i_result == -1 ) /* EOF */
        {
            return 0;
        }
        return -1; /* error */
    }

    p_sys->i_pcr += ( I64C(1000000) / p_sys->f_rate );

    return 1;
}


static int accept_frame(MXFReaderListener *listener, int trackIndex)
{
    return 1;
}

static int allocate_buffer( MXFReaderListener *listener, int trackIndex, uint8_t **buffer, uint32_t bufferSize )
{
    demux_t *p_demux = listener->data->p_demux;

    if ( ( p_demux->p_sys->p_block = block_New( p_demux, bufferSize ) ) == NULL )
    {
        msg_Err( p_demux, "failed to allocated buffer for frame" );
        return 0;
    }

    *buffer = p_demux->p_sys->p_block->p_buffer;

    return 1;
}

static void deallocate_buffer( MXFReaderListener *listener, int trackIndex, uint8_t **buffer )
{
    demux_t *p_demux = listener->data->p_demux;
    demux_sys_t *p_sys = p_demux->p_sys;

    if ( *buffer != p_sys->p_block->p_buffer )
    {
        /* this is really a debug assertion - we should never be here */
        msg_Err( p_demux, "invalid buffer pointer for deallocation" );
    }

    if ( p_sys->p_block->p_buffer != NULL)
    {
        block_Release( p_sys->p_block );
        p_sys->p_block = NULL;
        *buffer = NULL;
    }
}

static int receive_frame( MXFReaderListener *listener, int trackIndex, uint8_t *buffer, uint32_t bufferSize )
{
    demux_t *p_demux = listener->data->p_demux;
    demux_sys_t *p_sys = p_demux->p_sys;
    MXFTrack *p_track;
    vlc_bool_t b_audio;

    if ( buffer != p_sys->p_block->p_buffer )
    {
        /* this is really a debug assertion - we should never be here */
        msg_Err( p_demux, "invalid buffer pointer for receiving frame" );
        block_Release( p_sys->p_block );
        p_sys->p_block = NULL;
        return 0;
    }

    /* AES-3 audio data in D-10 is an example where the bufferSize changes
       because the AES-3 data was converted into PCM */
    if ( bufferSize < (uint32_t)p_sys->p_block->i_buffer )
    {
        p_sys->p_block->i_buffer = bufferSize;
    }

    p_track = get_mxf_track( p_sys->p_mxfReader, trackIndex );
    if ( p_track->isVideo )
    {
        p_sys->p_block->i_pts = p_sys->i_pcr;
        es_out_Send( p_demux->out, p_sys->p_es[trackIndex], p_sys->p_block );
        p_sys->p_block = NULL; /* release ownership */
    }
    else
    {
        es_out_Control( p_demux->out, ES_OUT_GET_ES_STATE, p_sys->p_es[trackIndex], &b_audio );
        if ( b_audio )
        {
            p_sys->p_block->i_pts = p_sys->i_pcr;
            es_out_Send( p_demux->out, p_sys->p_es[trackIndex], p_sys->p_block );
            p_sys->p_block = NULL; /* release ownership */
        }
        else
        {
            block_Release( p_sys->p_block );
            p_sys->p_block = NULL;
        }
    }

    return 1;
}


/*****************************************************************************
 * Control:
 *****************************************************************************/
static int Control( demux_t *p_demux, int i_query, va_list args )
{
    double *pf;
    double f;
    int64_t i64;
    int64_t *pi64;
    demux_sys_t *p_sys = p_demux->p_sys;
    MXFReader *p_mxfReader = p_sys->p_mxfReader;
    MXFClip *p_clip = get_mxf_clip(p_mxfReader);

!!!!
!!!! NOTE: get_duration() has changed in mxf_reader: it no longer returns the minimum duration
!!!! The use of this function needs to be checked here
!!!!
    switch( i_query )
    {
        case DEMUX_GET_LENGTH:
            pi64 = (int64_t*)va_arg( args, int64_t * );
            printf("DEMUX_GET_LENGTH\n");
            if ( get_duration(p_mxfReader) >= 0 )
            {
                *pi64 = (int64_t)(I64C(1000000) * get_duration(p_mxfReader) *
                    p_clip->frameRate.numerator / (double)p_clip->frameRate.denominator);
                printf("\t = %" PRId64 "\n", *pi64);
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;


        case DEMUX_GET_TIME:
            pi64 = (int64_t*)va_arg( args, int64_t * );
            printf("DEMUX_GET_TIME\n");
            if ( get_duration(p_mxfReader) >= 0 )
            {
                *pi64 = (int64_t)(I64C(1000000) * ( get_frame_number( p_mxfReader ) + 1 ) *
                    p_clip->frameRate.numerator / (double)p_clip->frameRate.denominator);
                printf("\t = %" PRId64 "\n", *pi64);
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;

        case DEMUX_GET_POSITION:
            pf = (double*)va_arg( args, double * );
            printf("DEMUX_GET_POSITION\n");
            if ( get_duration(p_mxfReader) >= 0 )
            {
                if ( get_duration(p_mxfReader) > 0 )
                {
                    *pf = (double)( ( get_frame_number(p_mxfReader) + 1 ) ) /
                          (double)( get_duration(p_mxfReader) );
                }
                else
                {
                    *pf = 0.0;
                }
                printf("\t = %lf\n", *pf);
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;


        case DEMUX_SET_POSITION:
            f = (double)va_arg( args, double );
            printf("DEMUX_SET_POSITION = %lf\n", f);
            if ( f >= 0.0 && f <= 1.0 && get_duration(p_mxfReader) > 0 )
            {
                if ( position_at_frame( p_mxfReader, (int64_t)( f * get_duration(p_mxfReader) ) ) )
                {
                    return VLC_SUCCESS;
                }
            }
            return VLC_EGENERIC;

        case DEMUX_SET_TIME:
            i64 = (int64_t)va_arg( args, int64_t );
            printf("DEMUX_SET_TIME = %" PRId64 "\n", i64);
            if ( i64 >= 0 && get_duration(p_mxfReader) > 0 )
            {
                if ( position_at_frame( p_mxfReader, (int64_t)( i64 /
                    ( I64C(1000000) *
                        p_clip->frameRate.numerator / (double)p_clip->frameRate.denominator ) ) ) )
                {
                    return VLC_SUCCESS;
                }
            }
            return VLC_EGENERIC;

        /* TODO: this will break easily - what is the best way to determine this demux is a slave? */
        case DEMUX_SET_NEXT_DEMUX_TIME:
            p_sys->b_isSlave = VLC_TRUE;
            return VLC_SUCCESS;


        case DEMUX_GET_FPS:
        case DEMUX_GET_META:
        case DEMUX_GET_TITLE_INFO:
        case DEMUX_SET_GROUP:
            return VLC_EGENERIC;

        default:
            msg_Err( p_demux, "unknown query in MXF Control" );
            return VLC_EGENERIC;
    }
}


/*****************************************************************************
 * Wrap the VLC stream in an MXF file
 *****************************************************************************/

static void vlcstream_file_close( MXFFileSysData *p_sysData )
{
    p_sysData->p_stream = NULL;
}

static uint32_t vlcstream_file_read( MXFFileSysData *p_sysData, uint8_t *p_data, uint32_t i_count )
{
    uint32_t i_totalRead = 0;
    int i_numRead;
    int i_actualRead;

    /* read bytes until we reach i_count or eof */
    i_totalRead = 0;
    while ( i_totalRead < i_count )
    {
        if ( i_count - i_totalRead > INT_MAX )
        {
            i_numRead = INT_MAX;
        }
        else
        {
            i_numRead = i_count - i_totalRead;
        }
        i_actualRead = stream_Read( p_sysData->p_stream, &p_data[i_totalRead], i_numRead );
        i_totalRead += i_actualRead;

        /* break if not enough was read */
        if ( i_actualRead < i_numRead )
        {
            break;
        }
    }

    return i_totalRead;
}

static uint32_t vlcstream_file_write( MXFFileSysData *p_sysData, const uint8_t *p_data, uint32_t i_count )
{
    /* not used and not implemented */
    return 0;
}

static int vlcstream_file_getchar( MXFFileSysData *p_sysData )
{
    int i_actualRead;
    uint8_t p_data[1];

    i_actualRead = stream_Read( p_sysData->p_stream, p_data, 1 );
    if ( i_actualRead != 1 )
    {
        return -1;
    }
    return p_data[0];
}

static int vlcstream_file_putchar( MXFFileSysData *p_sysData, int i_c )
{
    /* not used and not implemented */
    return 0;
}

static int vlcstream_file_eof( MXFFileSysData *p_sysData )
{
    /* TODO: check if eof supported and this is the way to get it */
    return stream_Size( p_sysData->p_stream ) <= stream_Tell( p_sysData->p_stream );
}

static int vlcstream_file_seek( MXFFileSysData *p_sysData, int64_t i_offset, int i_whence )
{
    int64_t i_fpos;

    if ( i_whence == SEEK_SET )
    {
        return stream_Seek( p_sysData->p_stream, i_offset ) == VLC_SUCCESS;
    }
    else if ( i_whence == SEEK_CUR )
    {
        i_fpos = stream_Tell( p_sysData->p_stream );
        return stream_Seek( p_sysData->p_stream, i_fpos + i_offset ) == VLC_SUCCESS;
    }
    else /* SEEK_END */
    {
        i_fpos = stream_Size( p_sysData->p_stream ) + i_offset;
        return stream_Seek( p_sysData->p_stream, i_fpos ) == VLC_SUCCESS;
    }

}

static int64_t vlcstream_file_tell( MXFFileSysData *p_sysData )
{
    return stream_Tell( p_sysData->p_stream );
}

static int vlcstream_file_is_seekable( MXFFileSysData *p_sysData )
{
    vlc_bool_t b_canSeek;

    stream_Control( p_sysData->p_stream, STREAM_CAN_SEEK, &b_canSeek );

    return b_canSeek == VLC_TRUE;
}

static void free_vlcstream_file( MXFFileSysData *p_sysData )
{
    if (p_sysData == NULL)
    {
        return;
    }

    free(p_sysData);
}


/*****************************************************************************
 * Create a MXF reader
 *****************************************************************************/
static int create_mxf_reader( stream_t *p_stream, MXFReader **pp_mxfReader )
{
    MXFFileSysData  *p_sysData = NULL;
    MXFFile *p_newMXFFile = NULL;

    /* wrap VLC input stream in MXF file */

    if ( ( p_sysData = malloc( sizeof( MXFFileSysData ) ) ) == NULL )
    {
        msg_Err( p_stream, "Out of memory" );
        return 0;
    }
    memset( p_sysData, 0, sizeof( MXFFileSysData ) );
    p_sysData->p_stream = p_stream;

    if ( ( p_newMXFFile = malloc( sizeof( MXFFile ) ) ) == NULL )
    {
        msg_Err( p_stream, "Out of memory" );
        free( p_sysData );
        return 0;
    }
    memset( p_newMXFFile, 0, sizeof( MXFFile ) );

    p_newMXFFile->close = vlcstream_file_close;
    p_newMXFFile->read = vlcstream_file_read;
    p_newMXFFile->write = vlcstream_file_write;
    p_newMXFFile->get_char = vlcstream_file_getchar;
    p_newMXFFile->put_char = vlcstream_file_putchar;
    p_newMXFFile->eof = vlcstream_file_eof;
    p_newMXFFile->seek = vlcstream_file_seek;
    p_newMXFFile->tell = vlcstream_file_tell;
    p_newMXFFile->is_seekable = vlcstream_file_is_seekable;
    p_newMXFFile->sysData = p_sysData;
    p_newMXFFile->free_sys_data = free_vlcstream_file;


    /* initialise MXF reader */

    if ( !init_mxf_reader( &p_newMXFFile, pp_mxfReader ) )
    {
        goto error;
    }

    return 1;

error:
    if ( p_newMXFFile != NULL )
    {
        mxf_file_close( &p_newMXFFile );
    }
    return 0;
}


/*****************************************************************************
 * Wrap the VLC byte array in an MXF file
 *****************************************************************************/

static void vlcbytearray_file_close( MXFFileSysData *p_sysData )
{
    p_sysData->p_data = NULL;
    p_sysData->i_size = -1;
    p_sysData->i_pos = -1;
}

static uint32_t vlcbytearray_file_read( MXFFileSysData *p_sysData, uint8_t *p_data, uint32_t i_count )
{
    uint32_t i_numRead;

    if ( p_sysData->i_pos >= p_sysData->i_size )
    {
        return 0;
    }

    if ( p_sysData->i_pos + i_count > p_sysData->i_size )
    {
        i_numRead = (uint32_t)(p_sysData->i_size - p_sysData->i_pos);
    }
    else
    {
        i_numRead = i_count;
    }

    memcpy( p_data, &p_sysData->p_data[p_sysData->i_pos], i_numRead );
    p_sysData->i_pos += i_numRead;

    return i_numRead;
}

static uint32_t vlcbytearray_file_write( MXFFileSysData *p_sysData, const uint8_t *p_data, uint32_t i_count )
{
    /* not used and not implemented */
    return 0;
}

static int vlcbytearray_file_getchar( MXFFileSysData *p_sysData )
{
    if ( p_sysData->i_pos + 1 > p_sysData->i_size )
    {
        return EOF;
    }

    return p_sysData->p_data[p_sysData->i_pos++];
}

static int vlcbytearray_file_putchar( MXFFileSysData *p_sysData, int i_c )
{
    /* not used and not implemented */
    return 0;
}

static int vlcbytearray_file_eof( MXFFileSysData *p_sysData )
{
    if ( p_sysData->i_pos >= p_sysData->i_size )
    {
        return 1;
    }
    return 0;
}

static int vlcbytearray_file_seek( MXFFileSysData *p_sysData, int64_t i_offset, int i_whence )
{
    if ( i_whence == SEEK_SET )
    {
        if ( i_offset < 0 || i_offset >= p_sysData->i_size )
        {
            return 0;
        }
        p_sysData->i_pos = i_offset;
    }
    else if ( i_whence == SEEK_CUR )
    {
        if ( p_sysData->i_pos + i_offset < 0 || p_sysData->i_pos + i_offset >= p_sysData->i_size )
        {
            return 0;
        }
        p_sysData->i_pos += i_offset;
    }
    else /* SEEK_END */
    {
        if ( p_sysData->i_size < 1 )
        {
            return 0;
        }
        p_sysData->i_pos = p_sysData->i_size - 1;
    }

    return 1;
}

static int64_t vlcbytearray_file_tell( MXFFileSysData *p_sysData )
{
    return p_sysData->i_pos;
}

static int vlcbytearray_file_is_seekable( MXFFileSysData *p_sysData )
{
    return VLC_TRUE;
}

static void free_vlcbytearray_file( MXFFileSysData *p_sysData )
{
    if (p_sysData == NULL)
    {
        return;
    }

    free(p_sysData);
}


static int wrap_byte_array( demux_t *p_demux, byte_t *p_data, int64_t i_size, MXFFile **pp_mxfFile )
{
    MXFFileSysData  *p_sysData = NULL;
    MXFFile *p_newMXFFile = NULL;

    if ( ( p_sysData = malloc( sizeof( MXFFileSysData ) ) ) == NULL )
    {
        msg_Err( p_demux, "Out of memory" );
        return 0;
    }
    memset( p_sysData, 0, sizeof( MXFFileSysData ) );
    p_sysData->p_data = p_data;
    p_sysData->i_size = i_size;
    p_sysData->i_pos = 0;

    if ( ( p_newMXFFile = malloc( sizeof( MXFFile ) ) ) == NULL )
    {
        msg_Err( p_demux, "Out of memory" );
        free( p_sysData );
        return 0;
    }
    memset( p_newMXFFile, 0, sizeof( MXFFile ) );

    p_newMXFFile->close = vlcbytearray_file_close;
    p_newMXFFile->read = vlcbytearray_file_read;
    p_newMXFFile->write = vlcbytearray_file_write;
    p_newMXFFile->get_char = vlcbytearray_file_getchar;
    p_newMXFFile->put_char = vlcbytearray_file_putchar;
    p_newMXFFile->eof = vlcbytearray_file_eof;
    p_newMXFFile->seek = vlcbytearray_file_seek;
    p_newMXFFile->tell = vlcbytearray_file_tell;
    p_newMXFFile->is_seekable = vlcbytearray_file_is_seekable;
    p_newMXFFile->sysData = p_sysData;
    p_newMXFFile->free_sys_data = free_vlcbytearray_file;


    *pp_mxfFile = p_newMXFFile;
    return 1;
}


/*****************************************************************************
 * Get the fourcc code for the essence type identified by the label
 *****************************************************************************/
static int get_fourcc( const mxfUL *essenceContainerLabel, vlc_fourcc_t *fourcc, int isVideo )
{

    if (mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_50_625_50_defined_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_50_625_50_extended_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_50_625_50_picture_only) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_50_525_60_defined_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_50_525_60_extended_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_50_525_60_picture_only) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_40_625_50_defined_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_40_625_50_extended_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_40_625_50_picture_only) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_40_525_60_defined_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_40_525_60_extended_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_40_525_60_picture_only) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_30_625_50_defined_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_30_625_50_extended_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_30_625_50_picture_only) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_30_525_60_defined_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_30_525_60_extended_template) ) ||
        mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(D10_30_525_60_picture_only) ) )
    {
        if ( isVideo )
        {
            *fourcc = VLC_FOURCC('m','p','g','v');
            return 1;
        }

        *fourcc = VLC_FOURCC('a','r','a','w');
        return 1;
    }

    if ( isVideo )
    {
        if ( mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(IECDV_25_525_60_FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(IECDV_25_525_60_ClipWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(IECDV_25_625_50_FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(IECDV_25_625_50_ClipWrapped) ) )
        {
            *fourcc = VLC_FOURCC('d','v','s','d');
            return 1;
        }
        else if ( mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_25_525_60_FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_25_525_60_ClipWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_25_625_50_FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_25_625_50_ClipWrapped) ) )
        {
            /* Should be: *fourcc = VLC_FOURCC('d','v','2','5'); */
            *fourcc = VLC_FOURCC('d','v','s','d');
            return 1;
        }
        else if (mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_50_525_60_FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_50_525_60_ClipWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_50_625_50_FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(DVBased_50_625_50_ClipWrapped) ) )
        {
            /* Should be: *fourcc = VLC_FOURCC('d','v','5','0'); */
            *fourcc = VLC_FOURCC('d','v','s','d');
            return 1;
        }
        else if ( mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped) ) )
        {
            *fourcc = VLC_FOURCC('U','Y','V','Y');
            return 1;
        }
    }
    else
    {
        if ( mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(BWFFrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(BWFClipWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(AES3FrameWrapped) ) ||
            mxf_equals_ul( essenceContainerLabel, &MXF_EC_L(AES3ClipWrapped) ) )
        {
            *fourcc = VLC_FOURCC('a','r','a','w');
            return 1;
        }
    }

    return 0;
}

