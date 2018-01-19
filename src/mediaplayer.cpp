/**************************************************************************
 *  Spivak Karaoke PLayer - a free, cross-platform desktop karaoke player *
 *  Copyright (C) 2015-2016 George Yunaev, support@ulduzsoft.com          *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *																	      *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <QtConcurrent>
#include <QApplication>

#include <gst/gst.h>

#include "pluginmanager.h"
#include "mediaplayer.h"
#include "logger.h"

// FIXME: part-missing-plugins.txt

MediaPlayer::MediaPlayer()
{
    m_gst_pipeline = 0;
    m_gst_bus = 0;

    m_gst_source = 0;
    m_gst_decoder = 0;
    m_gst_audioconverter = 0;
    m_gst_audio_volume = 0;
    m_gst_audiosink = 0;
    m_gst_audio_karaokesplitter = 0;
    m_gst_audio_tempo = 0;
    m_gst_audio_pitchadjust = 0;
    m_gst_video_colorconv = 0;
    m_gst_video_sink = 0;
    m_lastVideoSample = 0;
    m_mediaIODevice = 0;

    m_duration = -1;
    m_tempoRate = 1.0;
    m_playState = StateReset;
    m_errorsDetected = false;
    m_pitchPlugin = 0;
}

MediaPlayer::~MediaPlayer()
{    
    reset();
}

void MediaPlayer::loadMedia(const QString &file, MediaPlayer::LoadOptions options )
{
    reset();

    // Local file?
    if ( QFile::exists(file) )
        m_mediaFile = QUrl::fromLocalFile(file).toString();
    else
        m_mediaFile = file;

    m_mediaIODevice = 0;
    m_loadOptions = options;

    QtConcurrent::run( this, &MediaPlayer::threadLoadMedia );
}

void MediaPlayer::loadMedia(QIODevice *device, MediaPlayer::LoadOptions options)
{
    reset();

    m_loadOptions = options;
    m_mediaIODevice = device;

    QtConcurrent::run( this, &MediaPlayer::threadLoadMedia );
}

void MediaPlayer::play()
{
    setPipelineState( GST_STATE_PLAYING );
}

void MediaPlayer::pause()
{
    GstState state = GST_STATE(m_gst_pipeline);

    if ( state != GST_STATE_PAUSED )
        setPipelineState( GST_STATE_PAUSED );
}

void MediaPlayer::seekTo(qint64 pos)
{
    // Seek with m_tempoRate is also used to change the tempo (that's actually the only way in GStreamer)
    GstEvent * seek_event = gst_event_new_seek( m_tempoRate,
                                                GST_FORMAT_TIME,
                                                (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                                                GST_SEEK_TYPE_SET,
                                                pos * GST_MSECOND,
                                                GST_SEEK_TYPE_NONE,
                                                0 );

    gst_element_send_event( m_gst_pipeline, seek_event );
}

void MediaPlayer::stop()
{
    setPipelineState( GST_STATE_READY );
}

qint64 MediaPlayer::position()
{
    gint64 pos;

    if ( !gst_element_query_position( m_gst_pipeline, GST_FORMAT_TIME, &pos ) )
    {
        Logger::debug( "GstMediaPlayer: querying position failed" );
        return 0;
    }

    m_lastKnownPosition = pos / GST_MSECOND;

    return m_lastKnownPosition;
}

qint64 MediaPlayer::duration()
{
    // If we didn't know it yet, query the stream duration
    if ( m_duration == -1 )
    {
        gint64 dur;

        if ( !gst_element_query_duration( m_gst_pipeline, GST_FORMAT_TIME, &dur) )
            return -1;

        m_duration = dur / GST_MSECOND;
    }

    return m_duration;
}

MediaPlayer::State MediaPlayer::state() const
{
    return (State) m_playState.load();
}

void MediaPlayer::mediaTags(QString &artist, QString &title)
{
    artist = m_mediaArtist;
    title = m_mediaTitle;
}

bool MediaPlayer::setCapabilityValue( MediaPlayer::Capability cap, int value)
{
    switch ( cap )
    {
    case MediaPlayer::CapChangeVolume:
        if ( m_gst_audio_volume )
        {
            g_object_set( G_OBJECT(m_gst_audio_volume), "volume", (double) value / 100, NULL );
            return true;
        }
        break;

    case MediaPlayer::CapChangePitch:
        return adjustPitch( value );

    case MediaPlayer::CapChangeTempo:
        // Let's spread it to 0.75 ... +1.25
        m_tempoRate = (value / 200.0) + 0.75;
        position();
        seekTo( m_lastKnownPosition );
        break;

    case MediaPlayer::CapVoiceRemoval:
        return toggleKaraokeSplitter( value );
    }

    return false;
}

MediaPlayer::Capabilities MediaPlayer::capabilities()
{
    MediaPlayer::Capabilities caps( 0 );

    if ( m_gst_audio_volume )
        caps |= MediaPlayer::CapChangeVolume;

    if ( m_pitchPlugin )
        caps |= MediaPlayer::CapChangePitch;

    if ( m_gst_audio_tempo )
        caps |= MediaPlayer::CapChangeTempo;

    // This element is not precreated (as it is rarely needed)
    GstElement * splitter = gst_element_factory_make("audiokaraoke", "karaoketest" );

    if ( splitter )
    {
        g_object_unref( splitter );
        caps |= MediaPlayer::CapVoiceRemoval;
    }

    return caps;
}


bool MediaPlayer::adjustPitch( int newvalue )
{
    if ( !m_gst_audio_pitchadjust )
        return false;

    // Let's spread it to 0.5 ... +1.5
    double value = (newvalue / 200.0) + 0.75;

    g_object_set( G_OBJECT(m_gst_audio_pitchadjust), m_pitchPlugin->parameterName(), value, NULL );

    return true;
}

bool MediaPlayer::toggleKaraokeSplitter( int value )
{
    // Do nothing if the status did not change
    if ( (m_gst_audio_karaokesplitter && value) || (!m_gst_audio_karaokesplitter && !value) )
        return true;

    // To enable or disable the splitter, we insert or remove it.
    // The m_gst_karaokesplitter goes to a specific place which is between m_gst_audioconverter and m_gst_volume

    // To start we block the m_gst_audioconverter - this will trigger cb_event_probe_toggle_splitter() callback once it is blocked
    GstPad * srcpad = gst_element_get_static_pad( m_gst_audioconverter, "src" );
    gst_pad_add_probe( srcpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, cb_event_probe_toggle_splitter, this, NULL );

    return true;
}

void MediaPlayer::drawVideoFrame(QPainter &p, const QRect &rect)
{
    QMutexLocker m( &m_lastVideoSampleMutex );

    if ( !m_lastVideoSample )
        return;

    // get the snapshot buffer format now. We set the caps on the appsink so
    // that it can only be an rgb buffer.
    GstCaps *caps = gst_sample_get_caps( m_lastVideoSample );

    if ( !caps )
    {
        reportError( "could not get caps for the new video sample" );
        return;
    }

    GstStructure * structure = gst_caps_get_structure( caps, 0 );

    // We need to get the final caps on the buffer to get the size
    int width = 0;
    int height = 0;

    gst_structure_get_int( structure, "width", &width );
    gst_structure_get_int( structure, "height", &height );

    if ( !width || !height )
    {
        reportError( "could not get video height and width" );
        return;
    }

    // Create pixmap from buffer and save, gstreamer video buffers have a stride that
    // is rounded up to the nearest multiple of 4
    GstBuffer *buffer = gst_sample_get_buffer( m_lastVideoSample );
    GstMapInfo map;

    if ( !gst_buffer_map( buffer, &map, GST_MAP_READ ) )
    {
        reportError( "could not map video buffer" );
        return;
    }

    p.drawImage( rect, QImage( map.data, width, height, GST_ROUND_UP_4 (width * 4), QImage::Format_RGB32 ), QRect( 0, 0, width, height ) );

    // And clean up
    gst_buffer_unmap( buffer, &map );
}

void MediaPlayer::threadLoadMedia()
{
    m_duration = -1;
    m_errorsDetected = false;

    // Initialize gstreamer if not initialized yet
    if ( !gst_is_initialized() )
    {
        //qputenv( "GST_DEBUG", "*:4" );
#ifdef WIN32
        QString env = QString("GST_PLUGIN_PATH=%1\\gstreamer\\") .arg( QApplication::applicationDirPath() );
        env.replace( "/", "\\" );
        _putenv( qPrintable(env) );

        Logger::debug( "GstMediaPlayer: setting %s", qPrintable( env ) );
#endif
        gst_init(0, 0);
    }

    // Create the empty pipeline (this must be done first)
    m_gst_pipeline = gst_pipeline_new ("karaokepipeline");

    if ( !m_gst_pipeline )
    {
        reportError( "Pipeline could not be created." );
        return;
    }

    // Create the pipeline bus
    m_gst_bus = gst_element_get_bus( m_gst_pipeline );

    if ( !m_gst_bus )
    {
        reportError( "Pipeline bus could not be created." );
        return;
    }

    // Set the handler for the bus
    gst_bus_set_sync_handler( m_gst_bus, cb_busMessageDispatcher, this, 0 );

    // Create our media source, which could be either QIODevice/appsrc or a file
    // this also creates a decoder
    setupSource();

    // Those are mandatory
    if ( !m_gst_pipeline || !m_gst_source || !m_gst_decoder )
    {
        reportError( "Not all elements could be created." );
        return;
    }

    // Link and set up source and decoder if they are not the same object.
    if ( m_gst_source != m_gst_decoder )
    {
        if ( !gst_element_link( m_gst_source, m_gst_decoder ) )
        {
            reportError( "Cannot link source and decoder." );
            return;
        }
    }

    // If we do not have raw data, connect to the pad-added signal
    g_signal_connect( m_gst_decoder, "pad-added", G_CALLBACK (cb_pad_added), this );

    // Pre-create video elements if we need them
    if ( (m_loadOptions & MediaPlayer::LoadVideoStream) != 0 )
    {
        m_gst_video_colorconv = createElement( "videoconvert", "videoconvert" );
        m_gst_video_sink = createVideoSink();

        if ( !m_gst_video_colorconv || !m_gst_video_sink )
        {
            reportError( "Not all elements could be created." );
            return;
        }

        // Link the color converter and video sink
        if ( !gst_element_link( m_gst_video_colorconv, m_gst_video_sink ) )
        {
            reportError( "Cannor link video elements" );
            return;
        }
    }

    // Pre-create audio elements if we need them
    if ( (m_loadOptions & MediaPlayer::LoadAudioStream) != 0 )
    {
        // Load the pitch plugin if it is available
        m_pitchPlugin = pPluginManager->loadPitchChanger();

        // Create the audio elements, and add them to the bin
        m_gst_audioconverter = createElement ("audioconvert", "convert");
        m_gst_audio_volume = createElement("volume", "volume");
        m_gst_audiosink = createElement ("autoaudiosink", "sink");

        // Those are mandatory
        if ( !m_gst_audioconverter || !m_gst_audiosink || !m_gst_audio_volume )
        {
            reportError( "Not all elements could be created." );
            return;
        }

        // This one is optional, although it seems to be present everywhere
        m_gst_audio_tempo = createElement( "scaletempo", "tempo", false );

        // If we have the pitch changer
        if ( m_pitchPlugin && m_pitchPlugin->init() )
            m_gst_audio_pitchadjust = createElement( m_pitchPlugin->elementName(), "audiopitchchanger", false );
        else
            m_gst_audio_pitchadjust = 0;

        // Start linking
        bool linksucceed = true;
        GstElement * last = m_gst_audioconverter;

        if ( m_gst_audio_pitchadjust )
        {
            m_gst_audioconverter2 = createElement ("audioconvert", "convert2");

            linksucceed = gst_element_link_many( m_gst_audioconverter, m_gst_audio_pitchadjust, m_gst_audioconverter2, NULL );
            last = m_gst_audioconverter2;
        }

        // Now link in volume
        linksucceed = gst_element_link( last, m_gst_audio_volume );
        last = m_gst_audio_volume;

        // Now link in tempo if it is available
        if ( linksucceed && m_gst_audio_tempo )
        {
            linksucceed = gst_element_link( last, m_gst_audio_tempo );
            last = m_gst_audio_tempo;
        }

        // And finally the audio sink
        if ( linksucceed )
            linksucceed = gst_element_link( last, m_gst_audiosink );

        if ( !linksucceed )
        {
            reportError( "Audio elements could not be linked." );
            return;
        }
    }

    setPipelineState( GST_STATE_PAUSED );
}

void MediaPlayer::reset()
{
    if ( m_lastVideoSample )
    {
        gst_sample_unref( m_lastVideoSample );
        m_lastVideoSample = 0;
    }

    if ( m_gst_pipeline )
    {
        gst_element_set_state ( m_gst_pipeline, GST_STATE_NULL );

        // Clean up the bus and pipeline
        gst_bus_set_sync_handler( m_gst_bus, 0, 0, 0 );

        gst_object_unref( m_gst_bus );
        gst_object_unref( m_gst_pipeline );
    }

    delete m_mediaIODevice;
    m_mediaIODevice = 0;

    m_playState = StateReset;

    m_gst_pipeline = 0;
    m_gst_bus = 0;

    m_gst_source = 0;
    m_gst_decoder = 0;
    m_gst_audioconverter = 0;
    m_gst_audio_volume = 0;
    m_gst_audiosink = 0;
    m_gst_audio_karaokesplitter = 0;
    m_gst_audio_tempo = 0;
    m_gst_audio_pitchadjust = 0;
    m_gst_video_colorconv = 0;
    m_gst_video_sink = 0;
    m_lastVideoSample = 0;
    m_errorsDetected = false;
    m_mediaLoading = true;
    m_tempoRate = 1.0;
    m_pitchPlugin = 0;

    m_mediaArtist.clear();
    m_mediaTitle.clear();
}

void MediaPlayer::setPipelineState(GstState state)
{
    GstStateChangeReturn ret = gst_element_set_state( m_gst_pipeline, state );

    if ( ret == GST_STATE_CHANGE_FAILURE )
        reportError( QString("Unable to set the pipeline to the playing state") );
}

void MediaPlayer::reportError(const QString &text)
{
    Logger::error( "GstMediaPlayer: Reported error: %s", qPrintable(text));

    m_errorMsg = text;

    // Avoid sending multiple error messages; one is enough
    if ( !m_errorsDetected )
    {
        m_errorsDetected = true;
        QMetaObject::invokeMethod( this, "error", Qt::QueuedConnection, Q_ARG( QString, m_errorMsg ) );
    }
}

GstElement *MediaPlayer::createElement(const char *type, const char *name, bool mandatory)
{
    GstElement * e = gst_element_factory_make ( type, name );

    if ( !e )
    {
        if ( mandatory )
            reportError( QString("Cannot create element %1 for %2").arg( type ).arg(name) );

        return 0;
    }

    gst_bin_add( GST_BIN (m_gst_pipeline), e );
    return e;
}

GstElement *MediaPlayer::createVideoSink()
{
    GstElement * sink = createElement("appsink", "videosink");

    if ( !sink )
        return 0;

    // Set the caps - so far we only want our image to be RGB
    GstCaps *sinkCaps = gst_caps_new_simple( "video/x-raw", "format", G_TYPE_STRING, "BGRA", NULL );
    gst_app_sink_set_caps( GST_APP_SINK( sink ), sinkCaps );
    gst_caps_unref(sinkCaps);

    // Set up the callbacks
    GstAppSinkCallbacks callbacks = { 0, 0, 0, 0, 0 };
    callbacks.new_sample = cb_new_sample;

    gst_app_sink_set_callbacks( GST_APP_SINK(sink), &callbacks, this, NULL );
    return sink;
}


/* This function will be called by the pad-added signal */
void MediaPlayer::cb_pad_added (GstElement *src, GstPad *new_pad, MediaPlayer * self )
{
    GstPad *sink_pad = 0;
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    Logger::debug( "GstMediaPlayer: received new pad '%s' from '%s'", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src) );

    /* Check the new pad's type */
    new_pad_caps = gst_pad_query_caps (new_pad, NULL);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);

    if ( g_str_has_prefix (new_pad_type, "video/x-raw") )
    {
        if ( (self->m_loadOptions & MediaPlayer::LoadVideoStream) == 0 )
        {
            Logger::debug( "GstMediaPlayer:  Stream has video type, but video is not enabled, ignoring." );
            goto exit;
        }

        // Link the decoder pad with the color converter
        sink_pad = gst_element_get_static_pad ( self->m_gst_video_colorconv, "sink");

        // If our converter is already linked, we have nothing to do here
        if ( gst_pad_is_linked (sink_pad) )
        {
            Logger::debug( "GstMediaPlayer:  We are already linked. Ignoring.");
            goto exit;
        }

        // Attempt the link
        ret = gst_pad_link( new_pad, sink_pad );

        if ( GST_PAD_LINK_FAILED (ret) )
        {
            self->reportError( "link failed" );
        }
        else
        {
            Logger::debug( "GstMediaPlayer:   Video link succeeded (type '%s').", new_pad_type );
        }
    }
    else if ( g_str_has_prefix (new_pad_type, "audio/x-raw") )
    {
        if ( (self->m_loadOptions & MediaPlayer::LoadAudioStream) == 0 )
        {
            Logger::debug( "GstMediaPlayer:  Stream has audio type, but audio is not enabled, ignoring." );
            goto exit;
        }

        // Connect the pads
        sink_pad = gst_element_get_static_pad (self->m_gst_audioconverter, "sink");

        // Attempt the link
        ret = gst_pad_link (new_pad, sink_pad);

        if (GST_PAD_LINK_FAILED (ret))
        {
            Logger::debug( "GstMediaPlayer:  Audio link failed (type '%s').", new_pad_type );
        }
        else
        {
            Logger::debug( "GstMediaPlayer:  Audio link succeeded (type '%s').", new_pad_type );
        }
    }
    else
        Logger::debug( "GstMediaPlayer:  It has type '%s' which is not handled here, ignoring", new_pad_type );

