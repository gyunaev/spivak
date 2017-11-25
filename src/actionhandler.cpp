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

#include <QApplication>
#include <QInputDialog>

#include "actionhandler.h"
#include "database.h"
#include "settings.h"
#include "currentstate.h"
#include "mainwindow.h"
#include "songdatabasescanner.h"
#include "musiccollectionmanager.h"
#include "notification.h"
#include "logger.h"
#include "eventor.h"

#if defined (HAS_LIRC_SUPPORT)
    #include "actionhandler_lirc.h"
#endif

#if defined (HAS_HTTP_SUPPORT)
    #include "actionhandler_webserver.h"
#endif


ActionHandler * pActionHandler;
QMap< QString, ActionHandler::Action > ActionHandler::m_actionNameMap;

ActionHandler::ActionHandler()
    : QObject()
{
    m_webserver = 0;
    m_lircServer = 0;

    // Form the map
    m_actionNameMap["START"] = ACTION_PLAYER_START;
    m_actionNameMap["PAUSE"] = ACTION_PLAYER_PAUSERESUME;
    m_actionNameMap["STOP"] = ACTION_PLAYER_STOP;
    m_actionNameMap["SEEK_BACK"] = ACTION_PLAYER_SEEK_BACK;
    m_actionNameMap["SEEK_FORWARD"] = ACTION_PLAYER_SEEK_FORWARD;
    m_actionNameMap["QUEUE_NEXT"] = ACTION_QUEUE_NEXT;
    m_actionNameMap["QUEUE_PREVIOUS"] = ACTION_QUEUE_PREVIOUS;
    m_actionNameMap["QUEUE_CLEAR"] = ACTION_QUEUE_CLEAR;
    m_actionNameMap["LYRICS_EARLIER"] = ACTION_LYRIC_EARLIER;
    m_actionNameMap["LYRICS_LATER"] = ACTION_LYRIC_LATER;
    m_actionNameMap["VOLUME_UP"] = ACTION_PLAYER_VOLUME_UP;
    m_actionNameMap["VOLUME_DOWN"] = ACTION_PLAYER_VOLUME_DOWN;
    m_actionNameMap["RATING_DECREASE"] = ACTION_PLAYER_RATING_DECREASE;
    m_actionNameMap["RATING_INCREASE"] = ACTION_PLAYER_RATING_INCREASE;
    m_actionNameMap["QUIT"] = ACTION_QUIT;
    m_actionNameMap["MUSICPLAYER_START"] = ACTION_MUSICPLAYER_START;
    m_actionNameMap["PITCH_INCREASE"] = ACTION_PLAYER_PITCH_INCREASE;
    m_actionNameMap["PITCH_DECREASE"] = ACTION_PLAYER_PITCH_DECREASE;
    m_actionNameMap["TEMPO_INCREASE"] = ACTION_PLAYER_TEMPO_INCREASE;
    m_actionNameMap["TEMPO_DECREASE"] = ACTION_PLAYER_TEMPO_DECREASE;
    m_actionNameMap["TOGGLE_VOICE_REMOVAL"] = ACTION_PLAYER_TOGGLE_VOICE_REMOVAL;

    connect( pEventor, &Eventor::settingsChanged, this, &ActionHandler::settingsChanged );
}

ActionHandler::~ActionHandler()
{
    stop();
}

void ActionHandler::start()
{
#if defined (HAS_LIRC_SUPPORT)
    if ( pSettings->lircEnabled  )
    {
        m_lircServer = new ActionHandler_LIRC( 0 );

        connect( m_lircServer, &ActionHandler_LIRC::lircEvent, this, &ActionHandler::cmdAction );
    }
#endif

#if defined (HAS_HTTP_SUPPORT)
    if ( pSettings->httpEnabled )
    {
        m_webserver = new ActionHandler_WebServer( 0 );
        m_webserver->start();
    }
#endif

    // When a current song finishes, we want to play the next one automatically
    connect( pEventor, &Eventor::karaokeFinished, this, &ActionHandler::nextQueueItemKaraoke );

    // When a current music song finishes, we want to play the next one automatically
    connect( pEventor, &Eventor::musicFinished, this, &ActionHandler::nextQueueItemMusic );

    pNotification->settingsChanged();
}

void ActionHandler::stop()
{
#if defined (HAS_HTTP_SUPPORT)
    if ( m_webserver )
    {
        m_webserver->quit();
        m_webserver->wait();
        delete m_webserver;
        m_webserver = 0;
    }
#endif

#if defined (HAS_LIRC_SUPPORT)
    if ( m_lircServer )
    {
        delete m_lircServer;
        m_lircServer = 0;
    }
#endif
}

