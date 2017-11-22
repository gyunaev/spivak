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

#include <QTime>
#include <QTimer>

#include "playerwidget.h"
#include "currentstate.h"
#include "actionhandler.h"
#include "util.h"

PlayerWidget::PlayerWidget(QWidget *parent)
    : QWidget(parent), Ui::PlayerWidget()
{
	setupUi(this);
    m_spinSets = 0;

	// Set up icons
	btnFwd->setPixmap( QPixmap(":images/dryicons_forward.png") );
	btnPausePlay->setPixmap( QPixmap(":images/dryicons_play.png") );
	btnRew->setPixmap( QPixmap(":images/dryicons_rewind.png") );
	btnStop->setPixmap( QPixmap(":images/dryicons_stop") );

	// Connect button slots
    connect( btnFwd, SIGNAL( clicked() ), this, SIGNAL(buttonNext()) );
    connect( btnRew, SIGNAL( clicked() ), this, SIGNAL(buttonPrev()) );
    connect( btnPausePlay, SIGNAL( clicked() ), this, SIGNAL(buttonPlayPause()) );
    connect( btnStop, SIGNAL( clicked() ), this, SIGNAL(buttonStop()) );

    connect( sliderVolume, SIGNAL(sliderMoved(int)), this, SIGNAL(volumeSliderMoved(int)) );

    // break the valueChanged loop
    // add prefix for the spins
    // adjust values into 0-100 from 100-x
    connect( spinPitch, SIGNAL(valueChanged(int)), this, SLOT(spinPitchChanged(int)) );
    connect( spinTempo, SIGNAL(valueChanged(int)), this, SLOT(spinTempoChanged(int)) );
    connect( boxVoiceRemoval, SIGNAL(clicked(bool)), this, SIGNAL(toggledVoiceRemoval(bool)) );

    sliderVolume->setMinimum( 0 );
    sliderVolume->setMaximum( 100 );
    seekBar->setRange( 0, 100 );

    setStatus( PLAYER_STOPPED );
}

PlayerWidget::~PlayerWidget()
{
}

void PlayerWidget::setText(QString text)
{
    lblInfo->setText( text );
}

void PlayerWidget::setStatus(int status)
{
    switch ( status )
    {
        case PLAYER_PAUSED:
            btnPausePlay->setPixmap( QPixmap(":images/dryicons_play.png") );
            break;

        case PLAYER_STOPPED:
            btnPausePlay->setPixmap( QPixmap(":images/dryicons_play.png") );
            btnFwd->setEnabled( true );
            btnRew->setEnabled( true );
            btnStop->setEnabled( false );
            btnPausePlay->setEnabled( true );

            seekBar->setValue( 0 );

            lblTimeCur->setText( "<b>---</b>" );
            lblTimeRemaining->setText( "<b>---</b>" );
            break;

        case PLAYER_PLAYING:
            btnPausePlay->setPixmap( QPixmap(":images/dryicons_pause.png") );
            btnFwd->setEnabled( true );
            btnRew->setEnabled( true );
            btnStop->setEnabled( true );
            break;
    }
}

void PlayerWidget::setProgress(qint64 pos, qint64 duration)
{
    int total = qMax( 1, (int) duration );
    int percent = (pos * 100) / total;
    int remaining = total - pos;

    lblTimeCur->setText( tr("<b>%1</b>") .arg( Util::tickToString( pos ) ) );
    lblTimeRemaining->setText( tr("<b>-%1</b>") .arg( Util::tickToString( remaining ) ) );
    seekBar->setValue( percent );
}

void PlayerWidget::setVolume(int percentage)
{
    sliderVolume->setValue( percentage );
}

void PlayerWidget::applyCapabilities()
{
    m_spinSets = 0;

    if ( pCurrentState->playerCapabilities & MediaPlayer::CapChangePitch )
    {
        spinPitch->setEnabled( true );

        // Move it 50 up, as our range is 0-100 and we have 50% in the middle
        if ( spinPitch->value() != pCurrentState->playerPitch + 50 )
        {
            // setValue will only trigger a signal (which we guard against with m_spinSets)
            // if the value is different from the current one
            spinPitch->setValue( pCurrentState->playerPitch + 50 );
            m_spinSets++;
        }
    }
    else
        spinPitch->setEnabled( false );

    if ( pCurrentState->playerCapabilities & MediaPlayer::CapChangeTempo )
    {
        spinTempo->setEnabled( true );

        // Move it 50 up, as our range is 0-100 and we have 50% in the middle
        if ( spinTempo->value() != pCurrentState->playerTempo + 50 )
        {
            // setValue will only trigger a signal (which we guard against with m_spinSets)
            // if the value is different from the current one
            spinTempo->setValue( pCurrentState->playerTempo + 50 );
            m_spinSets++;
        }
    }
    else
        spinTempo->setEnabled( false );

    if ( pCurrentState->playerCapabilities & MediaPlayer::CapVoiceRemoval )
    {
        boxVoiceRemoval->setEnabled( true );
        boxVoiceRemoval->setChecked( pCurrentState->playerVoiceRemovalEnabled );
    }
    else
        boxVoiceRemoval->setEnabled( false );
}

void PlayerWidget::hideCapabilities()
{
    groupAdvanced->hide();
}

void PlayerWidget::spinPitchChanged(int val)
{
    if ( m_spinSets > 0 )
        m_spinSets--;
    else
        emit pitchSpinChanged( val - 50 );
}

void PlayerWidget::spinTempoChanged(int val)
{
    if ( m_spinSets > 0 )
        m_spinSets--;
    else
        emit tempoSpinChanged( val - 50 );
}