exit:
    // Unreference the new pad's caps, if we got them
    if (new_pad_caps != NULL)
        gst_caps_unref (new_pad_caps);

    // Unreference the sink pad
    if ( sink_pad )
        gst_object_unref (sink_pad);
}

void MediaPlayer::setupSource()
{
    // If we got QIODevice, we use appsrc
    if ( m_mediaIODevice )
    {
        m_gst_source = createElement("appsrc", "source");

        if ( m_mediaIODevice->isSequential() )
        {
            gst_app_src_set_size( GST_APP_SRC(m_gst_source), -1 );
            gst_app_src_set_stream_type( GST_APP_SRC(m_gst_source), GST_APP_STREAM_TYPE_STREAM );
        }
        else
        {
            gst_app_src_set_size( GST_APP_SRC(m_gst_source), m_mediaIODevice->size() );
            gst_app_src_set_stream_type( GST_APP_SRC(m_gst_source), GST_APP_STREAM_TYPE_SEEKABLE );
        }

        GstAppSrcCallbacks callbacks = { 0, 0, 0, 0, 0 };
        callbacks.need_data = &MediaPlayer::cb_source_need_data;
        callbacks.seek_data = &MediaPlayer::cb_source_seek_data;

        gst_app_src_set_callbacks( GST_APP_SRC(m_gst_source), &callbacks, this, 0 );

        // Our sources have bytes format
        g_object_set( m_gst_source, "format", GST_FORMAT_BYTES, NULL);

        m_gst_decoder = createElement ("decodebin", "decoder");
    }
    else
    {
        // For a regular file we do not need appsrc and decodebin
        m_gst_source = createElement("uridecodebin", "source");

        // We already set decoder so it is not created
        m_gst_decoder = m_gst_source;

        g_object_set( m_gst_source, "uri", m_mediaFile.toUtf8().constData(), NULL);
    }
}

