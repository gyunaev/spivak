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

#ifndef EVENTOR_H
#define EVENTOR_H

#include <QObject>
#include <QString>

#include "songqueue.h"

//
// This class is the main information point - every event which may be useful for other components
// should result in an Eventor signal. Components connect to those signals and receive the information.
// This allows the subscribers to only have one single connection point, and not wonder which component
// can generate an event it needs.
//
// Examples: karaokeStarted, karaokePaused etc.
//
// Note 1: signals which are not likely to be used by other components (such as "User clicked File menu")
// should not be generating those signals.
//
// Note 2: this class signals events which already happened, i.e. playerStopped (when the player has been
// stopped already) and NOT playerStop (i.e. a signal for the player to stop). Those go to ActionHandler!
//
class Eventor : public QObject
{
    Q_OBJECT

    public:
        explicit Eventor(QObject *parent = 0);

    signals:
        //
        // Karaoke player events, emitted by KaraokeSong
        //

        // A Karaoke song started playing
        void    karaokeStarted( SongQueueItem song );

        // Karaoke song paused or resumed
        void    karaokePausedResumed( bool pause );

        // Karaoke song volume changed
        void    karaokeVolumeChanged( int newvalue );

        // Karaoke song ended naturally
        void    karaokeFinished();

        // Karaoke song was stopped manually, or ended naturally (and stop was called)
        void    karaokeStopped();

        // One of karaoke parameters (song pitch/tempo/voice removal status) changed
        void    karaokeParametersChanged();

        // Karaoke song failed to start
        //void    karaokeStarted( SongQueueItem song );

        //
        // Background events
        //

        // A new background object is being shown (either because the old one ended/expired,
        //1 or because the custom background was shown
        void    newBackgroundStarted( QString file );

        //
        // Music player events, emitted by MusicCollectionManager
        //

        // A new background music song started playing
        void    musicStarted();

        // Current background music song was paused or resumed
        void    musicPausedResumed( bool pause );

        // Music volume changed
        void    musicVolumeChanged( int newvalue );

        // Music tags became available or changed
        void    musicTagsChanged( QString artist, QString title );

        // Background music ended naturally
        void    musicFinished();

        // Background music was stopped manually, or ended naturally (and stop was called)
        void    musicStopped();



        //
        // Karaoke Queue events
        //

        // The queue content changed (new singer added, singer removed, song edited)
        void    queueChanged();


        //
        // Music Queue events
        //
        void    musicQueueChanged();


        //
        // Settings events
        //

        // Background settings changed (and something else might have changed as well)
        void    settingsChangedBackground();

        // Music collection settings changed (and something else might have changed as well)
        void    settingsChangedMusic();

        // Some settings changed
        void    settingsChanged();

        //
        // Karaoke collection scan events (connect via Queued!)
        //

        // Scan started
        void    scanCollectionStarted();

        // Scan in progress, details update
        void    scanCollectionProgress( QString progressinfo );

        // Scan finished (either completed or aborted)
        void    scanCollectionFinished();


        //
        // Web server events
        //

        // Web server URL changed. Either it was enabled, or disabled, or IP changed, or host name resolved.
        void    webserverUrlChanged( QString newurl );
};

extern Eventor * pEventor;

#endif // EVENTOR_H
