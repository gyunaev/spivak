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

#include <QPainter>
#include <QThread>
#include <QTimer>

#include "eventor.h"
#include "logger.h"
#include "karaokesong.h"
#include "karaokewidget.h"
#include "songqueue.h"
#include "actionhandler.h"
#include "currentstate.h"
#include "playerrenderer.h"
#include "notification.h"
#include "musiccollectionmanager.h"
#include "messageboxautoclose.h"

KaraokeWidget::KaraokeWidget(QWidget *parent )
    : QWidget(parent)
{
    m_karaoke = 0;
    m_background = 0;

    connect( pActionHandler, &ActionHandler::actionKaraokePlayerStart, this, &KaraokeWidget::playCurrent );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerStop, this, &KaraokeWidget::stop );
    connect( pEventor, &Eventor::settingsChangedBackground, this, &KaraokeWidget::initBackground );

    // 0 is set as draw image
    m_images[0] = new QImage(100, 100, QImage::Format_ARGB32);
    m_images[0]->fill( Qt::black );
    m_drawImageIndex = 0;

    // 1 as render image
    m_images[1] = new QImage(100, 100, QImage::Format_ARGB32);
    m_renderer = new PlayerRenderer( pNotification, m_images[1], this );
    m_renderer->start();
}

KaraokeWidget::~KaraokeWidget()
{
    delete m_background;
    delete m_images[0];
    delete m_images[1];
}

bool KaraokeWidget::initBackground()
{
    pCurrentState->applySettingsBackground();

    QMutexLocker m( &m_karaokeMutex );

    // If not set already, which background should we use?
    if ( m_background )
        delete m_background;

    m_background = Background::create();

    if ( !m_background->initFromSettings() )
        return false;

    m_background->start();
    return true;
}

void KaraokeWidget::playCurrent()
{
    if ( pSongQueue->isEmpty() )
    {
        Logger::debug("KaraokeWidget::playCurrent nothing to play");
        pNotification->clearOnScreenMessage();
        return;
    }

    SongQueueItem current = pSongQueue->current();

    // This shouldn't happen
    if ( current.state == SongQueueItem::STATE_NOT_READY )
        abort();

    // If current song is not ready yet, show the notification and wait until it is
    if ( current.state == SongQueueItem::STATE_GETTING_READY )
    {
        Logger::debug("KaraokeWidget::playCurrent want %s play but it is not ready yet", qPrintable( current.file) );

        // While we're converting MIDI, ensure other songs are stopped
        m_karaokeMutex.lock();

        if ( m_karaoke )
        {
            m_karaoke->stop();
            delete m_karaoke;
            m_karaoke = 0;
        }

        m_karaokeMutex.unlock();

        pNotification->setOnScreenMessage( current.stateText() );
        QTimer::singleShot( 500, this, SLOT( playCurrent() ) );
        return;
    }

    Logger::debug("KaraokeWidget::playCurrent %s", qPrintable( current.file) );
    pNotification->clearOnScreenMessage();
    KaraokeSong * karfile = new KaraokeSong( this, current );

    // Pause the background while loading song (prevents GStreamer errors)
    m_background->pause( true );

    try
    {
        if ( !karfile->open() )
        {
            // Resume background
            m_background->pause( false );

            delete karfile;
            return;
        }
    }
    catch ( const QString& ex )
    {
        // Resume background
        m_background->pause( false );
        Logger::error("KaraokeWidget::playCurrent %s: exception %s", qPrintable( current.file), qPrintable(ex) );

        MessageBoxAutoClose::critical(
                    "Cannot play file",
                    tr("Cannot play file %1:\n%2") .arg( current.file ) .arg( ex ) );

        delete karfile;

        // Kick the current song out of queue
        pActionHandler->dequeueSong( current.id );
        return;
    }

    m_karaokeMutex.lock();

    if ( m_karaoke )
    {
        qWarning("BUG: new karaoke is started while the old is not stoppped!");
        m_karaoke->stop();
        delete m_karaoke;
    }

    m_karaoke = karfile;
    m_karaokeMutex.unlock();

    Logger::debug("KaraokeWidget: waiting for the song being loaded");
}