void MediaPlayer::cb_source_need_data(GstAppSrc *src, guint length, gpointer user_data)
{
    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );
    qint64 totalread = 0;

    if ( !self->m_mediaIODevice->atEnd() )
    {
        GstBuffer * buffer = gst_buffer_new_and_alloc( length );

        GstMapInfo map;
        gst_buffer_map( buffer, &map, GST_MAP_WRITE );

        totalread = self->m_mediaIODevice->read( (char*) map.data, length );
        gst_buffer_unmap( buffer, &map );

        if ( totalread > 0)
        {
            GstFlowReturn ret = gst_app_src_push_buffer( src, buffer );

            if (ret == GST_FLOW_ERROR) {
                qWarning()<<"appsrc: push buffer error";
            } else if (ret == GST_FLOW_FLUSHING) {
                qWarning()<<"appsrc: push buffer wrong state";
            }
        }
    }

    // We need to tell GStreamer this is end of stream
    if ( totalread <= 0 )
        gst_app_src_end_of_stream( src );
}

gboolean MediaPlayer::cb_source_seek_data(GstAppSrc *, guint64 offset, gpointer user_data)
{
    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );

    // If we operate in raw mode, the offset is time - we should convert to the file offset
    return self->m_mediaIODevice->seek( offset );
}

