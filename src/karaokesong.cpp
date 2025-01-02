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

#include <QDir>
#include <QBuffer>
#include <QCryptographicHash>

#include "logger.h"
#include "settings.h"
#include "karaokesong.h"
#include "karaokewidget.h"
#include "playerlyricscdg.h"
#include "playerlyricstext.h"
#include "backgroundimage.h"
#include "karaokepainter.h"
#include "actionhandler.h"
#include "database.h"
#include "notification.h"
#include "currentstate.h"
#include "util.h"
#include "karaokeplayable.h"
#include "mediaplayer.h"
#include "midisyntheser.h"
#include "midistripper.h"
#include "eventor.h"
#include "mediaplayer.h"

#include "libkaraokelyrics/lyricsloader.h"

KaraokeSong::KaraokeSong( KaraokeWidget *w, const SongQueueItem &song )
{
    m_widget = w;
    m_song = song;
    mPlayer = new MediaPlayer();

    pCurrentState->playerCapabilities = (MediaPlayer::Capability) 0;
    m_lyrics = 0;
    m_background = 0;
    m_rating = 0;
    m_tempMusicFile = 0;

    m_nextRedrawTime = -1;
    m_lastRedrawTime = -1;

    // Connect slots in widget - this way we're saving on declaring extra signals we'd just broadcast from a player
    connect( mPlayer->qObject(), SIGNAL( finished() ), w, SLOT(karaokeSongFinished()) );
    connect( mPlayer->qObject(), SIGNAL( loaded() ), this, SLOT(songLoaded()) );
    connect( mPlayer->qObject(), SIGNAL( error(QString) ), w, SLOT(karaokeSongError(QString)) );
    connect( mPlayer->qObject(), SIGNAL( durationChanged()), this, SLOT(durationChanged()) );

    // Use Karaoke actions here
    connect( pActionHandler, &ActionHandler::actionPlayerVolumeDown, this, &KaraokeSong::volumeDown );
    connect( pActionHandler, &ActionHandler::actionPlayerVolumeUp, this, &KaraokeSong::volumeUp );
    connect( pActionHandler, &ActionHandler::actionPlayerVolumeSet, this, &KaraokeSong::volumeSet );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerPauseResume, this, &KaraokeSong::pause );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerForward, this, &KaraokeSong::seekForward );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerBackward, this, &KaraokeSong::seekBackward );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerLyricsEarlier, this, &KaraokeSong::lyricEarlier );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerLyricsLater, this, &KaraokeSong::lyricLater );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerRatingDecrease, this, &KaraokeSong::ratingDown );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerRatingIncrease, this, &KaraokeSong::ratingUp );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerPitchDecrease, this, &KaraokeSong::pitchLower );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerPitchIncrease, this, &KaraokeSong::pitchHigher );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerTempoDecrease, this, &KaraokeSong::tempoSlower );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerTempoIncrease, this, &KaraokeSong::tempoFaster );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerChangePitch, this, &KaraokeSong::pitchSet );
    connect( pActionHandler, &ActionHandler::actionKaraokePlayerChangeTempo, this, &KaraokeSong::tempoSet );
}

KaraokeSong::~KaraokeSong()
{
    mPlayer->stop();

    delete m_lyrics;
    delete m_background;
    delete m_tempMusicFile;
    delete mPlayer;
}

