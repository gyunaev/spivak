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

#include "playerwidget.h"
#include "queuekaraokewidget.h"
#include "database.h"
#include "actionhandler.h"
#include "currentstate.h"
#include "eventor.h"
#include "queuetableviewmodels.h"
#include "queuekaraokewidget_addeditdialog.h"

#include "ui_queuekaraokewidget.h"


QueueKaraokeWidget::QueueKaraokeWidget(QWidget *parent)
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
    connect( pEventor, &Eventor::karaokeStarted, this, &QueueKaraokeWidget::karaokeStarted );
    connect( pEventor, &Eventor::karaokeStopped, this, &QueueKaraokeWidget::karaokeStopped );
    connect( pEventor, &Eventor::karaokePausedResumed, this, &QueueKaraokeWidget::karaokePausedResumed );
    connect( pEventor, &Eventor::karaokeVolumeChanged, this, &QueueKaraokeWidget::karaokeVolumeChanged );

    setWindowTitle( tr("Karaoke queue") );
}


QueueKaraokeWidget::~QueueKaraokeWidget()
{
    delete m_contentWidget;
    delete ui;
}

void QueueKaraokeWidget::updateQueue()
{
    m_modelQueue->updateQueue();
}

void QueueKaraokeWidget::closeEvent(QCloseEvent *event)
{
    // We force the window to hide, emit the event (to let MainWindow know it is closed so it can remove checkbox)
    // and then ignore the no-more-needed event
    hide();
    emit closed(this);
    event->ignore();
}

void QueueKaraokeWidget::buttonAdd()
{
    QueueKaraokeWidget_AddEditDialog dlg ( m_modelQueue->singers() );

    if ( dlg.exec() == QDialog::Accepted )
        pSongQueue->addSong( dlg.params() );
}


void QueueKaraokeWidget::buttonEdit()
{
    if ( !ui->tableQueue->currentIndex().isValid() )
        return;

    QueueKaraokeWidget_AddEditDialog dlg ( m_modelQueue->singers() );

    dlg.setCurrentParams( m_modelQueue->itemAt( ui->tableQueue->currentIndex() ) );

    if ( dlg.exec() == QDialog::Accepted )
        pSongQueue->replaceSong( ui->tableQueue->currentIndex().row(), dlg.params() );
}

void QueueKaraokeWidget::buttonRemove()
{
    if ( !ui->tableQueue->currentIndex().isValid() )
        return;

    pSongQueue->removeSong( ui->tableQueue->currentIndex().row() );
}

void QueueKaraokeWidget::buttonSearch()
{
    if ( ui->leSearch->text().isEmpty() )
        return;

    m_modelSearch->performSearch( ui->leSearch->text() );
}

void QueueKaraokeWidget::findSingers(const QString &text)
{
    if ( text.isEmpty() )
        return;

    QModelIndexList data = m_modelQueue->match( m_modelQueue->index( 0, 0 ), Qt::UserRole, text, 1, Qt::MatchContains | Qt::MatchWrap );

    if ( !data.isEmpty() )
        ui->tableQueue->setCurrentIndex( data[0] );
}

void QueueKaraokeWidget::clickedPlayerPlayPause()
{
    if ( m_playerStatus == PlayerWidget::PLAYER_PLAYING || m_playerStatus == PlayerWidget::PLAYER_PAUSED )
        emit pActionHandler->actionKaraokePlayerPauseResume();
    else
        emit pActionHandler->actionKaraokePlayerStart();
}

void QueueKaraokeWidget::clickedPlayerStop()
{
    emit pActionHandler->actionKaraokePlayerStop();
}

void QueueKaraokeWidget::clickedPlayerPrev()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_QUEUE_PREVIOUS );
}

void QueueKaraokeWidget::clickedPlayerNext()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_QUEUE_NEXT );
}

void QueueKaraokeWidget::movedPlayerVolumeSlider(int percentage)
{
    emit pActionHandler->actionPlayerVolumeSet( percentage );
}

void QueueKaraokeWidget::pitchSpinChanged(int percentage)
{
    emit pActionHandler->actionKaraokePlayerChangePitch( percentage );
}

void QueueKaraokeWidget::tempoSpinChanged(int percentage)
{
    emit pActionHandler->actionKaraokePlayerChangeTempo( percentage );
}

void QueueKaraokeWidget::toggledVoiceRemoval(bool enabled)
{
    emit pActionHandler->actionKaraokePlayerChangeVoiceRemoval( enabled );
}


