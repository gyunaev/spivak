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

#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QDockWidget>

#include "playerbutton.h"
#include "ui_playerwidget.h"

// Taken from my other project, Karaoke Lyric Editor
class PlayerWidget : public QWidget, public Ui::PlayerWidget
{
    Q_OBJECT

	public:
        enum
        {
            PLAYER_STOPPED,
            PLAYER_PAUSED,
            PLAYER_PLAYING,
        };

		PlayerWidget(QWidget *parent = 0);
        ~PlayerWidget();

	signals:
        // Player buttons clicked
        void	buttonPlayPause();
        void	buttonStop();
        void	buttonPrev();
        void	buttonNext();

        // Volume slider has moved by the user
        void    volumeSliderMoved( int percentage );

        // Pitch spin has moved by the user
        void    pitchSpinChanged( int percentage );

        // Tempo spin has moved by the user
        void    tempoSpinChanged( int percentage );

        // Voice removal toggled
        void    toggledVoiceRemoval( bool enabled );

	public slots:
        // Text showing the current player song or status
        void    setText( QString text );

        // Call to change the player status (default is STOPPED)
        void	setStatus( int status );

        // Call to update the progress
        void	setProgress( qint64 position, qint64 duration );

        // Call to update the volume
        void    setVolume( int percentage );

        // Call to update the Advanced window for current state capabilities
        void    applyCapabilities();
        void    hideCapabilities();

    private slots:
        void    spinPitchChanged( int val );
        void    spinTempoChanged( int val );

	private:
        int     m_spinSets;
};


#endif // PLAYERWIDGET_H