bool KaraokeSong::open()
{
    // Reset just in case
    m_lyrics = 0;
    m_background = 0;

    // If we have song ID, query the DB first
    Database_SongInfo info;

    if ( m_song.songid != 0 && pDatabase->songById( m_song.songid, info ) )
        m_rating = info.rating;
    else
        m_rating = 0;

    // If this is a video file, there is no lyrics
    if ( KaraokePlayable::isVideoFile( m_song.file ) )
    {
        // Load the video file and enable both video and audio streams.
        // It loads asynchronously, and emits loaded() signal afterward
        m_musicFileName = m_song.file;
        mPlayer->loadMedia( m_musicFileName, MediaPlayer::LoadAudioStream | MediaPlayer::LoadVideoStream );
        Logger::debug( "KaraokeSong: video file is being loaded" );
    }
    else
    {
        QScopedPointer<KaraokePlayable> karaoke( KaraokePlayable::create( m_song.file ) );

        if ( !karaoke || !karaoke->parse() )
            throw QString( "Cannot find a matching music/lyric file");

        // Get the files
        m_musicFileName = karaoke->musicObject();
        QString lyricFile = karaoke->lyricObject();

        Logger::debug( "KaraokeSong: found music file %s and lyric file %s", qPrintable(m_musicFileName), qPrintable(lyricFile) );

        // If this is MIDI file, try to find a cached file which must be there
        if ( KaraokePlayable::isMidiFile( m_musicFileName ) )
        {
            // Strip the MIDI before passing it to a syntheser
            QByteArray midiData = MIDIStripper::stripMIDI( m_musicFileName );

            if ( midiData.isEmpty() )
                throw QString( "Invalid MIDI file");

            if ( pSettings->playerUseBuiltinMidiSynth )
            {
                // Use built-in syntheser
                MIDISyntheser * midi = new MIDISyntheser();

                if ( !midi->open( midiData ) )
                    throw QString("Cannot open MIDI file %1").arg( m_musicFileName );

                mPlayer->loadMedia( midi, MediaPlayer::LoadAudioStream );
                Logger::debug( "KaraokeSong: MIDI file is being loaded via built-in MIDI sequencer" );
            }
            else
            {
                // Make the QIODevice from data
                QBuffer * midibuf = new QBuffer();
                midibuf->setData( midiData );
                midibuf->open( QIODevice::ReadOnly );

                mPlayer->loadMedia( midibuf, MediaPlayer::LoadAudioStream );
                Logger::debug( "KaraokeSong: MIDI file is being loaded via GStreamer" );
            }
        }
        else
        {
            // If music file is not local, we need to extract it into the temp file
            if ( karaoke->isCompound() )
            {
                // This temporary file will store the extracted music file
                m_tempMusicFile = new QTemporaryFile( QDir::tempPath() + QDir::separator() + "karaokemedia-XXXXXXX." + Util::fileExtension( m_musicFileName ) );

                if ( !m_tempMusicFile->open() )
                    throw QString( "Cannot open temporary file %1: %2") .arg( m_tempMusicFile->fileName() .arg( m_tempMusicFile->errorString() ) );

                // Extract the music into a temporary file
                if ( !karaoke->extract( m_musicFileName, m_tempMusicFile ) )
                    throw QString( "Cannot extract a music file from an archive");

                // this is our music file now! Close it and remember it.
                m_tempMusicFile->close();
                m_musicFileName = m_tempMusicFile->fileName();
            }
            else
                m_musicFileName = karaoke->absolutePath( karaoke->musicObject() );

            // Load the music file, enable only audio stream.
            // It loads asynchronously, and emits loaded() signal afterward
            mPlayer->loadMedia( m_musicFileName, MediaPlayer::LoadAudioStream );
            Logger::debug( "KaraokeSong: music file is being loaded" );
        }

        // Get the pointer to the device the lyrics are stored in (will be deleted after the pointer out-of-scoped)
        QScopedPointer<QIODevice> lyricDevice( karaoke->openObject( lyricFile ) );

        if ( lyricDevice == 0 )
            throw QString( "Cannot read a lyric file %1" ) .arg( lyricFile );

        // Now we can load the lyrics
        if ( lyricFile.endsWith( ".cdg", Qt::CaseInsensitive ) )
            m_lyrics = new PlayerLyricsCDG();
        else
            m_lyrics = new PlayerLyricsText( info.artist, info.title );

        if ( !m_lyrics->load( lyricDevice.data(), lyricFile ) )
        {
            QString errormsg = m_lyrics->errorMsg();
            delete m_lyrics;
            m_lyrics = 0;            
            throw( QObject::tr("Can't load lyrics file %1: %2") .arg( lyricFile ) .arg( errormsg ) );
        }

        // Destroy the object right away
        lyricDevice.reset( 0 );

        // Set the lyric delay
        m_lyrics->setDelay( info.lyricDelay );

        Logger::debug( "KaraokeSong: lyrics loaded" );

        // Now check if we got custom background (Ultrastar or KFN)
        if ( !pSettings->playerIgnoreBackgroundFromFormats )
        {
            QString filename;

            if (  m_lyrics->properties().contains( LyricsLoader::PROP_BACKGROUND ) )
                filename = m_lyrics->properties() [ LyricsLoader::PROP_BACKGROUND ];
            else if ( !karaoke->backgroundImageObject().isEmpty() )
                filename = karaoke->backgroundImageObject();

            // Same logic as with lyrics above
            if ( !filename.isEmpty() )
            {
                QScopedPointer<QIODevice> iod( karaoke->openObject( filename ) );

                if ( iod != 0 )
                {
                    // Load the background from this buffer
                    m_background = new BackgroundImage();

                    if ( !m_background->initFromFile( iod.data(), filename ) )
                    {
                        Logger::debug( "Could not load background from %s: invalid file", qPrintable(filename) );
                        delete m_background;
                        m_background = 0;
                    }
                }
                else
                    Logger::debug( "Could not load background from file %s: no such file", qPrintable(filename) );
            }
        }
    }

    return true;
}

