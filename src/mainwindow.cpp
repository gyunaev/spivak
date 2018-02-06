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
#include <QLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialog>
#include <QTimer>
#include <QDesktopWidget>

#include "settings.h"
#include "mainwindow.h"
#include "karaokewidget.h"
#include "karaokesong.h"
#include "songqueue.h"
#include "database.h"
#include "notification.h"
#include "actionhandler.h"
#include "currentstate.h"
#include "version.h"
#include "queuekaraokewidget.h"
#include "queuemusicwidget.h"
#include "songdatabasescanner.h"
#include "settingsdialog.h"
#include "musiccollectionmanager.h"
#include "welcome_wizard.h"
#include "songqueue.h"
#include "logger.h"
#include "eventor.h"
#include "feedbackdialog.h"
#include "pluginmanager.h"

#include "ui_dialog_about.h"


MainWindow * pMainWindow;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), Ui::MainWindow()
{
    setupUi( this );
    pMainWindow = this;

    // Just in case someone tries to use those
    m_songScanner = 0;
    m_queueKaraokeWindow = 0;
    m_queueMusicWindow = 0;
    m_settings = 0;
    m_welcomeWizard = 0;

    qRegisterMetaType<SongQueueItem>();

    // We don't use any crypto
    qsrand( (unsigned int) (long) pMainWindow * (unsigned int) QDateTime::currentMSecsSinceEpoch() );

    // Eventor is created first as it does not connect to anything, and others connect to it
    pEventor = new Eventor( this );

    // Settings should be created right after eventor
    pSettings = new Settings();

    // Controller uses settings, and many classes uses it, so it is right here
    pActionHandler = new ActionHandler();

    // Then notification
    pNotification = new Notifications( 0 );

    // Current state doesn't connect to anything
    pCurrentState = new CurrentState( this );

    // Song database
    pDatabase = new Database( this );

    // Lyrics window should be created depending on the mode we use
    m_widget = new KaraokeWidget( this );
    setCentralWidget( m_widget );

    // Song queue should be created last, as it emits signals intercepted by other modules
    pSongQueue = new SongQueue( this );

    // This may load the queue
    pSongQueue->init();

    // Set up the music collection manager
    pMusicCollectionMgr = new MusicCollectionManager( this );

    // Load the song database
    pDatabase->init();
    pDatabase->getDatabaseCurrentState();

    if ( pCurrentState->m_databaseSongs > 0 )
        statusbar->showMessage( tr("Song database: %1 songs, %2 artists, last updated %3")
                                .arg( pCurrentState->m_databaseSongs )
                                .arg( pCurrentState->m_databaseArtists )
                                .arg( pCurrentState->m_databaseUpdatedDateTime ),
                                20000 );

    // Initialize the controller
    pActionHandler->start();

    // Connect slots
    connect( pActionHandler, &ActionHandler::actionQueueClear, pSongQueue, &SongQueue::clear );
    connect( pEventor, &Eventor::settingsChanged, this, &MainWindow::settingsChanged );

    // Quitter
    connect( pActionHandler, SIGNAL(actionQuit()), qApp, SLOT(quit()) );

    // Connect menu actions
    connect( action_Quit, &QAction::triggered, qApp, &QApplication::quit );
    connect( action_About, &QAction::triggered, this, &MainWindow::menuAbout );
    connect( actionPlay_file, &QAction::triggered, this, &MainWindow::menuOpenKaraoke );
    connect( actionSettings, &QAction::triggered, this, &MainWindow::menuSettings );
    connect( actionStart_playing, &QAction::triggered, this, &MainWindow::menuPlay);
    connect( actionPause, &QAction::triggered, this, &MainWindow::menuPause );
    connect( actionStop, &QAction::triggered, this, &MainWindow::menuStop );
    connect( actionPrevious_song_in_queue, &QAction::triggered, this, &MainWindow::menuQueuePrev );
    connect( actionNext_song_in_queue, &QAction::triggered, this, &MainWindow::menuQueueNext );
    connect( actionToggle_full_screen, &QAction::triggered, this, &MainWindow::menuToggleFullscreen );
    connect( actionSettings, &QAction::triggered, this, &MainWindow::menuSettings );
    connect( actionShow_karaoke_queue_window, &QAction::triggered, this, &MainWindow::menuToggleWindowQueueKaraoke );
    connect( actionShow_music_queue_window, &QAction::triggered, this, &MainWindow::menuToggleWindowQueueMusic );
    connect( actionRun_First_Time_Wizard, &QAction::triggered, this, &MainWindow::menuShowWelcomeWizard );
    connect( actionSend_the_feedback, &QAction::triggered, this, &MainWindow::menuSendFeedback );

    // Connect song scanner slots
    connect( pEventor, &Eventor::scanCollectionStarted, this, &MainWindow::scanCollectionStarted, Qt::QueuedConnection );
    connect( pEventor, &Eventor::scanCollectionProgress, this, &MainWindow::scanCollectionProgress, Qt::QueuedConnection );
    connect( pEventor, &Eventor::scanCollectionFinished, this, &MainWindow::scanCollectionFinished, Qt::QueuedConnection );

    // We only allow showing the music queue window if we have settings set
    if ( pSettings->musicCollections.isEmpty() )
        actionShow_music_queue_window->setEnabled( false );

    // Init the proper notification
    pNotification->songStopped();

    // And load the background
    m_widget->initBackground();

    // Set up the music collection manager
    pMusicCollectionMgr->initCollection();

    // Karaoke window - just created, not shown
    m_queueKaraokeWindow = new QueueKaraokeWidget( 0 );
    connect( m_queueKaraokeWindow, SIGNAL(closed(QObject*)), this, SLOT(windowClosed(QObject*)) );

    // Music window - just created, not shown
    m_queueMusicWindow = new QueueMusicWidget( 0 );
    connect( m_queueMusicWindow, SIGNAL(closed(QObject*)), this, SLOT(windowClosed(QObject*)) );

    // Plugin manager
    pPluginManager = new PluginManager();

    if ( !pSettings->firstTimeWizardShown )
        QTimer::singleShot( 100, this, &MainWindow::menuShowWelcomeWizard );

    // Restore size if we're not fullscreen
    if ( pSettings->startInFullscreen || hasCmdLineOption("-fs") )
        toggleFullscreen();
    else
        resize( pCurrentState->windowSizeMain );

    // Generate a crash if defined (to test crash handler)
    if ( hasCmdLineOption( "--crashplease") )
    {
        qDebug("crashing in 2 seconds");
        QTimer::singleShot( 2000, this, SLOT(generateCrash()) );
    }

    setScreensaverSuppression( true );
}

