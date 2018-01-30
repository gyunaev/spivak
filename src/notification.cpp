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

#include <math.h>
#include <stdint.h>

#include "settings.h"
#include "currentstate.h"
#include "songqueue.h"
#include "background.h"
#include "eventor.h"
#include "actionhandler.h"
#include "notification.h"

Notifications * pNotification;

Notifications::Notifications(QObject *parent)
    : QObject( parent )
{
    m_scrollStep = 3;
    m_lastScreenHeight = 0;
    m_notificationLine = m_firstItem = tr("Queue is empty");
    m_karaokePlaying = false;
    m_customMessageAnimationValue = 0.0;
    m_customMessageAnimationAdder = 0.025;

    connect( pEventor, &Eventor::karaokeStarted, this, &Notifications::songStarted );
    connect( pEventor, &Eventor::karaokeStopped, this, &Notifications::songStopped );
    connect( pEventor, &Eventor::queueChanged, this, &Notifications::queueUpdated );
    connect( pEventor, &Eventor::webserverUrlChanged, this, &Notifications::webServerUrlChanged, Qt::QueuedConnection );

    reset();
    updateWelcomeMessage();
}

Notifications::~Notifications()
{
}

qint64 Notifications::drawTop( KaraokePainter &p )
{
    // If the height change, see how large the font we could fit height-wise
    if ( m_lastScreenHeight != p.notificationRect().height() )
    {
        m_lastScreenHeight = p.notificationRect().height();
        p.tallestFontSize( m_font, m_lastScreenHeight );
        m_scrollOffset = INT32_MAX;
    }

    p.setPen( pSettings->notificationTopColor );
    p.setFont( m_font );

    int remainingms = -1;

    if ( pCurrentState->playerState != CurrentState::PLAYERSTATE_STOPPED )
        remainingms = qMax( 0, pCurrentState->playerDuration - pCurrentState->playerPosition );

    if ( remainingms == -1 || remainingms > 5000 )
    {
        int textwidth = p.fontMetrics().width( m_notificationLine );

        int drawpos = 0;

        // If the notification text fits into the line, we need no scrolling, so we only handle when it doesn't
        if ( textwidth > p.notificationRect().width() )
        {
            // We allow showing up to the whole width of the notification line minus 3/4 of the screen
            int maxshownwidth = p.fontMetrics().width( m_notificationLine ) - ((p.notificationRect().width() * 3) / 4 );

            // If scroll offset was reset or we scrolled past 3/4 of the last part, reset it
            if ( m_scrollOffset == INT32_MAX || (qAbs(m_scrollOffset) >= maxshownwidth ) )
            {
                // We start at 1/4 width
                m_scrollOffset = p.notificationRect().width() / 4;
            }
            else
            {
                // Speed is constant 3 here
                m_scrollOffset -= 3;
            }

            drawpos = m_scrollOffset;
        }
        else
            m_scrollOffset = INT32_MAX;

        // Draw the notification line
        p.drawText( drawpos, p.fontMetrics().ascent(), m_notificationLine );
    }
    else
        p.drawText( 0, p.fontMetrics().ascent(), m_firstItem );

    return 0;
}

qint64 Notifications::drawRegular(KaraokePainter &p)
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

void Notifications::queueUpdated()
{
    // Get the song list and currently scheduled song
    QList<SongQueueItem> queue = pSongQueue->exportQueue();
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
            if ( queue[i].state == SongQueueItem::STATE_PLAYING )
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

void Notifications::songStarted()
{
    QMutexLocker m( &m_mutex );
    m_karaokePlaying = true;
}

void Notifications::songStopped()
{
    QMutexLocker m( &m_mutex );
    m_karaokePlaying = false;
    updateWelcomeMessage();
}

void Notifications::settingsChanged()
{
    QMutexLocker m( &m_mutex );
    updateWelcomeMessage();
}

void Notifications::setOnScreenMessage(const QString &message)
{
    QMutexLocker m( &m_mutex );
    m_customMessage = message;
}

void Notifications::clearOnScreenMessage()
{
    QMutexLocker m( &m_mutex );
    m_customMessage.clear();
}

void Notifications::showMessage(const QString &message, int show)
{
    QMutexLocker m( &m_mutex );
    m_smallMessage = message;
    m_smallPercentage = -1;
    m_smallMessageExpires = QTime::currentTime().addMSecs( show );
}

void Notifications::showMessage(int percentage, const QString &message, int show)
{
    QMutexLocker m( &m_mutex );
    m_smallMessage = message;
    m_smallPercentage = percentage;
    m_smallMessageExpires = QTime::currentTime().addMSecs( show );
}

void Notifications::webServerUrlChanged(QString newurl)
{
    m_webserverURL = newurl;
    updateWelcomeMessage();
}

void Notifications::updateWelcomeMessage()
{
    m_customMessage = m_webserverURL.isEmpty() ? "Please select a song" : m_webserverURL;
}

void Notifications::reset()
{
    m_scrollOffset = INT32_MAX;
}