void KaraokeSong::start()
{
    // Indicate that we started
    pEventor->karaokeStarted( m_song );

    pCurrentState->playerDuration = mPlayer->duration();
    pCurrentState->playerState = CurrentState::PLAYERSTATE_PLAYING;

    if ( m_background )
        m_background->start();

    // Set the last remembered volume. We do not want notification, so no VolumeSet
    mPlayer->setCapabilityValue( MediaPlayer::CapChangeVolume, pCurrentState->playerVolume );

    // Thus we notify the eventor manually
    pEventor->karaokeVolumeChanged( pCurrentState->playerVolume );

    mPlayer->play();

    pSongQueue->statusChanged( m_song.id, true );
    pNotification->clearOnScreenMessage();
}

void KaraokeSong::pause()
{
    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_PLAYING )
    {
        pCurrentState->playerState = CurrentState::PLAYERSTATE_PAUSED;
        mPlayer->pause();

        if ( m_background )
            m_background->pause( true );

        // Notify the event generator
        pEventor->karaokePausedResumed( true );
    }
    else
    {
        pCurrentState->playerState = CurrentState::PLAYERSTATE_PLAYING;
        mPlayer->play();

        if ( m_background )
            m_background->pause( false );

        // Notify the event generator
        pEventor->karaokePausedResumed( false );
    }
}

void KaraokeSong::seekForward()
{
    seekTo( qMin( mPlayer->duration() - 10000, mPlayer->position() + 10000 ) );
}

void KaraokeSong::seekBackward()
{
    qint64 pos = mPlayer->position() - 10000;

    if ( pos < 0 )
        pos = 0;

    seekTo( pos );
}

void KaraokeSong::seekTo(qint64 timing)
{
    mPlayer->seekTo( timing );
}

qint64 KaraokeSong::draw(KaraokePainter &p)
{
    // Update position
    pCurrentState->playerPosition = mPlayer->position();

    qint64 time = pCurrentState->playerDuration;

    if ( m_background )
        time = qMin( m_background->draw( p ), time );

    // And tint it according to percentage
    p.fillRect( p.rect(), QColor ( 0, 0, 0, int( (double) pSettings->playerLyricBackgroundTintPercentage * 2.55) ) );

    if ( m_lyrics )
    {
        m_lyrics->draw( p );
    }
    else
    {
        mPlayer->drawVideoFrame( p, p.rect() );
        time = 0;
    }

    return time;
}

bool KaraokeSong::hasCustomBackground() const
{
    return m_background != 0;
}

void KaraokeSong::stop()
{
    pCurrentState->playerState = CurrentState::PLAYERSTATE_STOPPED;

    if ( m_background )
    {
        m_background->stop();
        delete m_background;
        m_background = 0;
    }

    mPlayer->stop();

    pSongQueue->statusChanged( m_song.id, false );

    if ( m_song.songid )
        pDatabase->updatePlayedSong( m_song.songid, m_lyrics ? m_lyrics->delay() : 0, m_rating );
}

void KaraokeSong::lyricEarlier()
{
    if ( !m_lyrics )
        return;

    int newdelay = m_lyrics->delay() + 200;
    m_lyrics->setDelay( newdelay );

    pNotification->showMessage( tr("Lyrics show %1 by %2ms") .arg( newdelay < 0 ? "earlier" : "later") .arg(newdelay));
}

