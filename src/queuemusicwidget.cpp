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

#include <QFileDialog>

#include "util.h"
#include "eventor.h"
#include "playerwidget.h"
#include "currentstate.h"
#include "queuemusicwidget.h"
#include "musiccollectionmanager.h"
#include "queuetableviewmodels.h"
#include "actionhandler.h"

#include "ui_queuemusicwidget.h"

QueueMusicWidget::QueueMusicWidget(QWidget *parent)
    : QWidget(parent)
{
    ui = 0;
    m_contentWidget = 0;
    m_player = 0;
    m_playerStatus = PlayerWidget::PLAYER_STOPPED;
    m_playText = tr("Nothing is being played");

    // Prepare the timer
    m_updateTimer.setInterval( 500 );
    m_updateTimer.setTimerType( Qt::CoarseTimer );
    connect( &m_updateTimer, SIGNAL(timeout()), this, SLOT(timerUpdatePlayer()) );

    // Events
    connect( pEventor, &Eventor::musicStarted, this, &QueueMusicWidget::musicStarted );
    connect( pEventor, &Eventor::musicStopped, this, &QueueMusicWidget::musicFinished );
    connect( pEventor, &Eventor::musicPausedResumed, this, &QueueMusicWidget::musicPausedResumed );
    connect( pEventor, &Eventor::musicVolumeChanged, this, &QueueMusicWidget::musicVolumeChanged );
    connect( pEventor, &Eventor::musicTagsChanged, this, &QueueMusicWidget::musicTagsChanged );

    connect( pEventor, &Eventor::karaokeStopped, this, &QueueMusicWidget::karaokeStopped );
    connect( pEventor, &Eventor::karaokeStarted, this, &QueueMusicWidget::karaokeStarted );

    setWindowTitle( tr("Music queue") );
}

QueueMusicWidget::~QueueMusicWidget()
{
    delete ui;
    delete m_contentWidget;
}

void QueueMusicWidget::updateCollection()
{
    m_model->updateCollection();
}

void QueueMusicWidget::buttonAddMusic()
{
    QStringList files = QFileDialog::getOpenFileNames( 0,
                                                      tr("Please choose music file(s)") );

    Q_FOREACH ( const QString& filename, files )
    {
        // We only allow dropping music files, so its extension must be in the supportedExtensions
        if ( !pMusicCollectionMgr->supportedExtensions().contains( Util::fileExtension( filename )) )
            continue;

        // Insert the file at this row
        pMusicCollectionMgr->insertMusic( -1, filename );
    }
}

void QueueMusicWidget::buttonRemoveMusic()
{
    if ( !ui->tableQueue->currentIndex().isValid() )
        return;

    pMusicCollectionMgr->removeMusic( ui->tableQueue->currentIndex().row() );
}

void QueueMusicWidget::findTextChanged(QString text)
{
    if ( text.isEmpty() )
        return;

    QModelIndexList data = m_model->match( m_model->index( 0, 0 ), Qt::ToolTipRole, text, 1, Qt::MatchContains | Qt::MatchWrap );

    if ( !data.isEmpty() )
        ui->tableQueue->setCurrentIndex( data[0] );
}

void QueueMusicWidget::clickedPlayerPlayPause()
{
    switch ( m_playerStatus )
    {
        case PlayerWidget::PLAYER_STOPPED:
            emit pActionHandler->actionMusicPlayerPlay();
            break;

        case PlayerWidget::PLAYER_PAUSED:
        case PlayerWidget::PLAYER_PLAYING:
            emit pActionHandler->actionMusicPlayerPauseResume();
            break;
    }
}

void QueueMusicWidget::clickedPlayerStop()
{
    emit pActionHandler->actionMusicPlayerStop();
}

void QueueMusicWidget::clickedPlayerPrev()
{
    emit pActionHandler->actionMusicQueuePrev();
}

void QueueMusicWidget::clickedPlayerNext()
{
    emit pActionHandler->actionMusicQueueNext();
}

void QueueMusicWidget::movedPlayerVolumeSlider(int percentage)
{
    emit pActionHandler->actionPlayerVolumeSet( percentage );
}


void QueueMusicWidget::closeEvent(QCloseEvent *event)
{
    // We force the window to hide, emit the event (to let MainWindow know it is closed so it can remove checkbox)
    // and then ignore the no-more-needed event
    hide();
    emit closed(this);
    event->ignore();
}

