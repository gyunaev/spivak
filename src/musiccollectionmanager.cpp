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

#include <QSettings>
#include <QTimer>

#include "settings.h"
#include "musiccollectionmanager.h"
#include "notification.h"
#include "currentstate.h"
#include "actionhandler.h"
#include "eventor.h"
#include "logger.h"
#include "util.h"

MusicCollectionManager * pMusicCollectionMgr;


MusicCollectionManager::MusicCollectionManager( QObject *parent )
    : QObject(parent)
{
    m_player = 0;
    m_currentFile = 0;

    m_supportedExtensions << "mp3" << "ogg" << "flac" << "wav" << "wma";
    connect( pEventor, &Eventor::settingsChangedMusic, this, &MusicCollectionManager::initCollection );
}

MusicCollectionManager::~MusicCollectionManager()
{
    delete m_player;
}

void MusicCollectionManager::exportCollection(QStringList &musicFiles)
{
    musicFiles = m_musicFiles;
}

void MusicCollectionManager::insertMusic(int pos, const QString &file)
{
    if ( pos == -1 )
        pos = m_musicFiles.size() - 1;

    m_musicFiles.insert( pos, file );
    emit pEventor->musicQueueChanged();
}

void MusicCollectionManager::removeMusic(unsigned int position)
{
    m_musicFiles.removeAt( position );

    // If we removed the currently playing song, switch to next right away
    if ( m_currentFile == position )
        playerCurrentSongFinished();

    emit pEventor->musicQueueChanged();
}

void MusicCollectionManager::moveMusicItem(unsigned int from, unsigned int to)
{
    m_musicFiles.move( from, to );
    emit pEventor->musicQueueChanged();
}

const QStringList &MusicCollectionManager::supportedExtensions() const
{
    return m_supportedExtensions;
}

void MusicCollectionManager::initCollection()
{
    // This function is also called when settings are updated - and if nothing changes, we do nothing
    if ( m_musicDirectories == pSettings->musicCollections )
        return;

    m_musicFiles.clear();

    // If we had the collection but now disabled it, remove the player
    if ( pSettings->musicCollections.isEmpty() )
    {
        delete m_player;
        m_player = 0;
        return;
    }

    m_musicDirectories = pSettings->musicCollections;

    // Find the music files
    Util::enumerateDirectory( m_musicDirectories[0], supportedExtensions(), m_musicFiles );
    Logger::debug( "Music collection: %d music files found", m_musicFiles.size() );

    // If there are no music files, disable the player
    if ( m_musicFiles.isEmpty() )
    {
        m_musicDirectories.clear();
        delete m_player;
        m_player = 0;
        return;
    }

    // Load temporary parameters and validate them
    QSettings settings;

    // Do we still have this object?
    QString lastobject = settings.value( "temp/musicCollectionLastObject", "" ).toString();
    int lastindex;

    if ( !lastobject.isEmpty() && (lastindex = m_musicFiles.indexOf( lastobject )) != -1 )
        m_currentFile = lastindex;
    else
        m_currentFile = 0;

    // Player controls (via actions)
    connect( pActionHandler, &ActionHandler::actionPlayerVolumeDown, this, &MusicCollectionManager::volumeDown );
    connect( pActionHandler, &ActionHandler::actionPlayerVolumeUp, this, &MusicCollectionManager::volumeUp );
    connect( pActionHandler, &ActionHandler::actionPlayerVolumeSet, this, &MusicCollectionManager::volumeSet );
    connect( pActionHandler, &ActionHandler::actionMusicPlayerPauseResume, this, &MusicCollectionManager::pause );
    connect( pActionHandler, &ActionHandler::actionMusicPlayerStop, this, &MusicCollectionManager::stop );
    connect( pActionHandler, &ActionHandler::actionMusicPlayerPlay, this, &MusicCollectionManager::start );

    // Queue control (via actions)
    connect( pActionHandler, &ActionHandler::actionMusicQueueNext, this, &MusicCollectionManager::nextMusic );
    connect( pActionHandler, &ActionHandler::actionMusicQueuePrev, this, &MusicCollectionManager::prevMusic );

    // Notifications
    connect( pEventor, &Eventor::karaokeStarted, this, &MusicCollectionManager::karaokeStarted );
    connect( pEventor, &Eventor::karaokeStopped, this, &MusicCollectionManager::karaokeStopped );
}

void MusicCollectionManager::start()
{
    // This will autostart
    if ( m_player )
        return; // already playing

    m_player = new MediaPlayer();

    connect( m_player, SIGNAL(finished()), this, SLOT( playerCurrentSongFinished()) );
    connect( m_player, SIGNAL(error(QString)), this, SLOT(playerCurrentSongFinished()) );
    connect( m_player, SIGNAL(loaded()), this, SLOT(songLoaded()) );
    connect( m_player, SIGNAL(tagsChanged(QString,QString)), pEventor, SIGNAL( musicTagsChanged(QString,QString)) );

    // Initiate loading the audio
    m_player->loadMedia( m_musicFiles[m_currentFile], MediaPlayer::LoadAudioStream );
    Logger::debug( "Music collection: loading music file %s", qPrintable(m_musicFiles[m_currentFile]) );
}

void MusicCollectionManager::stop()
{
    if ( !m_player )
        return;

    //TODO: move to currentstate
    // Save temporary parameters and validate them
    QSettings settings;

    if ( (int) m_currentFile < m_musicFiles.size() )
        settings.setValue( "temp/musicCollectionLastObject", m_musicFiles[m_currentFile] );

    m_player->stop();
    delete m_player;
    m_player = 0;

    emit pEventor->musicStopped();
}

