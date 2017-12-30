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

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QFlags>
#include <QObject>
#include <QFuture>
#include <QPainter>

#include <gst/gst.h>
#include <gst/app/app.h>

#include "interface_mediaplayer.h"

// This media player is used by the app. However it doesn't implement anything itself,
// it is just a front Qt-style interface (with signals and slots) for the interface.
class MediaPlayer : public QObject
{
    Q_OBJECT

    public:
        MediaPlayer();
        ~MediaPlayer();

        // Player state
        enum State
        {
            StateReset,
            StateStopped,
            StatePlaying,
            StatePaused,
        };

        // Options used in loadMedia
        enum LoadOption
        {
            LoadAudioStream = 0x1,
            LoadVideoStream = 0x2
        };

        // Capabilities used in getCapabilities and changeCapability
        enum Capability
        {
            CapChangeVolume = 0x1,
            CapChangePitch = 0x2,
            CapChangeTempo = 0x4,
            CapVoiceRemoval = 0x8,
        };

        Q_DECLARE_FLAGS(LoadOptions, LoadOption)
        Q_DECLARE_FLAGS(Capabilities, Capability)

    signals:
        // The media file is loaded, and is ready to play (this however does not guarantee
        // it will be played)
        void    loaded();

        // The media file has finished naturally
        void    finished();

        // The media file cannot be loaded, or finished with an error
        void    error( QString text );

        // Media tags are available
        void    tagsChanged( QString artist, QString title );

        // Media duration changed/became available
        void    durationChanged();

    public slots:

        // Loads the media file, and plays audio, video or both
        void    loadMedia( const QString &file, LoadOptions options );

        // Loads the media file, and plays audio, video or both from a device.
        // Takes ownership of the device, and will delete it upon end
        void    loadMedia( QIODevice * device, MediaPlayer::LoadOptions options );

        //
        // Player actions
        //
        void    play();
        void    pause();
        void    seekTo( qint64 pos );
        void    stop();

        // Reports player media position and duration. Returns -1 if unavailable.
        qint64  position();
        qint64  duration();

        // Reports current player state
        State   state() const;

        // Gets the media tags (if available)
        void    mediaTags( QString& artist, QString& title );

        // Sets the player capacity. The parameter is a value which differs:
        // CapChangeVolume: supports range from 0 (muted) - 100 (full)
        // CapChangePitch: supports range from -50 to +50 up to player interpretation
        // CapChangeTempo: supports range from -50 to +50 up to player interpretation
        // CapVoiceRemoval: supports values 0 (disabled) or 1 (enabled)
        bool    setCapabilityValue( Capability cap, int value );

        // Returns the supported player capabilities, which are settable (if available)
        Capabilities capabilities();

        // Draws a last video frame using painter p on a rect. Does nothing if video is
        // not played, or not available.
        void    drawVideoFrame( QPainter& p, const QRect& rect );

        // Resets the pipeline
        void    reset();

    private:
        // Those three functions run concurrently, i.e. in a separate threads!
        void    threadLoadMedia();
        void    threadRunPipeline();

        // Sets up the source according to file type
        void    setupSource();

        // Change the pipeline state, and emit error if failed
        void    setPipelineState( GstState state );

        // Stores the error, emits a message and prints into log
        void    reportError( const QString& text );

        bool    adjustPitch(int value);
        bool    toggleKaraokeSplitter(int value);

        // Element creation
        GstElement * createElement( const char * type, const char * name, bool mandatory = true );
        GstElement * createVideoSink();

        // Source callbacks
        static void cb_source_need_data( GstAppSrc *src, guint length, gpointer user_data );
        static gboolean cb_source_seek_data( GstAppSrc *src, guint64 offset, gpointer user_data );

        // Video sink callback
        static GstFlowReturn cb_new_sample( GstAppSink *appsink, gpointer user_data );

        // Decoder pad handling callbacks
        static void cb_pad_added( GstElement *src, GstPad *new_pad, MediaPlayer *self );
        static void cb_no_more_pads( GstElement *src, MediaPlayer *self );

        // see http://gstreamer.freedesktop.org/data/doc/gstreamer/head/manual/html/section-dynamic-pipelines.html#section-dynamic-changing
        static GstPadProbeReturn cb_event_probe_toggle_splitter( GstPad * pad, GstPadProbeInfo * info, gpointer user_data );
        static GstPadProbeReturn cb_event_probe_add_pitcher( GstPad * pad, GstPadProbeInfo * info, gpointer user_data );

        // Bus callback
        static GstBusSyncReply cb_busMessageDispatcher( GstBus *bus, GstMessage *message, gpointer userData );

        QString     m_mediaFile;
        QString     m_errorMsg;
        QIODevice * m_mediaIODevice;

        gint64      m_duration;

        // Those are created objects
        GstElement *m_gst_pipeline;
        GstElement *m_gst_source;
        GstElement *m_gst_decoder;
        GstElement *m_gst_audioconverter;
        GstElement *m_gst_audioconverter2;
        GstElement *m_gst_audio_volume;
        GstElement *m_gst_audiosink;
        GstElement *m_gst_audio_karaokesplitter;
        GstElement *m_gst_audio_tempo;
        GstElement *m_gst_audio_pitchadjust;

        // For video playing
        GstElement *m_gst_video_colorconv;
        GstElement *m_gst_video_sink;

        // Bus object
        GstBus   *  m_gst_bus;

        // Current tempo rate
        double      m_tempoRate;
        qint64      m_lastKnownPosition;

        MediaPlayer::LoadOptions m_loadOptions;

        // Current pipeline state
        QAtomicInt  m_playState;
        bool        m_errorsDetected;
        bool        m_mediaLoading;

        QMutex      m_lastVideoSampleMutex;
        GstSample * m_lastVideoSample;

        // Media information from tags
        QString     m_mediaArtist;
        QString     m_mediaTitle;

        // If pitch changing plugin is available
        Interface_MediaPlayerPlugin *   m_pitchPlugin;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MediaPlayer::LoadOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(MediaPlayer::Capabilities)


#endif // MEDIAPLAYER_H