void QueueMusicWidget::show()
{
    // but when you delete a layout, it does not delete its widgets. So we need to:
    // 1. Create a layout
    QVBoxLayout * mainLayout = new QVBoxLayout();
    setLayout( mainLayout );

    // 2. Create a widget which would own the UI widgets (so when we delete it, they all got deleted)
    m_contentWidget = new QWidget();
    mainLayout->addWidget( m_contentWidget );

    // Call UIC-generated code
    ui = new Ui::QueueMusicWidget();
    ui->setupUi(m_contentWidget);

    // Dialog controls
    connect( ui->btnAdd, SIGNAL(clicked()), this, SLOT(buttonAddMusic()) );
    connect( ui->btnRemove, SIGNAL(clicked()), this, SLOT(buttonRemoveMusic()) );
    connect( ui->leFilterSingers, SIGNAL(textEdited(QString)), this, SLOT(findTextChanged(QString)) );

    // Add the player window
    QVBoxLayout * l = new QVBoxLayout( 0 );
    m_player = new PlayerWidget(0);
    m_player->hideCapabilities();
    l->addWidget( m_player );
    ui->groupPlayerControl->setLayout( l );

    // Player buttons
    connect( m_player, &PlayerWidget::buttonNext, this, &QueueMusicWidget::clickedPlayerNext );
    connect( m_player, &PlayerWidget::buttonPlayPause, this, &QueueMusicWidget::clickedPlayerPlayPause );
    connect( m_player, &PlayerWidget::buttonPrev, this, &QueueMusicWidget::clickedPlayerPrev );
    connect( m_player, &PlayerWidget::buttonStop, this, &QueueMusicWidget::clickedPlayerStop );
    connect( m_player, &PlayerWidget::volumeSliderMoved, this, &QueueMusicWidget::movedPlayerVolumeSlider );

    // Add the music files model
    m_model = new TableModelMusic();
    connect( pEventor, &Eventor::musicQueueChanged, this, &QueueMusicWidget::updateCollection );
    ui->tableQueue->setModel( m_model );

    // And fill it up
    m_model->updateCollection();

    // If we're playing, set the update for the player
    m_player->setText( m_playText );
    m_player->setStatus( m_playerStatus );
    m_player->setEnabled( !m_karaokePlaying );

    // If we're playing, arm the timer
    if ( m_playerStatus == PlayerWidget::PLAYER_PLAYING )
        m_updateTimer.start();

    // Restore the window size
    resize( pCurrentState->windowSizeMusicQueue );

    // Base class
    QWidget::show();
}

void QueueMusicWidget::hide()
{
    // Remember the window size
    pCurrentState->windowSizeMusicQueue = size();
    pCurrentState->saveTempData();

    // First we remove the layout from the qwidget (it will not free widgets)
    delete layout();

    // Here we free the widgets
    delete m_contentWidget;

    // This just deletes the UI - it doesn't own any widgets it created
    delete ui;

    // Cleanup
    ui = 0;
    m_contentWidget = 0;
    m_player = 0;   // deleted as child widget

    QWidget::hide();
}


void QueueMusicWidget::timerUpdatePlayer()
{
    // If we're not playing, stop the timer - no reason to waste CPU cycles while paused
    if ( m_playerStatus != PlayerWidget::PLAYER_PLAYING )
        m_updateTimer.stop();
    else
    {
        if ( m_player )
        {
            qint64 pos = 0, duration = 0;
            pMusicCollectionMgr->currentProgress( pos, duration );
            m_player->setProgress( pos, duration );
        }
    }
}


void QueueMusicWidget::musicStarted()
{
    // Remember the text and params
    m_playerStatus = PlayerWidget::PLAYER_PLAYING;

    // Are we visible? In this case show the text and get ready for updates
    if ( m_player )
    {
        m_player->setStatus( m_playerStatus );
        m_updateTimer.start();
    }
}

void QueueMusicWidget::musicPausedResumed(bool pause)
{
    m_playerStatus = pause ? PlayerWidget::PLAYER_PAUSED : PlayerWidget::PLAYER_PLAYING;

    if ( m_player )
    {
        m_player->setStatus( m_playerStatus );

        // If we resumed, restart the timer which was stopped
        if ( m_playerStatus == PlayerWidget::PLAYER_PLAYING )
            m_updateTimer.start();
    }
}

void QueueMusicWidget::musicVolumeChanged(int newvalue)
{
    m_playerVolume = newvalue;

    if ( m_player )
        m_player->setVolume( m_playerVolume );
}

void QueueMusicWidget::musicFinished()
{
    m_playText = tr("Nothing is being played");
    m_playerStatus = PlayerWidget::PLAYER_STOPPED;

    // Are we visible? In this case show the text and get ready for updates
    if ( m_player )
    {
        m_player->setStatus( m_playerStatus );
        m_player->setText( m_playText );
    }
}

void QueueMusicWidget::musicTagsChanged(QString artist, QString title)
{
    m_playText = tr("\"%1\" by %2") .arg(title) .arg(artist);

    if ( m_player )
        m_player->setText( m_playText );
}

void QueueMusicWidget::karaokeStarted(SongQueueItem )
{
    m_karaokePlaying = true;

    // Are we visible? Disable the player controls
    if ( m_player )
        m_player->setEnabled( false );
}

void QueueMusicWidget::karaokeStopped()
{
    m_karaokePlaying = false;

    // Are we visible? Enable the player controls
    if ( m_player )
        m_player->setEnabled( true );
}
