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

#include "karaokesong.h"
#include "playernotification.h"
#include "playerrenderer.h"
#include "karaokewidget.h"
#include "settings.h"
#include "currentstate.h"

static const unsigned int RENDER_FPS = 20;


PlayerRenderer::PlayerRenderer(PlayerNotification *notification, QImage * render, KaraokeWidget *parent)
    : QThread(parent)
{
    m_widget = parent;
    m_notification = notification;
    m_continue = 1;
    m_renderImage = render;
}

PlayerRenderer::~PlayerRenderer()
{
    stop();
}

void PlayerRenderer::stop()
{
    if ( m_continue )
    {
        m_continue.store( 0 );
        wait();
    }
}

void PlayerRenderer::run()
{
    // from http://www.koonsolo.com/news/dewitters-gameloop/
    // We do not need fast rendering, and need constant FPS, so 2nd solution works well
    QTime next_cycle;

    while ( m_continue )
    {
        next_cycle = QTime::currentTime().addMSecs( pCurrentState->msecPerFrame );

        // Prepare the image
        m_renderImage->fill( Qt::black );

        // Use this painter
        KaraokePainter p( m_renderImage );

        // Render the top area notification
        m_notification->drawTop( p );

        // Switch the painter to main area
        p.setClipAreaMain();

        m_widget->m_karaokeMutex.lock();

        // Draw background if we're not playing
        if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED || !m_widget->m_karaoke->hasCustomBackground() )
        {
            if ( m_widget->m_background )
                m_widget->m_background->draw( p );
        }

        // Draw background (possibly) and lyrics if we are
        if ( pCurrentState->playerState != CurrentState::PLAYERSTATE_STOPPED )
            m_widget->m_karaoke->draw( p );

        m_widget->m_karaokeMutex.unlock();

        // And subsequent notifications, if any (they must go on top of lyrics)
        m_notification->drawRegular( p );
        p.end();

        // And finally render it
        m_renderImage = m_widget->switchImages();

        // When should we have next tick?
        int remainingms = qMax( QTime::currentTime().msecsTo( next_cycle ), 5 );
        msleep( remainingms );
    }
}
