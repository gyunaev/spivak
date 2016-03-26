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

#include "settings.h"
#include "currentstate.h"
#include "songqueue.h"
#include "background.h"
#include "eventor.h"
#include "actionhandler.h"
#include "playernotification.h"

PlayerNotification * pNotification;

PlayerNotification::PlayerNotification(QObject *parent)
    : QObject( parent )
{
    m_scrollStep = 3;
    m_lastScreenHeight = 0;
    m_notificationLine = m_firstItem = tr("Queue is empty");
    m_karaokePlaying = false;
    m_customMessageAnimationValue = 0.0;
    m_customMessageAnimationAdder = 0.025;

    connect( pEventor, &Eventor::karaokeStarted, this, &PlayerNotification::songStarted );
    connect( pEventor, &Eventor::karaokeStopped, this, &PlayerNotification::songStopped );
    connect( pEventor, &Eventor::queueChanged, this, &PlayerNotification::queueUpdated );
    connect( pEventor, &Eventor::webserverUrlChanged, this, &PlayerNotification::webServerUrlChanged, Qt::QueuedConnection );

    reset();
    updateWelcomeMessage();
}

PlayerNotification::~PlayerNotification()
{
}

qint64 PlayerNotification::drawTop( KaraokePainter &p )
{
    // If the height change, see how large the font we could fit height-wise
    if ( m_lastScreenHeight != p.notificationRect().height() )
    {
        m_lastScreenHeight = p.notificationRect().height();
        p.tallestFontSize( m_font, m_lastScreenHeight );
    }

    p.setPen( pSettings->notificationTopColor );
    p.setFont( m_font );

    int remainingms = -1;

    if ( pCurrentState->playerState != CurrentState::PLAYERSTATE_STOPPED )
        remainingms = qMax( 0, pCurrentState->playerDuration - pCurrentState->playerPosition );

    if ( remainingms == -1 || remainingms > 5000 )
    {
        if ( m_scrollOffset >= p.fontMetrics().width( m_notificationLine[m_textOffset] ) )
        {
            m_scrollOffset = 0;
            m_textOffset++;

            if ( m_textOffset >= m_notificationLine.length() )
                m_textOffset = 0;
        }
        else
            m_scrollOffset++;

        p.drawText( -m_scrollOffset, p.fontMetrics().ascent(), m_notificationLine.mid( m_textOffset ) + "      " + m_notificationLine );
    }
    else
        p.drawText( 0, p.fontMetrics().ascent(), m_firstItem );

    return 0;
}

qint64 PlayerNotification::drawRegular(KaraokePainter &p)
{
    QTime now = QTime::currentTime();

    QMutexLocker m( &m_mutex );

    if ( !m_karaokePlaying && !m_customMessage.isEmpty() )
    {
        m_customFont.setPointSize( p.largestFontSize( m_customFont, pSettings->playerLyricsFontMaxSize, p.textRect().width(), m_customMessage ));

        p.setFont( m_customFont );
        p.drawCenteredOutlineTextGradient( 50, m_customMessageAnimationValue, m_customMessage );

        // Do a round-trip animation when the spot goes back and forth
        m_customMessageAnimationValue += m_customMessageAnimationAdder;

        if ( m_customMessageAnimationValue >= 1.0 )
        {
            m_customMessageAnimationValue = 1.0;
            m_customMessageAnimationAdder = -m_customMessageAnimationAdder;
        }
        else if ( m_customMessageAnimationValue < 0.0 )
        {
            m_customMessageAnimationValue = 0.0;
            m_customMessageAnimationAdder = -m_customMessageAnimationAdder;
        }
    }

    if ( !m_smallMessage.isEmpty() )
    {
        // Expires?
        int msleft = now.msecsTo( m_smallMessageExpires );

        if ( msleft > 0 )
        {
            // We will only take 5% of screen space for the message, but no less than 10px
            p.tallestFontSize( m_customFont, qMax( 10, p.textRect().height() / 20 ) );

            QColor color( pSettings->notificationCenterColor );

            // If less than 0.5 sec left - we fade it out
            if ( msleft <= 500 )
                color.setAlpha( (msleft * 255) / 500 );

            p.setFont( m_customFont );
            int height = p.fontMetrics().height();
            p.drawOutlineText( p.textRect().x(), p.textRect().y() + height, color, m_smallMessage );

            // Do we need to draw the percentage box as well?
            if ( m_smallPercentage != -1 )
            {
                QColor black( Qt::black );
                black.setAlpha( color.alpha() );

                // First draw it without filling
                unsigned int boxwidth = p.textRect().width() / 4;
                int startx = p.textRect().x() + p.fontMetrics().width( m_smallMessage + "aaa" );

                QPen pen( black );
                pen.setWidth( 3 );

                p.setPen( pen );
                p.setBrush( QBrush() );
                p.drawRect( startx, p.textRect().y(), boxwidth, height + p.fontMetrics().descent() );

                // Now filled according to percentage
                p.setBrush( color );
                p.drawRect( startx, p.textRect().y(), (m_smallPercentage * boxwidth) / 100, height + p.fontMetrics().descent() );
            }
        }
        else
            m_smallMessage.clear();
    }

    return 0;
}

