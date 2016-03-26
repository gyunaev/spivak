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

#include <QDebug>
#include <math.h>

#include "logger.h"
#include "actionhandler.h"
#include "settings.h"
#include "currentstate.h"
#include "backgroundimage.h"

#ifndef M_PI
    # define M_PI		3.14159265358979323846	/* pi */
#endif

BackgroundImage::BackgroundImage()
{
    m_percentage = 0;
    m_customBackground = false;
    m_playbackPaused = 0;

    m_animationSpeed = 5;
}

bool BackgroundImage::initFromSettings()
{
    // Do we have a directory path or file?
    if ( pCurrentState->currentBackgroundObject().isEmpty() )
        return false;

    // Load the first image
    loadNewImage();
    return true;
}

bool BackgroundImage::initFromFile( QIODevice * file, const QString& filename )
{
    // Find the file extension
    int p = filename.lastIndexOf( '.' );

    if ( p == -1 )
        return false;

    if ( !m_currentImage.load( file, qPrintable( filename.mid( p + 1) ) ) )
        return false;

    m_customBackground = true;
    return true;
}

void BackgroundImage::pause(bool pausing)
{
    m_playbackPaused = pausing ? 1 : 0;
}

qint64 BackgroundImage::draw(KaraokePainter &p)
{
    // Transition
    if ( !m_newImage.isNull() )
    {
        if ( !m_currentImage.isNull() && performTransition(p) )
            return 0;

        // No transition or it is finished
        m_currentImage = m_newImage;
        m_newImage = QImage();
    }

    if ( !m_currentImage.isNull() )
    {
        // We only animate if the image is at least 20% larger than display resolution
        if ( m_animationSpeed > 0
        && (double) m_currentImage.height() / (double) p.rect().height() > 1.10
        && (double) m_currentImage.width() / (double) p.rect().width() > 1.10 )
        {
            if ( true )
                animateMove(p);
            else
                animateZoom(p);
        }
        else
        {
            // Image is too small, just draw it as-is
            p.drawImage( p.rect(), m_currentImage, m_currentImage.rect() );
        }
    }

    if ( pSettings->playerBackgroundTransitionDelay > 0
    && m_lastUpdated.elapsed() > (int) pSettings->playerBackgroundTransitionDelay * 1000 )
        loadNewImage();

    return -1;
}

void BackgroundImage::loadNewImage()
{
    if ( m_customBackground )
    {
        // Just reset the animation
        m_movementOrigin = QPoint( 0, 0 );
        m_zoomFactor = 0;
        return;
    }

    // Do we have more than one image?
    pCurrentState->nextBackgroundObject();

    //Logger::debug( "BgImage: show image %s", qPrintable( pCurrentState->currentBackgroundObject() ) );

    // If current image is not set, fill up it. Otherwise fill up new image
    if ( !m_newImage.load( pCurrentState->currentBackgroundObject() ) )
    {
        m_newImage = QImage();
        pActionHandler->warning( QString("BgImage: error loading image %1") .arg( pCurrentState->currentBackgroundObject() ) );
    }

    m_lastUpdated = QTime::currentTime();
    m_lastUpdated.start();

    // Start new transition
    m_percentage = 0;

    // Animation move
    // Choose new base angle - make it sharp (in 20-50 degree range), as we don't want to have like 1% angle
    //
    int angle = qrand() % 50 + 10;

    // And convert the angle to speed
    int mv_x = qMax( qRound( (sin( ((double) angle * M_PI) / 180 ) * (double) m_animationSpeed )), 1 );
    int mv_y = qMax( qRound( (cos( ((double) angle * M_PI) / 180 ) * (double) m_animationSpeed )), 1 );

    // And adjust speed and origin based on random intial direction
    switch ( qrand() % 4 )
    {
        case 0: // From left-top: x+, y+
            m_movementVelocity = QPoint( mv_x, mv_y );
            break;

        case 1: // From right-top: x-, y+
            m_movementVelocity = QPoint( -mv_x, mv_y );
            break;

        case 2: // From right-bottom: x-, y-
            m_movementVelocity = QPoint( -mv_x, -mv_y );
            break;

        case 3: // From left-bottom: x-, y-
            m_movementVelocity = QPoint( mv_x, -mv_y );
            break;
    }

    m_movementOrigin = QPoint( 0, 0 );
    m_zoomFactor = 0;
}