int ActionHandler::actionByName(const char *eventname)
{
    if ( !m_actionNameMap.contains( eventname ) )
        return -1;

    return (int) m_actionNameMap[eventname];
}

void ActionHandler::enqueueSong(QString singer, int id)
{
    pSongQueue->addSong( singer, id );

    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED )
        emit actionKaraokePlayerStart();
}

void ActionHandler::enqueueSong(QString singer, QString path)
{
    pSongQueue->addSong( singer, path );

    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED )
        emit actionKaraokePlayerStart();
}

void ActionHandler::dequeueSong(int id)
{
    // If the first song is removed, and its being played, skip it
    bool current = pSongQueue->removeSongById( id );

    if ( current && pCurrentState->playerState != CurrentState::PLAYERSTATE_STOPPED )
    {
        // If we have more songs in queue, switch the next one - otherwise stop
        if ( !pSongQueue->isEmpty() )
            playerStartAction();
        else
            playerStopAction();
    }
}

void ActionHandler::error(QString message)
{
    qDebug("ERROR: %s", qPrintable(message));
}

void ActionHandler::warning(QString message)
{
    qDebug("WARNING: %s", qPrintable(message));
}

bool ActionHandler::cmdAction(int event )
{
    switch ( event )
    {
    case ACTION_PLAYER_START:
        playerStartAction();
        break;

    case ACTION_PLAYER_PAUSERESUME:
        playerPauseAction();
        break;

    case ACTION_PLAYER_STOP:
        playerStopAction();
        break;

    case ACTION_PLAYER_SEEK_BACK:
        emit actionKaraokePlayerBackward();
        break;

    case ACTION_PLAYER_SEEK_FORWARD:
        emit actionKaraokePlayerForward();
        break;

    case ACTION_QUEUE_NEXT:
        nextQueueAction();
        break;

    case ACTION_QUEUE_PREVIOUS:
        prevQueueAction();
        break;

    case ACTION_QUEUE_CLEAR:
        emit actionQueueClear();
        break;

    case ACTION_LYRIC_EARLIER:
        emit actionKaraokePlayerLyricsEarlier();
        break;

    case ACTION_LYRIC_LATER:
        emit actionKaraokePlayerLyricsLater();
        break;

    case ACTION_PLAYER_VOLUME_DOWN:
        emit actionPlayerVolumeDown();
        break;

    case ACTION_PLAYER_VOLUME_UP:
        emit actionPlayerVolumeUp();
        break;

    case ACTION_PLAYER_RATING_DECREASE:
        emit actionKaraokePlayerRatingDecrease();
        break;

    case ACTION_PLAYER_RATING_INCREASE:
        emit actionKaraokePlayerRatingIncrease();
        break;

    case ACTION_QUIT:
        emit actionQuit();
        break;

    case ACTION_MUSICPLAYER_START:
        emit actionMusicPlayerPlay();
        break;

    case ACTION_PLAYER_PITCH_INCREASE:
        emit actionKaraokePlayerPitchIncrease();
        break;

    case ACTION_PLAYER_PITCH_DECREASE:
        emit actionKaraokePlayerPitchDecrease();
        break;

    case ACTION_PLAYER_TEMPO_INCREASE:
        emit actionKaraokePlayerTempoIncrease();
        break;

    case ACTION_PLAYER_TEMPO_DECREASE:
        emit actionKaraokePlayerTempoDecrease();
        break;

    case ACTION_PLAYER_TOGGLE_VOICE_REMOVAL:
        emit actionKaraokePlayerToggleVoiceRemoval();
        break;

    default:
        return false;
    }

    return true;
}