void PlayerNotification::queueUpdated()
{
    // Get the song list and currently scheduled song
    QList<SongQueue::Song> queue = pSongQueue->exportQueue();
    int current = pSongQueue->currentItem();

    m_notificationLine.clear();
    m_firstItem.clear();
    m_textQueueSize.clear();

    if ( !queue.isEmpty() && current <= queue.size() - 1 )
    {
        m_notificationLine.clear();
        int elements = 0;

        for ( int i = current; i < queue.size(); i++ )
        {
            // Do not show the song if it is being played
            if ( queue[i].state == SongQueue::Song::STATE_PLAYING )
                continue;

            QString itemname = queue[i].title;

            if ( itemname.isEmpty() )
                itemname = queue[i].file;

            if ( elements == 0 )
            {
                if ( !queue[i].singer.trimmed().isEmpty() )
                    m_firstItem = tr("Next: %1 (%2)") .arg( itemname ) .arg( queue[i].singer );
                else
                    m_firstItem = tr("Next: %1") .arg( itemname );
            }

            // Trim down itemname if there are more than three items in queue and it is longer than 10 chars
            if ( queue.size() - current > 3 && itemname.length() > 10 )
                itemname = itemname.left( 8 ) + "..";

            if ( !queue[i].singer.trimmed().isEmpty() )
                m_notificationLine.append( tr("%1: %2 (%3) ").arg( elements + 1 ) .arg( itemname ) .arg( queue[i].singer ) );
            else
                m_notificationLine.append( tr("%1: %2 ").arg( elements + 1 ) .arg( itemname ) );

            elements++;
        }

        m_textQueueSize = tr("[%1]").arg( elements );
    }

    reset();
}

void PlayerNotification::songStarted()
{
    QMutexLocker m( &m_mutex );
    m_karaokePlaying = true;
}

void PlayerNotification::songStopped()
{
    QMutexLocker m( &m_mutex );
    m_karaokePlaying = false;
    updateWelcomeMessage();
}

void PlayerNotification::settingsChanged()
{
    QMutexLocker m( &m_mutex );
    updateWelcomeMessage();
}

void PlayerNotification::setOnScreenMessage(const QString &message)
{
    QMutexLocker m( &m_mutex );
    m_customMessage = message;
}

void PlayerNotification::clearOnScreenMessage()
{
    QMutexLocker m( &m_mutex );
    m_customMessage.clear();
}

void PlayerNotification::showMessage(const QString &message, int show)
{
    QMutexLocker m( &m_mutex );
    m_smallMessage = message;
    m_smallPercentage = -1;
    m_smallMessageExpires = QTime::currentTime().addMSecs( show );
}

void PlayerNotification::showMessage(int percentage, const QString &message, int show)
{
    QMutexLocker m( &m_mutex );
    m_smallMessage = message;
    m_smallPercentage = percentage;
    m_smallMessageExpires = QTime::currentTime().addMSecs( show );
}

void PlayerNotification::webServerUrlChanged(QString newurl)
{
    m_webserverURL = newurl;
    updateWelcomeMessage();
}

void PlayerNotification::updateWelcomeMessage()
{
    m_customMessage = m_webserverURL.isEmpty() ? "Please select a song" : m_webserverURL;
}

void PlayerNotification::reset()
{
    m_textOffset = 0;
    m_scrollOffset = 0;
}