GstFlowReturn MediaPlayer::cb_new_sample(GstAppSink *appsink, gpointer user_data)
{
    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );
    GstSample * sample = gst_app_sink_pull_sample(  appsink );

    if ( sample )
    {
        QMutexLocker m( &self->m_lastVideoSampleMutex );

        if ( self->m_lastVideoSample )
            gst_sample_unref( self->m_lastVideoSample );

        self->m_lastVideoSample = sample;
    }

    return GST_FLOW_OK;
}

GstPadProbeReturn MediaPlayer::cb_event_probe_toggle_splitter(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );

    // remove the probe first
    gst_pad_remove_probe( pad, GST_PAD_PROBE_INFO_ID (info) );

    // Is the element already in the bin?
    if ( self->m_gst_audio_karaokesplitter == 0 )
    {
        Logger::debug( "GstMediaPlayer:  karaokesplitter is not enabled, enabling");
        self->m_gst_audio_karaokesplitter = self->createElement ("audiokaraoke", "karaoke", false );

        // This might happen if the player requested it despite us returning no such capability
        if ( !self->m_gst_audio_karaokesplitter )
            return GST_PAD_PROBE_OK;

        // Add splitter into the bin
        gst_bin_add( GST_BIN (self->m_gst_pipeline), self->m_gst_audio_karaokesplitter );

        // Unlink the place for the splitter
        gst_element_unlink( self->m_gst_audioconverter, self->m_gst_audio_volume );

        // Link it in
        gst_element_link_many( self->m_gst_audioconverter, self->m_gst_audio_karaokesplitter, self->m_gst_audio_volume, NULL );

        // And start playing it
        gst_element_set_state( self->m_gst_audio_karaokesplitter, GST_STATE_PLAYING );

        Logger::debug( "GstMediaPlayer: karaoke splitter enabled");
    }
    else
    {
        Logger::debug( "GstMediaPlayer: karaokesplitter is enabled, disabling");

        // Stop the splitter
        gst_element_set_state( self->m_gst_audio_karaokesplitter, GST_STATE_NULL );

        // Remove splitter from the bin (it unlinks it too)
        gst_bin_remove( GST_BIN (self->m_gst_pipeline), self->m_gst_audio_karaokesplitter );
        self->m_gst_audio_karaokesplitter = 0;

        // And link the disconnected elements again
        gst_element_link_many( self->m_gst_audioconverter, self->m_gst_audio_volume, NULL );

        Logger::debug( "GstMediaPlayer: karaoke splitter disabled");
    }

    return GST_PAD_PROBE_OK;
}