void QueueKaraokeWidget::show()
{
    // A layout takes kind of weird ownership of its widgets - you cannot delete them while it is present,
    // but when you delete a layout, it does not delete its widgets. So we need to:
    // 1. Create a layout
    QVBoxLayout * mainLayout = new QVBoxLayout();
    setLayout( mainLayout );

    // 2. Create a widget which would own the UI widgets (so when we delete it, they all got deleted)
    m_contentWidget = new QWidget();
    mainLayout->addWidget( m_contentWidget );

    // Call UIC-generated code
    ui = new Ui::QueueKaraokeWidget();
    ui->setupUi(m_contentWidget);

    // Add the player window into the groupbox
    QVBoxLayout * playerLayout = new QVBoxLayout( 0 );
    m_player = new PlayerWidget(0);
    playerLayout->addWidget( m_player );
    ui->groupPlayerControl->setLayout( playerLayout );

    // Player buttons
    connect( m_player, &PlayerWidget::buttonNext, this, &QueueKaraokeWidget::clickedPlayerNext );
    connect( m_player, &PlayerWidget::buttonPlayPause, this, &QueueKaraokeWidget::clickedPlayerPlayPause );
    connect( m_player, &PlayerWidget::buttonPrev, this, &QueueKaraokeWidget::clickedPlayerPrev );
    connect( m_player, &PlayerWidget::buttonStop, this, &QueueKaraokeWidget::clickedPlayerStop );
    connect( m_player, &PlayerWidget::volumeSliderMoved, this, &QueueKaraokeWidget::movedPlayerVolumeSlider );
    connect( m_player, &PlayerWidget::pitchSpinChanged, this, &QueueKaraokeWidget::pitchSpinChanged );
    connect( m_player, &PlayerWidget::tempoSpinChanged, this, &QueueKaraokeWidget::tempoSpinChanged );
    connect( m_player, &PlayerWidget::toggledVoiceRemoval, this, &QueueKaraokeWidget::toggledVoiceRemoval );

    connect( pEventor, &Eventor::karaokeParametersChanged, m_player, &PlayerWidget::applyCapabilities );

    // Buttons
    connect( ui->btnFind, &QAbstractButton::clicked, this, &QueueKaraokeWidget::buttonSearch );
    connect( ui->leSearch, &QLineEdit::returnPressed, this, &QueueKaraokeWidget::buttonSearch );
    connect( ui->btnAdd, &QAbstractButton::clicked, this, &QueueKaraokeWidget::buttonAdd );
    connect( ui->btnEdit, &QAbstractButton::clicked, this, &QueueKaraokeWidget::buttonEdit );
    connect( ui->tableQueue, &QAbstractItemView::doubleClicked, this, &QueueKaraokeWidget::buttonEdit );
    connect( ui->btnRemove, &QAbstractButton::clicked, this, &QueueKaraokeWidget::buttonRemove );
    connect( ui->leFilterSingers, &QLineEdit::textEdited, this, &QueueKaraokeWidget::findSingers );

    connect( pEventor, &Eventor::queueChanged, this, &QueueKaraokeWidget::updateQueue );

    // Queue model
    m_modelQueue = new TableModelQueue( this );
    m_modelQueue->updateQueue();
    ui->tableQueue->setModel( m_modelQueue );

    // Search model
    m_modelSearch = new TableModelSearch( this );
    ui->tableSearch->setModel( m_modelSearch );

    // If we're playing, set the update for the player
    m_player->setText( m_playText );
    m_player->setStatus( m_playerStatus );
    m_player->applyCapabilities();

    // If we're playing, arm the timer
    if ( m_playerStatus == PlayerWidget::PLAYER_PLAYING )
        m_updateTimer.start();

    // Restore the sizes
    resize( pCurrentState->windowSizeKarQueue );

    if ( !pCurrentState->splitterSizeKarQueue.isEmpty() )
        ui->splitter->setSizes( pCurrentState->splitterSizeKarQueue );

    // Base class
    QWidget::show();
}

void QueueKaraokeWidget::hide()
{
    // Remember the window data
    pCurrentState->windowSizeKarQueue = size();

    // Ui is deleted before hide() is called
    if ( ui )
        pCurrentState->splitterSizeKarQueue = ui->splitter->sizes();

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

    // Stop the timer, no need to call anything
    m_updateTimer.stop();

    // Save window sizes
    pCurrentState->saveTempData();

    QWidget::hide();
}

void QueueKaraokeWidget::timerUpdatePlayer()
{
    // If we're not playing, stop the timer - no reason to waste CPU cycles while paused
    if ( m_playerStatus != PlayerWidget::PLAYER_PLAYING )
        m_updateTimer.stop();
    else
    {
        if ( m_player )
            m_player->setProgress( pCurrentState->playerPosition, pCurrentState->playerDuration );
    }
}

void QueueKaraokeWidget::karaokeStarted(SongQueueItem song)
{
    // Remember the text and params
    m_playText = tr("\"%1\" by %2") .arg(song.title) .arg(song.artist);
    m_playerStatus = PlayerWidget::PLAYER_PLAYING;

    // Are we visible? In this case show the text and get ready for updates
    if ( m_player )
    {
        m_player->setStatus( m_playerStatus );
        m_player->setText( m_playText );
        m_player->applyCapabilities();
        m_updateTimer.start();
    }
}

void QueueKaraokeWidget::karaokePausedResumed(bool pause)
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

void QueueKaraokeWidget::karaokeVolumeChanged(int newvalue)
{
    m_playerVolume = newvalue;

    if ( m_player )
        m_player->setVolume( m_playerVolume );
}

void QueueKaraokeWidget::karaokeStopped()
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