void ActionHandler::keyEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_F11 )
    {
        pMainWindow->toggleFullscreen();

        // Do not let mainwindow action trigger
        event->ignore();
        return;
    }

    if ( event->key() ==  Qt::Key_Escape )
    {
        if ( pMainWindow->isFullScreen() )
            pMainWindow->toggleFullscreen();
        else
            cmdAction( ACTION_PLAYER_STOP );
    }

    if ( event->key() == Qt::Key_Space )
        cmdAction( ACTION_PLAYER_PAUSERESUME );

    if ( event->key() == Qt::Key_X )
        cmdAction( ACTION_PLAYER_STOP );

    if ( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
        cmdAction( ACTION_PLAYER_START );

    if ( event->key() == Qt::Key_Left )
    {
        if ( event->modifiers() & Qt::ShiftModifier )
            cmdAction( ACTION_PLAYER_TEMPO_DECREASE );
        else
            cmdAction( ACTION_PLAYER_SEEK_BACK );
    }

    if ( event->key() == Qt::Key_Right )
    {
        if ( event->modifiers() & Qt::ShiftModifier )
            cmdAction( ACTION_PLAYER_TEMPO_INCREASE );
        else
            cmdAction( ACTION_PLAYER_SEEK_FORWARD );
    }

    if ( event->key() == Qt::Key_Period )
        cmdAction( ACTION_LYRIC_LATER );

    if ( event->key() == Qt::Key_Comma )
        cmdAction( ACTION_LYRIC_EARLIER );

    if ( event->key() == Qt::Key_Equal )
        cmdAction( ACTION_PLAYER_VOLUME_UP );

    if ( event->key() == Qt::Key_Minus )
        cmdAction( ACTION_PLAYER_VOLUME_DOWN );

    if ( event->key() == Qt::Key_Plus )
        cmdAction( ACTION_PLAYER_RATING_INCREASE );

    if ( event->key() == Qt::Key_Underscore )
        cmdAction( ACTION_PLAYER_RATING_DECREASE );

    if ( event->key() == Qt::Key_M )
        cmdAction( ACTION_MUSICPLAYER_START );

    if ( event->key() == Qt::Key_V )
        cmdAction( ACTION_PLAYER_TOGGLE_VOICE_REMOVAL );

    if ( event->key() == Qt::Key_N || event->key() == Qt::Key_Up )
    {
        if ( event->modifiers() & Qt::ShiftModifier )
            cmdAction( ACTION_PLAYER_PITCH_INCREASE );
        else
            cmdAction( ACTION_QUEUE_NEXT );
    }

    if ( event->key() == Qt::Key_P || event->key() == Qt::Key_Down )
    {
        if ( event->modifiers() & Qt::ShiftModifier )
            cmdAction( ACTION_PLAYER_PITCH_DECREASE );
        else
            cmdAction( ACTION_QUEUE_PREVIOUS );
    }

    if ( event->key() == Qt::Key_Z )
        cmdAction( ACTION_QUEUE_CLEAR );
}

void ActionHandler::nextQueueAction()
{
    // This triggets next current music or next Karaoke song. Note the STOPPED check - this is only for manual change,
    // and should not be used for automatic between-song change
    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED && pMusicCollectionMgr->isPlaying() )
        emit actionMusicQueueNext();
    else
        nextQueueItemKaraoke();
}

void ActionHandler::nextQueueItemKaraoke()
{
    Logger::debug("ActionHandler: next Karaoke requested");

    if ( pSongQueue->next() )
    {
        emit actionMusicPlayerStop();
        emit actionKaraokePlayerStart();
    }
    else
    {
        // If there are no more songs, we stop the current song and clear the queue
        pSongQueue->clear();
        emit actionKaraokePlayerStop();
    }
}

void ActionHandler::settingsChanged()
{
    stop();
    start();
}

void ActionHandler::prevQueueAction()
{
    // This triggets previous current music or next Karaoke song
    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED && pMusicCollectionMgr->isPlaying() )
        emit actionMusicQueuePrev();
    else
    {
        if ( pCurrentState->playerState != CurrentState::PLAYERSTATE_STOPPED && pCurrentState->playerPosition > 10000 )
        {
            cmdAction( ActionHandler::ACTION_PLAYER_SEEK_BACK );
            return;
        }

        if ( pSongQueue->prev() )
        {
            Logger::debug("ActionHandler: previous Karaoke requested");
            emit actionMusicPlayerStop();
            emit actionKaraokePlayerStart();
        }
        else
            pNotification->showMessage( tr("Already at the beginning of queue") );
    }
}

void ActionHandler::playerStartAction()
{
    // Start with an empty queue means start Music; otherwise start Karaoke
    if ( pSongQueue->isEmpty() )
        emit actionMusicPlayerPlay();
    else
        emit actionKaraokePlayerStart();
}

void ActionHandler::playerStopAction()
{
    // This stops the current music or Karaoke player, which will trigger musicFinished/karaokeFinished
    // and we do not want them to translate into automatic queueNext
    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED )
        emit actionMusicPlayerStop();
    else
        emit actionKaraokePlayerStop();
}

void ActionHandler::playerPauseAction()
{
    // This pauses/resumes the current music or Karaoke player
    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED )
        emit actionMusicPlayerPauseResume();
    else
        emit actionKaraokePlayerPauseResume();
}

void ActionHandler::nextQueueItemMusic()
{
    Logger::debug("ActionHandler: next music requested");
    emit actionMusicQueueNext();
}