void MusicCollectionManager::pause()
{
    if ( !m_player )
        return;

    if ( pSettings->musicCollectionCrossfadeTime != 0 )
    {
        // We are fading from m_current_volume to zero in pSettings->musicCollectionCrossfadeTime
        // and we will call us every 200ms
        int called_times = qMax( 1, (pSettings->musicCollectionCrossfadeTime * 1000) / 200 );
        m_crossfadeVolumeStep = pCurrentState->musicVolume / called_times;

        // On resume m_player->volume() is 0
        if ( m_player->state() == MediaPlayer::StatePlaying )
        {
            m_crossfadeVolumeStep = -m_crossfadeVolumeStep;
            m_crossfadeCurrentVolume = pCurrentState->musicVolume;
        }
        else
            m_crossfadeCurrentVolume = 0;

        fadeVolume();

        // We do not pause here in case of fadeout (we pause later) but we resume here
        if ( m_player->state() == MediaPlayer::StatePaused )
        {
            m_player->play();
            emit pEventor->musicPausedResumed( false);
        }
        else
            emit pEventor->musicPausedResumed( true );
    }
    else
    {
        // No crossfade
        if ( m_player->state() == MediaPlayer::StatePaused )
        {
            m_player->play();
            emit pEventor->musicPausedResumed( false);
        }
        else
        {
            m_player->pause();
            emit pEventor->musicPausedResumed( true );
        }
    }
}

void MusicCollectionManager::nextMusic()
{
    stop();

    // move to next song in queue; wrap it around
    m_currentFile++;

    if ( (int) m_currentFile >= m_musicFiles.size() )
        m_currentFile = 0; // restart from the beginning

    emit pEventor->musicQueueChanged();
    start();
}

void MusicCollectionManager::prevMusic()
{
    stop();

    // move to prev song in queue; wrap it around
    if ( m_currentFile == 0 )
        m_currentFile = m_musicFiles.size() - 1;
    else
        m_currentFile--;

    emit pEventor->musicQueueChanged();
    start();
}

void MusicCollectionManager::volumeDown()
{
    setVolume( qMax( pCurrentState->musicVolume - pSettings->playerVolumeStep, 0 ) );
}

void MusicCollectionManager::volumeUp()
{
    setVolume( qMin( pCurrentState->musicVolume + pSettings->playerVolumeStep, 100 ) );
}

void MusicCollectionManager::volumeSet(int percentage)
{
    setVolume( percentage );
}

void MusicCollectionManager::karaokeStarted(SongQueueItem )
{
    if ( !m_player )
        return;

    Logger::debug( "Music collection: Karaoke finished, pausing music" );

    if ( m_player->state() == MediaPlayer::StatePlaying )
    {
        pause();
        pEventor->musicPausedResumed( true );
    }
}

void MusicCollectionManager::karaokeStopped()
{
    if ( !m_player )
        return;

    Logger::debug( "Music collection: Karaoke finished, resuming music" );

    if ( m_player->state() == MediaPlayer::StatePaused )
    {
        pause(); // this would resume
        pEventor->musicPausedResumed( false );
    }
}

void MusicCollectionManager::playerCurrentSongFinished()
{
    stop();
    emit pEventor->musicFinished();
}

bool MusicCollectionManager::isPlaying() const
{
    return m_player && m_player->state() == MediaPlayer::StatePlaying;
}

void MusicCollectionManager::fadeVolume()
{
    if ( !m_player )
        return;

    // Apply the crossfade step
    m_crossfadeCurrentVolume = qMax( 0, qMin( m_crossfadeCurrentVolume + m_crossfadeVolumeStep, (int) pCurrentState->musicVolume ) );

    // Here we set volume without notifying anyone
    m_player->setCapabilityValue( MediaPlayer::CapChangeVolume, m_crossfadeCurrentVolume );

    // If we reached out to zero, pause the player
    if ( m_crossfadeCurrentVolume == 0 )
    {
        m_crossfadeVolumeStep = 0;
        m_player->pause();
    }
    else
    {
        // If we got the full volume, no more crossfading
        if ( m_crossfadeCurrentVolume >= (int) pCurrentState->musicVolume )
        {
            m_crossfadeVolumeStep = 0;
            return;
        }

        // Rinse and repeat
        QTimer::singleShot( 200, this, SLOT(fadeVolume()) );
    }
}

void MusicCollectionManager::songLoaded()
{
    // We do not need notifications, so we do not call volumeSet() - thus notify eventor manually
    setVolume( pCurrentState->musicVolume, false );

    // Retrieve the artist/title
    QString artist, title;
    m_player->mediaTags( artist, title );

    Logger::debug( "Music collection: loaded music file %s", qPrintable(m_musicFiles[m_currentFile]) );

    m_player->play();
    emit pEventor->musicStarted();
}

void MusicCollectionManager::setVolume(int percentage, bool notify)
{
    if ( !m_player )
        return;

    m_player->setCapabilityValue( MediaPlayer::CapChangeVolume, percentage );

    // Notify the eventor and remember it
    pEventor->musicVolumeChanged( percentage );
    pCurrentState->musicVolume = percentage;

    if ( notify )
        pNotification->showMessage( percentage, "Volume" );
}

void MusicCollectionManager::currentProgress(qint64 &position, qint64 &duration)
{
    if ( !m_player )
        return;

    position = m_player->position();
    duration = m_player->duration();
}
