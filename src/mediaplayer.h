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

#include <QObject>
#include <QPainter>
#include <QFlags>
#include <QMutex>

#include <gst/gst.h>
#include <gst/app/app.h>


// Gtreamer-based media player
class MediaPlayer : public QObject
{
    Q_OBJECT

    public:
        MediaPlayer( QObject * parent = 0 );
        ~MediaPlayer();

        // Player state
        enum State
        {
            StateReset,
            StatePlaying,
            StatePaused,
        };

        // Options used in loadMedia
        enum
        {
            LoadAudioStream = 0x1,
            LoadVideoStream = 0x2
        };

        // Capabilities used in getCapabilities and changeCapability
        enum Capability
        {
            CapChangeVolume = 0x1,
            CapChangePitch = 0x2,
            CapChangeTempo = 0x4
        };

        Q_DECLARE_FLAGS(Capabilities, Capability)

    signals:
        // The media file is loaded, and is ready to play
        // (this however does not guarantee it will be played)
        void    loaded();

        // The media file has finished naturally
        void    finished();

        // The media file cannot be loaded, or finished with an error
        void    error( QString text );

        // Media tags are available
        void    tagsChanged( QString artist, QString title );

        // Media duration changed/became available
        void    durationChanged();

        // For logging
        void    logging( const QString& facility, const QString& message );

    public:
        // Loads the media file, and plays audio, video or both
        virtual void    loadMedia( const QString &file, int load_options );

        // Loads the media file, and plays audio, video or both from a device.
        // Takes ownership of the device, and will delete it upon end
        virtual void    loadMedia( QIODevice * device, int load_options );

        //
        // Player actions
        //
        virtual void    play();
        virtual void    pause();
        virtual void    seekTo( qint64 pos );
        virtual void    stop();

        // Reports player media position and duration. Returns -1 if unavailable.
        virtual qint64  position();
        virtual qint64  duration();

        // Reports current player state
        virtual State   state() const;

        // Gets the media tags (if available)
        virtual void    mediaTags( QString& artist, QString& title );

        // Sets the player capacity. The parameter is a value which differs:
        // CapChangeVolume: supports range from 0 (muted) - 100 (full)
        // CapChangePitch: supports range from -50 to +50 up to player interpretation
        // CapChangeTempo: supports range from -50 to +50 up to player interpretation
        virtual bool    setCapabilityValue( Capability cap, int value );

        // Returns the supported player capabilities, which are settable (if available)
        virtual Capabilities capabilities();

        // Draws a last video frame using painter p on a rect. Does nothing if video is
        // not played, or not available.
        virtual void    drawVideoFrame( QPainter& p, const QRect& rect );

    private:
        // Generic loader
        void    loadMediaGeneric();

        // Resets the pipeline
        void    reset();

        // Change the pipeline state, and emit error if failed
        void    setPipelineState( GstState state );

        // Stores the error, emits a message and prints into log
        void    reportError( const QString& text );

        // Logging through signal
        void    addlog( const char *type, const char * str, ... );

        bool    adjustPitch(int value);

        // Element creation
        GstElement * getElement(const QString &name );

        // Source callbacks
        static void cb_source_need_data( GstAppSrc *src, guint length, gpointer user_data );
        static gboolean cb_source_seek_data( GstAppSrc *src, guint64 offset, gpointer user_data );

        // Video sink callback
        static GstFlowReturn cb_new_sample( GstAppSink *appsink, gpointer user_data );

        // Bus callback
        static GstBusSyncReply cb_busMessageDispatcher( GstBus *bus, GstMessage *message, gpointer userData );

        QString     m_mediaFile;
        QString     m_errorMsg;
        QIODevice * m_mediaIODevice;

        gint64      m_duration;

        // Those are created objects
        GstElement *m_gst_pipeline;

        // Bus object
        GstBus   *  m_gst_bus;

        // Current tempo percentage rate; default value is 100%, allowed range is 75-125
        int         m_tempoRatePercent;
        qint64      m_lastKnownPosition;

        int         m_loadOptions;

        // Current pipeline state
        QAtomicInt  m_playState;
        bool        m_errorsDetected;
        bool        m_mediaLoading;

        QMutex      m_lastVideoSampleMutex;
        GstSample * m_lastVideoSample;

        // Media information from tags
        QString     m_mediaArtist;
        QString     m_mediaTitle;
};

#endif // MEDIAPLAYER_H