MainWindow::~MainWindow()
{
    setScreensaverSuppression( false );

    if ( m_songScanner )
    {
        m_songScanner->stopScan();
        delete m_songScanner;
    }

    delete m_queueKaraokeWindow;
    delete m_queueMusicWindow;
}

void MainWindow::settingsChanged()
{
    // We only allow showing the music queue window if we have settings set
    if ( pSettings->musicCollections.isEmpty() )
        actionShow_music_queue_window->setEnabled( false );
}

void MainWindow::menuOpenKaraoke()
{
    QString file = QFileDialog::getOpenFileName( 0, "Music file", "/home/tim/work/my/karaokeplayer/test/", "*.*");

    if ( file.isEmpty() )
        return;

    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_STOPPED )
        pSongQueue->clear();

    pActionHandler->enqueueSong( "", file );
}

void MainWindow::menuSettings()
{
    if ( m_settings )
    {
        m_settings->show();
        return;
    }

    m_settings = new SettingsDialog();
    connect( m_settings, SIGNAL(destroyed(QObject*)), this, SLOT(windowClosed(QObject*)) );
    m_settings->show();
}

void MainWindow::menuAbout()
{
    QDialog dlg;
    Ui::DialogAbout ui_about;

    ui_about.setupUi( &dlg );

    ui_about.labelAbout->setText( tr("<b>Spivak Karaoke Player version %1.%2</b><br><br>"
            "Copyright (C) George Yunaev 2015-2018, <a href=\"mailto:support@ulduzsoft.com\">support@ulduzsoft.com</a><br><br>"
            "Web site: <a href=\"http://www.ulduzsoft.com\">www.ulduzsoft.com/karplayer</a><br><br>"
            "This program is licensed under terms of GNU General Public License "
            "version 3; see LICENSE file for details.") .arg(APP_VERSION_MAJOR) .arg(APP_VERSION_MINOR) );

    ui_about.textThirdParty->setHtml( tr(
        "<qt>"
        "<h3>Qt toolkit library</h3>"
        "<p>Spivak uses a portable GUI/core library available from "
        "<a href=\"http://qt.nokia.com\">http://qt.nokia.com</a>. This toolkit library is "
        "available and used under LGPL license.</p>"
        "<h3>Icons</h3>"
        "<p>Spivak uses icons developed by DryIcons, "
        "<a href=\"http://www.dryicons.com\">http://www.dryicons.com</a>"
        "<h3>GStreamer</h3>"
        "<p>Spivak uses GStreamer library from <a href=\"http://gstreamer.org\">"
        "gstreamer.org</a> for playing music and video files</p>"
        "</qt>" ) );

    dlg.exec();
}

void MainWindow::toggleFullscreen()
{
    if ( isFullScreen() )
    {
        QApplication::restoreOverrideCursor();
        mainMenuBar->show();
        statusbar->show();
        showNormal();

        actionToggle_full_screen->setChecked( false );
    }
    else
    {
        mainMenuBar->hide();
        statusbar->hide();
        showFullScreen();

        // In case we run without screen manager
        move( 0, 0 );
        resize( QApplication::desktop()->size() );

        // Hide the mouse cursor
        QApplication::setOverrideCursor( Qt::BlankCursor );

        actionToggle_full_screen->setChecked( true );
    }
}

