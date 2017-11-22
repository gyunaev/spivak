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

#ifndef ACTIONHANDLER_H
#define ACTIONHANDLER_H

#include <QString>
#include <QKeyEvent>

#define HAS_DBUS_SUPPORT
#define HAS_LIRC_SUPPORT
#define HAS_HTTP_SUPPORT

class ActionHandler_WebServer;
class ActionHandler_LIRC;

//
// ActionHandler receives events (from keyboard, remote, dbus, webserver...) and translated them into actions which it emits as signals.
// For example, when you press STOP button on remote, the LIRC action handler gets it, translates into Action and posts it via cmdAction(ACTION_PLAYER_STOP)
// and the handler emits playerStop() to which everyone who needs it could connect.
//
class ActionHandler : public QObject
{
    Q_OBJECT

    public:
        enum Action
        {
            ACTION_PLAYER_START,
            ACTION_PLAYER_PAUSERESUME,
            ACTION_PLAYER_STOP,
            ACTION_PLAYER_SEEK_BACK,
            ACTION_PLAYER_SEEK_FORWARD,
            ACTION_PLAYER_VOLUME_UP,
            ACTION_PLAYER_VOLUME_DOWN,

            ACTION_QUEUE_NEXT,
            ACTION_QUEUE_PREVIOUS,
            ACTION_QUEUE_CLEAR,

            ACTION_LYRIC_EARLIER,   // decreases the music-lyric delay (negative too)
            ACTION_LYRIC_LATER,     // increases the music-lyric delay

            ACTION_PLAYER_RATING_DECREASE,
            ACTION_PLAYER_RATING_INCREASE,

            // Those might not be available depending on player/installation
            ACTION_PLAYER_PITCH_INCREASE,
            ACTION_PLAYER_PITCH_DECREASE,
            ACTION_PLAYER_TEMPO_INCREASE,
            ACTION_PLAYER_TEMPO_DECREASE,
            ACTION_PLAYER_TOGGLE_VOICE_REMOVAL,

            ACTION_MUSICPLAYER_START,

            ACTION_QUIT
        };

        ActionHandler();
        ~ActionHandler();

        // Post-create initialization/shutdown
        void    start();
        void    stop();

        // Looks up the event (for translations)
        static int actionByName( const char * eventname );

    signals:
        // Actions for Karaoke player
        void    actionKaraokePlayerStart();
        void    actionKaraokePlayerStop();  // do not emit! use karaokeStopAction()
        void    actionKaraokePlayerPauseResume();
        void    actionKaraokePlayerForward();
        void    actionKaraokePlayerBackward();
        void    actionKaraokePlayerLyricsEarlier();
        void    actionKaraokePlayerLyricsLater();
        void    actionKaraokePlayerRatingIncrease();
        void    actionKaraokePlayerRatingDecrease();
        void    actionKaraokePlayerPitchIncrease();
        void    actionKaraokePlayerPitchDecrease();
        void    actionKaraokePlayerTempoIncrease();
        void    actionKaraokePlayerTempoDecrease();
        void    actionKaraokePlayerToggleVoiceRemoval();
        void    actionKaraokePlayerChangePitch(int);   // from a slider or some kind of adaptive control
        void    actionKaraokePlayerChangeTempo(int);   // from a slider or some kind of adaptive control
        void    actionKaraokePlayerChangeVoiceRemoval(int); // from a slider or some kind of adaptive control


        // Actions for both players (music and karaoke)
        void    actionPlayerVolumeUp();
        void    actionPlayerVolumeDown();
        void    actionPlayerVolumeSet(int);     // from a slider or some kind of adaptive control

        // Actions for musc player - implemented in MusicCollectionManager
        //
        void    actionMusicPlayerPlay();
        void    actionMusicPlayerPauseResume();
        void    actionMusicPlayerStop();

        // Actions for music queue - implemented in MusicCollectionManager
        //
        void    actionMusicQueueNext();
        void    actionMusicQueuePrev();

        // Generic actions
        void    actionQuit();

        // Actions for queue manager
        void    actionQueueClear();

    public slots:
        // Those slots enqueue the song and start the queue automatically
        void    enqueueSong( QString singer, int id );
        void    enqueueSong( QString singer, QString path );
        void    dequeueSong(int id );

        // Reports an error or warning
        void    error( QString message );
        void    warning( QString message );

        // Most events should end up here
        bool    cmdAction( int event );

        // EVents from different sources
        void    keyEvent( QKeyEvent * event );

        // Settings changed
        void    settingsChanged();

    private slots:
        void    nextQueueAction();
        void    prevQueueAction();

        void    playerStartAction();
        void    playerStopAction();
        void    playerPauseAction();

        void    nextQueueItemMusic();
        void    nextQueueItemKaraoke();

    private:
        // Handlers for http and lirc
        ActionHandler_WebServer *   m_webserver;
        ActionHandler_LIRC      *   m_lircServer;

        static QMap< QString, ActionHandler::Action > m_actionNameMap;
};

extern ActionHandler * pActionHandler;

#endif // ACTIONHANDLER_H
