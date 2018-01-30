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

#ifndef QUEUEMUSICWIDGET_H
#define QUEUEMUSICWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QCloseEvent>

#include "songqueue.h"


namespace Ui {
class QueueMusicWidget;
}

class TableModelMusic;
class PlayerWidget;

class QueueMusicWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit QueueMusicWidget(QWidget *parent = 0);
        ~QueueMusicWidget();

        // Overriden functions
        void    show();
        void    hide();

    signals:
        void    closed(QObject*);

    private slots:
        void    updateCollection();
        void    buttonAddMusic();
        void    buttonRemoveMusic();
        void    findTextChanged( QString find );

        // Player controls work through ActionHandler:
        // - Play is clicked;
        // - an action is generated for ActionHandler
        // - it emits a signal which goes to MusicCollectionManager
        // - MusicCollectionManager starts a song
        // - MusicCollectionManager emits a musicStarted() via Emitter
        // - We receive musicStarted() and update the player controls
        void	clickedPlayerPlayPause();
        void	clickedPlayerStop();
        void	clickedPlayerPrev();
        void	clickedPlayerNext();

        // Volume slider has moved by the user
        void    movedPlayerVolumeSlider( int percentage );

        // Timer-called update for player when we're playing
        void    timerUpdatePlayer();

        // Events from eventor
        void    musicStarted();
        void    musicPausedResumed( bool pause );
        void    musicVolumeChanged( int newvalue );
        void    musicFinished();
        void    musicTagsChanged( QString artist, QString title );

        // Notification slots (so we can turn off the player when song is started, and then turn it back)
        void    karaokeStarted( SongQueueItem song );
        void    karaokeStopped();

        // To handle window closure
        void    closeEvent(QCloseEvent * event);

    private:

        // We want to keep this object always present (as it receives player notifications),
        // but we do not want to waste CPU and memory cycles on having it as hidden layout.
        // Hence we recreate the widget when it is shown, and free it when it is hidden.
        QWidget*       m_contentWidget;
        Ui::QueueMusicWidget *ui;
        PlayerWidget * m_player;

        TableModelMusic *   m_model;

        // Player information
        QString     m_playText;
        int         m_playerStatus;
        int         m_playerVolume;
        QTimer      m_updateTimer;
        bool        m_karaokePlaying;
};

#endif // QUEUEMUSICWIDGET_H
