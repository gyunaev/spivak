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

#include <QObject>
#include <QPainter>
#include <QCoreApplication>
#include <QFile>
#include <QUrl>

#include <gst/gst.h>

#include "mediaplayer.h"
#include "settings.h"
#include "logger.h"

// FIXME: part-missing-plugins.txt

MediaPlayer::MediaPlayer( QObject * parent )
    : QObject( parent )
{
    m_gst_pipeline = 0;
    m_gst_bus = 0;
    m_lastVideoSample = 0;
    m_mediaIODevice = 0;

    m_duration = -1;
    m_tempoRatePercent = 100;
    m_playState = StateReset;
    m_errorsDetected = false;
}

MediaPlayer::~MediaPlayer()
{
    reset();
}

void MediaPlayer::loadMedia(const QString &file, int load_options )
{
    reset();

    // Local file?
    m_mediaFile = file;
    m_mediaIODevice = 0;
    m_loadOptions = load_options;

    loadMediaGeneric();
}

void MediaPlayer::loadMedia(QIODevice *device, int load_options)
{
    reset();

    m_loadOptions = load_options;
    m_mediaIODevice = device;

    loadMediaGeneric();
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
    // Calculate the new tempo rate as it is used to change the tempo (that's actually the only way in GStreamer)
    //
    double temporate = m_tempoRatePercent / 100.0;

    GstEvent * seek_event = gst_event_new_seek( temporate,
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
        addlog( "DEBUG",  "GstMediaPlayer: querying position failed" );
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

    return (m_duration * 100) / m_tempoRatePercent;
}

MediaPlayer::State MediaPlayer::state() const
{
    return (State) m_playState.loadAcquire();
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
        g_object_set( G_OBJECT( getElement("volume")), "volume", (double) value / 100, NULL );
        return true;

    case MediaPlayer::CapChangePitch:
        return adjustPitch( value );

    case MediaPlayer::CapChangeTempo:

        // The UI gives us tempo rate as percentage, from 0 to 100 with 50 being normal value.
        // Thus we convert it into 75% - 125% range
        m_tempoRatePercent = 75 + value / 2;
        addlog( "DEBUG",  "MediaPlayer_GStreamer: tempo change: UI %d -> player %d", value, m_tempoRatePercent );
        position();
        seekTo( m_lastKnownPosition );

        // Because the total song duration now changed, we need to inform the widgets
        QMetaObject::invokeMethod( this,
                                   "durationChanged",
                                   Qt::QueuedConnection );

        break;
    }

    return false;
}

MediaPlayer::Capabilities MediaPlayer::capabilities()
{
    MediaPlayer::Capabilities caps( 0 );

    if ( getElement("volume") )
        caps |= MediaPlayer::CapChangeVolume;

    if ( pSettings->isRegistered() && getElement(pSettings->registeredDigest) )
        caps |= MediaPlayer::CapChangePitch;

    if ( getElement("tempo") )
        caps |= MediaPlayer::CapChangeTempo;

    return caps;
}


bool MediaPlayer::adjustPitch( int newvalue )
{
    GstElement * pitch = getElement(pSettings->registeredDigest);

    if ( !pitch )
        return false;

    // Let's spread it to 0.5 ... +1.5
    double value = (newvalue / 200.0) + 0.75;

    g_object_set( G_OBJECT(pitch), "pitch", value, NULL );
    return true;
}