void KaraokeWidget::stopKaraoke()
{
    Logger::debug("KaraokeWidget::stopKaraoke");
    pCurrentState->playerState = CurrentState::PLAYERSTATE_STOPPED;

    m_karaokeMutex.lock();
    KaraokeSong * k = m_karaoke;
    m_karaoke = 0;
    m_karaokeMutex.unlock();

    // Is anything playing?
    if ( k == 0 )
        return;

    // If we stopped our background, start it
    if ( k->hasCustomBackground() )
        m_background->pause( false );

    k->stop();
    delete k;

    pCurrentState->saveTempData();
    Logger::debug("KaraokeWidget::stopKaraoke finished");
}

void KaraokeWidget::stop()
{
    Logger::debug("KaraokeWidget::stop");

    m_karaokeMutex.lock();
    KaraokeSong * k = m_karaoke;
    m_karaokeMutex.unlock();

    if ( k )
    {
        stopKaraoke();
        emit pEventor->karaokeStopped();
    }
    else
        pMusicCollectionMgr->stop();

    Logger::debug("KaraokeWidget::stopped");
}


void KaraokeWidget::stopEverything()
{
    pMusicCollectionMgr->stop();
    stop();

    // Stop the rendering thread
    m_renderer->stop();
    delete m_renderer;
    m_renderer = 0;
}

void KaraokeWidget::karaokeSongFinished()
{
    // A song ended naturally
    stop();
    emit pEventor->karaokeFinished();
}

void KaraokeWidget::karaokeSongLoaded()
{
    Logger::debug("KaraokeWidget: song loaded successfully, starting");

    // Resume our background if we do not have custom background (we paused it before in playCurrent() )
    if ( !m_karaoke->hasCustomBackground() )
        m_background->pause( false );

    m_karaoke->start();
}

void KaraokeWidget::karaokeSongError( QString errormsg )
{
    Logger::debug( "KaraokeWidget: song failed to load, or aborted: %s", qPrintable(errormsg) );

    MessageBoxAutoClose::critical(
                "Cannot play file",
                tr("Cannot play music:\n%1") .arg( errormsg ) );

    // Resume our background if we do not have custom background (we paused it before in playCurrent() )
    if ( m_karaoke && !m_karaoke->hasCustomBackground() )
        m_background->pause( false );

    karaokeSongFinished();
}

// This function is called from a different thread, which is guaranteed not to use render image until the call returns
QImage * KaraokeWidget::switchImages()
{
    int newRenderImage;

    // The current draw image, however, can be touched in draw(),
    // so we should not touch it until we switch the pointer
    m_imageMutex.lock();

    if ( m_drawImageIndex == 0 )
    {
        newRenderImage = 0;
        m_drawImageIndex = 1;
    }
    else
    {
        newRenderImage = 1;
        m_drawImageIndex = 0;
    }

    m_imageMutex.unlock();

    // We have switched the draw image which now uses our image; see if we need to resize the current QImage before returning it
    if ( m_images[newRenderImage]->size() != size() )
    {
        delete m_images[newRenderImage];
        m_images[newRenderImage] = new QImage( size(), QImage::Format_ARGB32 );
    }

    // Queue a refresh event for ourselves - note Qt::QueuedConnection as we're calling from a different thread!
    QMetaObject::invokeMethod( this, "update", Qt::QueuedConnection );

    return m_images[newRenderImage];
}

void KaraokeWidget::paintEvent(QPaintEvent *)
{
    QPainter p( this );
    QRect prect = QRect( 0, 0, width(), height() );

    m_imageMutex.lock();
    p.drawImage( prect, *m_images[ m_drawImageIndex ] );
    m_imageMutex.unlock();
}

void KaraokeWidget::keyPressEvent(QKeyEvent *event)
{
    pActionHandler->keyEvent( event );
}
