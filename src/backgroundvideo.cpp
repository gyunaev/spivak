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

#include "logger.h"
#include "settings.h"
#include "karaokepainter.h"
#include "actionhandler.h"
#include "currentstate.h"
#include "backgroundvideo.h"
#include "pluginmanager.h"

BackgroundVideo::BackgroundVideo()
    : QObject(), Background()
{
    m_player = 0;
}

BackgroundVideo::~BackgroundVideo()
{
    delete m_player;
}

void BackgroundVideo::start()
{
}

void BackgroundVideo::pause(bool pausing)
{
    if ( !m_player )
    {
        Logger::error( "BackgroundVideo::pause called with no player" );
        return;
    }

    if ( pausing && m_player->state() == MediaPlayer::StatePlaying )
        m_player->pause();
    else if ( !pausing && m_player->state() == MediaPlayer::StatePaused )
        m_player->play();
}

void BackgroundVideo::stop()
{
    if ( !m_player )
    {
        Logger::error( "BackgroundVideo::stop called with no player" );
        return;
    }

    // Store the current video position so next song could resume video
    if ( m_player->position() > 0 )
        pCurrentState->playerBackgroundLastVideoPosition = m_player->position();

    m_player->stop();
}


void BackgroundVideo::finished()
{
    Logger::debug("Background video: current video ended, switching to new video" );
    pCurrentState->nextBackgroundObject();

    initFromSettings();
}

void BackgroundVideo::errorplaying()
{
    // FIXME: should we remove the current object from list
    Logger::debug("Background video: current video failed to play at positon %d and generated an error, switching to new video", m_player->position() );
    pCurrentState->nextBackgroundObject();

    initFromSettings();
}

bool BackgroundVideo::initFromSettings()
{
    QString videofile = pCurrentState->currentBackgroundObject();

    if ( m_player )
    {
        m_player->stop();
        delete m_player;
    }

    m_player = pPluginManager->createMediaPlayer();

    if ( !m_player )
        return false;

    connect( m_player->qObject(), SIGNAL(finished()), this, SLOT(finished()) );
    connect( m_player->qObject(), SIGNAL(error(QString)), this, SLOT(errorplaying()) );

    // Autostart video
    connect( m_player->qObject(), SIGNAL(loaded()), m_player->qObject(), SLOT(play()) );

    if ( videofile.isEmpty() )
    {
        Logger::debug("Background video: video file is empty" );
        delete m_player;
        m_player = 0;
        return false;
    }

    m_player->loadMedia( videofile, MediaPlayer::LoadVideoStream );
    Logger::debug("Background video: video file %s is being loaded", qPrintable(videofile) );

    if ( pCurrentState->playerBackgroundLastVideoPosition )
        m_player->seekTo( pCurrentState->playerBackgroundLastVideoPosition );

    return true;
}

bool BackgroundVideo::initFromFile(QIODevice *, const QString &)
{
    // FIXME: This is not unused (i.e. UltraStart and KFN use it), but not currently implemented
    return false;
}

qint64 BackgroundVideo::draw(KaraokePainter &p)
{
    if ( m_player )
        m_player->drawVideoFrame( p, p.rect() );

    // Next frame
    return 0;
}