void MainWindow::menuPlay()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_PLAYER_START );
}

void MainWindow::menuPause()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_PLAYER_PAUSERESUME );
}

void MainWindow::menuStop()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_PLAYER_STOP );
}

void MainWindow::menuQueueNext()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_QUEUE_NEXT );
}

void MainWindow::menuQueuePrev()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_QUEUE_PREVIOUS );
}

void MainWindow::menuQueueClear()
{
    pActionHandler->cmdAction( ActionHandler::ACTION_QUEUE_CLEAR );
}

void MainWindow::menuToggleFullscreen()
{
    toggleFullscreen();

    // Show notification in case we're full screen
    if ( actionToggle_full_screen->isChecked() )
        pNotification->showMessage( tr("Press F11 on a keyboard to turn fullscreen mode off") );
}

void MainWindow::menuToggleWindowQueueKaraoke()
{
    if ( m_queueKaraokeWindow->isVisible() )
        m_queueKaraokeWindow->hide();
    else
        m_queueKaraokeWindow->show();
}

void MainWindow::menuToggleWindowQueueMusic()
{
    if ( m_queueMusicWindow->isVisible() )
        m_queueMusicWindow->hide();
    else
        m_queueMusicWindow->show();
}

void MainWindow::menuShowWelcomeWizard()
{
    if ( !isFullScreen() )
    {
        m_welcomeWizard = new WelcomeWizard(0);
        m_welcomeWizard->show();
    }
}

void MainWindow::menuSendFeedback()
{
    FeedbackDialog dlg;
    dlg.exec();
}


void MainWindow::windowClosed(QObject *widget)
{
    if ( widget == (QObject*) m_queueKaraokeWindow )
    {
        // Player dock window closed
        actionShow_karaoke_queue_window->setChecked( false );
    }
    else if ( widget == (QObject*) m_queueMusicWindow )
    {
        // Player dock window closed, and is already destroyed (no need to delete it)
        actionShow_music_queue_window->setChecked( false );
    }
    else if ( widget == m_settings )
    {
        m_settings = 0;
    }
    else
        abort();
}

bool MainWindow::karaokeDatabaseStartScan()
{
    m_songScanner = new SongDatabaseScanner();

    if ( !m_songScanner->startScan() )
    {
        QMessageBox::critical( 0,
                               tr("Failed to start scan"),
                               tr("Collection scan cannot be started as the language detection plugin couldn't be loaded"));
        return false;
    }

    return true;
}

void MainWindow::karaokeDatabaseAbortScan()
{
    // This will emit finished() but since it is queued, it will return before it gets anywhere
    if ( m_songScanner )
    {
        m_songScanner->stopScan();
        delete m_songScanner;
        m_songScanner = 0;
    }
}

bool MainWindow::karaokeDatabaseIsScanning() const
{
    return m_songScanner != 0;
}

void MainWindow::setScreensaverSuppression(bool supress)
{
#if defined (Q_OS_LINUX)
    // On Linux we use the xdg-screensaver tool
    QStringList args;

    if ( supress )
        args << "suspend";
    else
        args << "resume";

    args << QString::number( winId() );

    int ret = QProcess::execute( "xdg-screensaver", args );

    if ( ret == -1 )
        Logger::error( "xdg-screensaver tool is not installed, screensaver won't be disabled");

#elif defined (Q_OS_WIN)
    if ( supress )
        SetThreadExecutionState( ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED );
    else
        SetThreadExecutionState( ES_CONTINUOUS );
#endif
}

void MainWindow::scanCollectionStarted()
{
    statusbar->showMessage( "Collection scan started", 1000 );
}

void MainWindow::scanCollectionProgress( QString progressinfo )
{
    statusbar->showMessage( progressinfo, 1000 );
}

void MainWindow::scanCollectionFinished()
{
    statusbar->showMessage( "Collection scan finished", 5000 );
    delete m_songScanner;
    m_songScanner = 0;
}

void MainWindow::generateCrash()
{
    *((volatile char*) 0) = 1;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    pActionHandler->keyEvent( event );
}

void MainWindow::closeEvent(QCloseEvent *)
{
    pCurrentState->windowSizeMain = size();
    pCurrentState->saveTempData();

    m_widget->stopEverything();

    pActionHandler->stop();

    delete m_queueKaraokeWindow;
    delete m_queueMusicWindow;
    delete m_welcomeWizard;
    delete m_settings;

    m_queueKaraokeWindow = 0;
    m_queueMusicWindow = 0;
    m_welcomeWizard = 0;
    m_settings = 0;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *)
{
    toggleFullscreen();
}

bool MainWindow::hasCmdLineOption(const QString &option)
{
    for ( int i = 1; i < qApp->arguments().size(); i++ )
    {
        if ( qApp->arguments()[i] == option )
            return true;
    }

    return false;
}
