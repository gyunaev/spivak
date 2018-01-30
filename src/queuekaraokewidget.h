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

#ifndef QUEUEKARAOKEWIDGET_H
#define QUEUEKARAOKEWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QTreeWidget>
#include <QCloseEvent>

#include "songqueue.h"


namespace Ui {
class QueueKaraokeWidget;
}


// Queue and search models
class TableModelQueue;
class TableModelSearch;
class PlayerWidget;

class QueueKaraokeWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit QueueKaraokeWidget(QWidget *parent = 0);
        ~QueueKaraokeWidget();

        // Overriden functions
        void    show();
        void    hide();

    signals:
        void    closed(QObject*);

    public slots:
        // If the queue is updated remotely
        void    updateQueue();

        // To handle window closure
        void    closeEvent(QCloseEvent * event);

    private slots:
        // Dialog items
        void    buttonAdd();
        void    buttonEdit();
        void    buttonRemove();

        void    buttonSearch();

        void    findSingers( const QString& text );

        // Player controls
        void	clickedPlayerPlayPause();
        void	clickedPlayerStop();
        void	clickedPlayerPrev();
        void	clickedPlayerNext();

        // Volume slider has moved by the user
        void    movedPlayerVolumeSlider( int percentage );

        // Pitch spin has moved by the user
        void    pitchSpinChanged( int percentage );

        // Tempo spin has moved by the user
        void    tempoSpinChanged( int percentage );

        // Voice removal toggled
        void    toggledVoiceRemoval( bool enabled );

        // Timer-called update for player when we're playing
        void    timerUpdatePlayer();

        // Events from eventor
        void    karaokeStarted( SongQueueItem song );
        void    karaokePausedResumed( bool pause );
        void    karaokeVolumeChanged( int newvalue );
        void    karaokeStopped();

    private:
        // We want to keep this object always present (as it receives player notifications),
        // but we do not want to waste CPU and memory cycles on having it as hidden layout.
        // Hence we recreate the widget when it is shown, and free it when it is hidden.
        QWidget*       m_contentWidget;
        Ui::QueueKaraokeWidget *ui;
        PlayerWidget * m_player;

        // Actual models
        TableModelQueue *   m_modelQueue;
        TableModelSearch *  m_modelSearch;

        // Player information
        QString     m_playText;
        int         m_playerStatus;
        int         m_playerVolume;
        QTimer      m_updateTimer;
};


#endif // QUEUEKARAOKEWIDGET_H