void KaraokeSong::lyricLater()
{
    if ( !m_lyrics )
        return;

    int newdelay = m_lyrics->delay() - 200;
    m_lyrics->setDelay( newdelay );

    pNotification->showMessage( tr("Lyrics show %1 by %2ms") .arg( newdelay < 0 ? "earlier" : "later") .arg(newdelay));
}

void KaraokeSong::ratingDown()
{
    m_rating--;
    pNotification->showMessage( tr("Song rating decreased to %1") .arg(m_rating) );
}

void KaraokeSong::ratingUp()
{
    m_rating++;
    pNotification->showMessage( tr("Song rating increased to %1") .arg(m_rating) );
}

void KaraokeSong::volumeDown()
{
    volumeSet( qMax( pCurrentState->playerVolume - pSettings->playerVolumeStep, 0 ) );
}

void KaraokeSong::volumeUp()
{
    volumeSet( qMin( pCurrentState->playerVolume + pSettings->playerVolumeStep, 100 ) );
}

void KaraokeSong::volumeSet(int newvalue)
{
    setVolume( newvalue );
}

void KaraokeSong::durationChanged()
{
    pCurrentState->playerDuration = mPlayer->duration();
}

void KaraokeSong::pitchLower()
{
    setPitch( pCurrentState->playerPitch - 5 );
}

void KaraokeSong::pitchHigher()
{\
    setPitch( pCurrentState->playerPitch + 5 );
}

void KaraokeSong::pitchSet(int newvalue)
{
    setPitch( newvalue, false );
}

void KaraokeSong::tempoSlower()
{
    setTempo( pCurrentState->playerTempo -5  );
}

void KaraokeSong::tempoFaster()
{
    setTempo( pCurrentState->playerTempo + 5 );
}

void KaraokeSong::tempoSet(int newvalue)
{
    setTempo( newvalue, false );
}

void KaraokeSong::songLoaded()
{
    // We might end up in a situation where the music is loaded, but the lyrics aren't.
    // In this case we'll get this signal but m_lyrics is NULL.
    // But m_lyrics is also NULL when we're playing a video file so check for this too.
    if ( m_lyrics == 0 && !KaraokePlayable::isVideoFile( m_song.file ) )
        return;

    pCurrentState->playerPitch = 50;
    pCurrentState->playerTempo = 50;

    pCurrentState->playerCapabilities = mPlayer->capabilities();
    m_widget->karaokeSongLoaded();
}

void KaraokeSong::setVolume(int newvalue, bool notify)
{
    // Make sure the value is in 0-100 range
    int newvolume = qMax( 0, qMin( newvalue, 100 ) );

    mPlayer->setCapabilityValue( MediaPlayer::CapChangeVolume, newvolume );

    pCurrentState->playerVolume = newvolume;

    if ( notify )
    {
        pNotification->showMessage( newvolume, "Volume" );
        pEventor->karaokeVolumeChanged( newvolume );
    }
}

void KaraokeSong::setPitch(int newvalue, bool notify)
{
    if ( pCurrentState->playerCapabilities & MediaPlayer::CapChangePitch )
    {
        pCurrentState->playerPitch = newvalue;
        mPlayer->setCapabilityValue( MediaPlayer::CapChangePitch, pCurrentState->playerPitch );

        if ( notify )
        {
            if ( pCurrentState->playerPitch > newvalue )
                pNotification->showMessage( tr("Pitch decreased to %1") .arg(newvalue) );
            else
                pNotification->showMessage( tr("Pitch increased to %1") .arg(newvalue) );

            emit pEventor->karaokeParametersChanged();
        }
    }
}

void KaraokeSong::setTempo(int newvalue, bool notify)
{
    if ( pCurrentState->playerCapabilities & MediaPlayer::CapChangeTempo )
    {
        pCurrentState->playerTempo = newvalue;
        mPlayer->setCapabilityValue( MediaPlayer::CapChangeTempo, pCurrentState->playerTempo );

        if ( notify )
        {
            if ( pCurrentState->playerTempo > newvalue )
                pNotification->showMessage( tr("Tempo decreased to %1") .arg(newvalue) );
            else
                pNotification->showMessage( tr("Tempo increased to %1") .arg(newvalue) );

            emit pEventor->karaokeParametersChanged();
        }
    }
}
