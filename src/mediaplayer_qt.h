/**************************************************************************
 *  Ulduzsoft Karaoke PLayer - cross-platform desktop karaoke player      *
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

#if 0

#ifndef UNIVERSALPLAYER_QMEDIAPLAYER_H
#define UNIVERSALPLAYER_QMEDIAPLAYER_H

#include <QMutex>
#include <QObject>
#include <QString>
#include <QAtomicInt>
#include <QMediaPlayer>

class KaraokePainter;
class QMediaPlayer;
class UniversalPlayerVideoSurface;
class UniversalPlayerID3StripedMediaFile;


// A universal player class - can play videos or music. Must support multiple instances
// (typically one plays video, and one plays music)
//
class UniversalPlayer_QMediaPlayer : public QObject
{
    Q_OBJECT

    public:
        explicit UniversalPlayer_QMediaPlayer(QObject *parent = 0);
        ~UniversalPlayer_QMediaPlayer();

        // Load the audio file, no video output is possible. Returns true on success (ready to start),
        // false on failure (see errorMsg() then)
        bool    loadAudio( const QString& musicfile );

        // Load the video file, video output is possible. Returns true on success (ready to start),
        // false on failure (see errorMsg() then)
        bool    loadVideo( const QString& videofile, bool muted );

        // Draws the video content (only for video player) on the image. For non-video does nothing.
        // This function is called from a different thread, so locking is necessary.
        void    draw( KaraokePainter& p );

    public slots:
        // Start playing (may be delayed until the media is loaded)
        void    play();

        // Stops playing; for video the rendering image is not updated anymore
        // (but still remains valid)
        void    stop();

        // Pause or resume
        void    pauseOrResume( bool pause );

        // Seek to position
        void    seekTo( qint64 time );

        // Current position
        qint64  position();

        // Volume getter/setter
        void    setVolume(int volume);
        int     volume();

        // Media duration, if known
        qint64  duration();

        // Error message, if any
        QString errorMsg() const { return m_errorMsg; }

        // Is valid player?
        bool    isValid() const;

        // Is it paused?
        bool    isPaused() const;

        // Returns metadata from the music file
        void    infoFromFile( QString& artist, QString& title );

        // Sets the fadein/fadeout duration
        void    setFadeDuration( unsigned int duration );

    signals:
        // Media is finished
        void    finished();

        // Media is started
        void    started( qint64 duration );

        // Media has been loaded, and ready to be started (connect it to play() for autostart)
        void    loaded();

        // Media has not been loaded (if during loading), or somehow play aborted (if during playing).
        // errormsg() contains the text. Playing is aborted/not started, and loaded() will never be emitted.
        void    error();

    private slots:
        void	slotError( QMediaPlayer::Error error );
        void    slotMediaStatusChanged(QMediaPlayer::MediaStatus status);
        void    fadeVolume();

    private:
        void    newMediaPlayer( bool video, bool muted );

        // Media file name and error message (used mostly for reporting)
        QString         m_mediaFile;
        QString         m_errorMsg;

        // The actual player
        QMediaPlayer  * m_player;

        // If video playing is enabled, it draws on this surface
        UniversalPlayerVideoSurface *  m_surface;

        // Tracks playing status (for draw() calls) - mutex prevents destroying the player while drawing
        QAtomicInt      m_drawEnabled;
        QMutex          m_drawMutex;

        // For crossfading: volume decrease/increase step; if zero we're not crossfading
        int             m_crossfadeOrigVolume;
        int             m_crossfadeVolumeStep;
        int             m_crossfadeDelay;

        // Is the player paused?
        bool            m_isPaused;

        // Is the player ended with an error
        bool            m_hasErrors;

        // The QIODevice we're playing from
        UniversalPlayerID3StripedMediaFile  *   m_deviceMusicFile;
};

#endif // UNIVERSALPLAYER_QMEDIAPLAYER_H

#endif