bool BackgroundImage::performTransition(KaraokePainter &p)
{
    // We want the whole transition to be done in 1.5 sec
    static const int TRANSITION = 500;
    int percstep = ( pCurrentState->msecPerFrame * 100) / TRANSITION;

    // Paint the original image first so we have it
    p.drawImage( QPoint(0,0), m_currentImage.scaled( p.size() ) );

    // Now we have four rectangles; see how big they should be in current image
    int curwidth = (p.size().width() * m_percentage) / 200;
    int curheight = (p.size().height() * m_percentage) / 200;

    // and in the new image
    int newwidth = ( m_newImage.width() * m_percentage) / 200;
    int newheight = ( m_newImage.height() * m_percentage) / 200;

    // Left top corner
    p.drawImage( QRect( 0, 0, curwidth, curheight ), m_newImage, QRect( 0, 0, newwidth, newheight ) );

    // Left bottom corner
    p.drawImage( QRect( 0, p.size().height() - curheight, curwidth, p.size().height() ),
                 m_newImage,
                 QRect( 0, m_newImage.height() - newheight, newwidth, m_newImage.height() ) );

    // Right top corner
    p.drawImage( QRect( p.size().width() - curwidth, 0, p.size().width(), curheight ),
                 m_newImage,
                 QRect( m_newImage.width() - newwidth, 0, m_newImage.width(), newheight ) );

    // Right bottom corner
    p.drawImage( QRect( p.size().width() - curwidth, p.size().height() - curheight, p.size().width(), p.size().height() ),
                 m_newImage,
                 QRect( m_newImage.width() - newwidth, m_newImage.height() - newheight, m_newImage.width(), m_newImage.height() ) );

    m_percentage += percstep;

    if ( m_percentage >= 100 )
        return false;

    return true;
}

void BackgroundImage::animateMove( KaraokePainter &p )
{
    QRect srcrect = QRect( 0, 0, (m_currentImage.width() - p.rect().width()) * 0.85,
                           (m_currentImage.height() - p.rect().height()) * 0.85 );

    QPoint neworigin = m_movementOrigin + m_movementVelocity;
    srcrect.moveTopLeft( neworigin );

    // Depends on direction
    if ( srcrect.x() < 0 || srcrect.right() > m_currentImage.width() )
        m_movementVelocity.setX( -m_movementVelocity.x() );

    if ( srcrect.y() < 0 || srcrect.bottom() > m_currentImage.height() )
        m_movementVelocity.setY( -m_movementVelocity.y() );

    if ( m_playbackPaused == 0 )
        m_movementOrigin += m_movementVelocity;

    srcrect.moveTopLeft( m_movementOrigin + m_movementVelocity );
    p.drawImage( p.rect(), m_currentImage, srcrect );
}

void BackgroundImage::animateZoom(KaraokePainter &p)
{
    QRect destrect = m_currentImage.rect();

    destrect.adjust( m_zoomFactor,
                     (m_zoomFactor * m_currentImage.height()) / m_currentImage.width(),
                     -m_zoomFactor,
                     (-m_zoomFactor * m_currentImage.height()) / m_currentImage.width() );

    if ( destrect.width() <= p.rect().width() )
        loadNewImage();

    // Decrease the animation speed the more the image is zoomed out
    if ( m_playbackPaused == 0 )
        m_zoomFactor += qMax( 1, qRound(m_animationSpeed * ((double)destrect.width() / m_currentImage.width())) );

    p.drawImage( p.rect(), m_currentImage, destrect );
}