GstBusSyncReply MediaPlayer::cb_busMessageDispatcher( GstBus *bus, GstMessage *msg, gpointer user_data )
{
    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );
    Q_UNUSED(bus);

    GError *err;
    gchar *debug_info;

    if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR )
    {
        gst_message_parse_error (msg, &err, &debug_info);

        self->reportError( QString("GStreamer error received from element %1: %2, debug info: %3")
                           .arg( GST_OBJECT_NAME (msg->src) )
                           .arg( err->message)
                           .arg( debug_info ? debug_info : "none" ) );

        g_clear_error( &err );
        g_free( debug_info );
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_DURATION_CHANGED )
    {
        Logger::debug( "GstMediaPlayer: duration changed message" );
        self->m_duration = -1;

        // Call the signal invoker
        QMetaObject::invokeMethod( self,
                                   "durationChanged",
                                   Qt::QueuedConnection );
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS )
    {
        Logger::debug( "GstMediaPlayer: media playback finished naturally, emitting finished()" );
        QMetaObject::invokeMethod( self, "finished", Qt::QueuedConnection );
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_STATE_CHANGED )
    {
        GstState old_state, new_state, pending_state;

        gst_message_parse_state_changed( msg, &old_state, &new_state, &pending_state );

        // We are only interested in state-changed messages from the pipeline
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (self->m_gst_pipeline))
        {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

            Logger::debug( "GstMediaPlayer: pipeline state changed from %s to %s, pending %s",
                           gst_element_state_get_name (old_state),
                           gst_element_state_get_name (new_state),
                           gst_element_state_get_name (pending_state) );

            switch ( new_state )
            {
                case GST_STATE_PLAYING:
                    self->m_playState = (State) StatePlaying;
                    break;

                case GST_STATE_PAUSED:
                    self->m_playState = (State) StatePaused;

                    if ( self->m_mediaLoading )
                    {
                        self->m_mediaLoading = false;

                        Logger::debug( "GstMediaPlayer: Media state set to PAUSED, %s",
                                       self->m_errorsDetected ? "but errors were detected, no event" : "sending loaded event");

                        if ( !self->m_errorsDetected )
                            QMetaObject::invokeMethod( self, "loaded", Qt::QueuedConnection );
                    }
                    break;

                case GST_STATE_READY:
                    self->m_playState = (State) StateStopped;
                    break;

                default:
                    Logger::error( "GStreamerPlayer: warning unhandled state %d", new_state );
                    break;
            }
        }
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_TAG )
    {
        GstTagList *tags = 0;
        gchar *value;

        gst_message_parse_tag( msg, &tags );

        if ( self->m_mediaArtist.isEmpty() && gst_tag_list_get_string( tags, "artist", &value ) )
        {
            Logger::debug( "GstMediaPlayer: got artist tag %s", value );
            self->m_mediaArtist = QString::fromUtf8( value );
            g_free( value );
        }

        if ( self->m_mediaTitle.isEmpty() && gst_tag_list_get_string( tags, "title", &value ) )
        {
            Logger::debug( "GstMediaPlayer: got title tag %s", value );
            self->m_mediaTitle = QString::fromUtf8( value );
            g_free( value );
        }

        // Call the signal invoker if both tags are present
        if ( !self->m_mediaTitle.isEmpty() && !self->m_mediaArtist.isEmpty() )
            QMetaObject::invokeMethod( self,
                                       "tagsChanged",
                                       Qt::QueuedConnection,
                                       Q_ARG( QString, self->m_mediaArtist ),
                                       Q_ARG( QString, self->m_mediaTitle ) );

        gst_tag_list_unref( tags );
    }

    gst_message_unref (msg);
    return GST_BUS_DROP;
}