void MediaPlayer::drawVideoFrame(QPainter &p, const QRect &rect)
{
    QMutexLocker m( &m_lastVideoSampleMutex );

    if ( !m_lastVideoSample )
        return;

    // get the snapshot buffer format now. We had set the caps on the appsink,
    // so the content can only be an rgb buffer.
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

void MediaPlayer::loadMediaGeneric()
{
    m_duration = -1;
    m_errorsDetected = false;

    // Initialize gstreamer if not initialized yet
    if ( !gst_is_initialized() )
    {
#if defined (WIN32)
        qputenv( "GST_PLUGIN_PATH", qPrintable( QCoreApplication::applicationDirPath() ) );
#endif
        //qputenv( "GST_DEBUG", "*:4" );
        gst_init(0, 0);
    }

    // The content of the pipeline - which could be video-only, audio-only or audio-video
    // See https://gstreamer.freedesktop.org/documentation/tutorials/basic/gstreamer-tools.html
    QString pipeline;

    // Data source for our pipeline
    if ( m_mediaIODevice )
        pipeline = "appsrc name=iosource";
    else
        pipeline = "filesrc name=filesource";

    // Add a decoder with the name so it can be plugged in later
    pipeline += " ! decodebin name=decoder";

    // Video decoding part
    if ( (m_loadOptions & MediaPlayer::LoadVideoStream) != 0 )
       pipeline += " decoder. ! video/x-raw ! videoconvert ! video/x-raw,format=BGRA ! appsink name=videosink";

    // Audio decoding part
    if ( (m_loadOptions & MediaPlayer::LoadAudioStream) != 0 )
    {
       pipeline += " decoder. ! audio/x-raw ! audioconvert ! scaletempo name=tempo";

       if ( pSettings->isRegistered() )
           pipeline += " ! pitch name=" + pSettings->registeredDigest;

        pipeline += " ! volume name=volume ! audioconvert ! audioresample ! autoaudiosink";
    }

    Logger::debug( "Gstreamer: setting up pipeline: '%s'", qPrintable( pipeline ) );

    GError * error = 0;
    m_gst_pipeline = gst_parse_launch( qPrintable( pipeline ), &error );

    if ( !m_gst_pipeline )
    {
        Logger::error( "Gstreamer pipeline '%s' cannot be created; error %s\n", qPrintable( pipeline ), error->message );
        reportError( tr("Pipeline could not be created; most likely your GStreamer installation is incomplete.") );
        return;
    }

    //
    // Setup various elements if we have them
    //

    // Video sink - video output
    if ( getElement( "videosink" ) )
    {
        Logger::debug( "Gstreamer: setting up videosink");

        // Setup the video sink callbacks
        GstAppSinkCallbacks callbacks;
        memset( &callbacks, 0, sizeof(callbacks) );
        callbacks.new_sample = cb_new_sample;

        gst_app_sink_set_callbacks( GST_APP_SINK(getElement( "videosink" )), &callbacks, this, NULL );
    }

    // Input source - I/O device
    if ( getElement( "iosource" ) )
    {
        GstElement * gst_source = getElement( "iosource" );

        if ( m_mediaIODevice->isSequential() )
        {
            gst_app_src_set_size( GST_APP_SRC(gst_source), -1 );
            gst_app_src_set_stream_type( GST_APP_SRC(gst_source), GST_APP_STREAM_TYPE_STREAM );
        }
        else
        {
            gst_app_src_set_size( GST_APP_SRC(gst_source), m_mediaIODevice->size() );
            gst_app_src_set_stream_type( GST_APP_SRC(gst_source), GST_APP_STREAM_TYPE_SEEKABLE );
        }

        GstAppSrcCallbacks callbacks = { 0, 0, 0, 0, 0 };
        callbacks.need_data = &MediaPlayer::cb_source_need_data;
        callbacks.seek_data = &MediaPlayer::cb_source_seek_data;

        gst_app_src_set_callbacks( GST_APP_SRC(gst_source), &callbacks, this, 0 );

        // Our sources have bytes format
        g_object_set( gst_source, "format", GST_FORMAT_BYTES, NULL);
    }

    // Input source - file
    if ( getElement( "filesource" ) )
        g_object_set( getElement( "filesource" ), "location", m_mediaFile.toUtf8().constData(), NULL);

    // Get the pipeline bus - store it since it has to be "unref after usage"
    m_gst_bus = gst_element_get_bus( m_gst_pipeline );

    if ( !m_gst_bus )
    {
        reportError( "Can't obtrain the pipeline bus." );
        return;
    }

    // Set the handler for the bus
    gst_bus_set_sync_handler( m_gst_bus, cb_busMessageDispatcher, this, 0 );

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
    m_lastVideoSample = 0;
    m_errorsDetected = false;
    m_mediaLoading = true;
    m_tempoRatePercent = 100;

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
    addlog( "ERROR", "GstMediaPlayer: Reported error: %s", qPrintable(text));

    m_errorMsg = text;

    // Avoid sending multiple error messages; one is enough
    if ( !m_errorsDetected )
    {
        m_errorsDetected = true;
        QMetaObject::invokeMethod( this, "error", Qt::QueuedConnection, Q_ARG( QString, m_errorMsg ) );
    }
}

GstElement *MediaPlayer::getElement(const QString& name)
{
    if ( !m_gst_pipeline )
        return 0;

    return gst_bin_get_by_name (GST_BIN(m_gst_pipeline), qPrintable(name));
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
                self->addlog( "WARNING", "appsrc: push buffer error" );
            } else if (ret == GST_FLOW_FLUSHING) {
                self->addlog( "WARNING", "appsrc: push buffer wrong state" );
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
        self->addlog( "DEBUG",  "GstMediaPlayer: duration changed message" );
        self->m_duration = -1;

        // Call the signal invoker
        QMetaObject::invokeMethod( self,
                                   "durationChanged",
                                   Qt::QueuedConnection );
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS )
    {
        self->addlog( "DEBUG",  "GstMediaPlayer: media playback finished naturally, emitting finished()" );
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

            self->addlog( "DEBUG",  "GstMediaPlayer: pipeline state changed from %s to %s, pending %s",
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

                        self->addlog( "DEBUG",  "GstMediaPlayer: Media state set to PAUSED, %s",
                                       self->m_errorsDetected ? "but errors were detected, no event" : "sending loaded event");

                        if ( !self->m_errorsDetected )
                            QMetaObject::invokeMethod( self, "loaded", Qt::QueuedConnection );
                    }
                    break;

                case GST_STATE_READY:
                    // meaningless for us
                    break;

                default:
                    self->addlog( "ERROR",  "GStreamerPlayer: warning unhandled state %d", new_state );
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
            self->addlog( "DEBUG",  "GstMediaPlayer: got artist tag %s", value );
            self->m_mediaArtist = QString::fromUtf8( value );
            g_free( value );
        }

        if ( self->m_mediaTitle.isEmpty() && gst_tag_list_get_string( tags, "title", &value ) )
        {
            self->addlog( "DEBUG",  "GstMediaPlayer: got title tag %s", value );
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

void MediaPlayer::addlog(const char *type, const char *fmt, ... )
{
    va_list vl;
    char buf[1024];

    va_start( vl, fmt );
    vsnprintf( buf, sizeof(buf) - 1, fmt, vl );
    va_end( vl );

    emit logging( type, buf );
}
