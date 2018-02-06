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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

#include "ui_mainwindow.h"

class KaraokeWidget;
class KaraokeSong;
class SongQueue;
class WebServer;
class SettingsDialog;
class SongDatabaseScanner;
class QueueKaraokeWidget;
class QueueMusicWidget;
class WelcomeWizard;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    public slots:
        // Settings changed
        void    settingsChanged();

        // Menu actions
        void    menuOpenKaraoke();
        void    menuSettings();
        void    menuAbout();
        void    menuPlay();
        void    menuPause();
        void    menuStop();
        void    menuQueueNext();
        void    menuQueuePrev();
        void    menuQueueClear();
        void    menuToggleFullscreen();

        void    menuToggleWindowQueueKaraoke();
        void    menuToggleWindowQueueMusic();

        void    menuShowWelcomeWizard();
        void    menuSendFeedback();

        // Toggle full screen mode
        void    toggleFullscreen();

        // Handling window closures
        void    windowClosed( QObject* widget );

        // Database rescan and update
        bool    karaokeDatabaseStartScan();
        void    karaokeDatabaseAbortScan();
        bool    karaokeDatabaseIsScanning() const;

        // Whether to deactivate or reactivate screensever supression
        void    setScreensaverSuppression ( bool supress );

    private slots:
        // Scan collection slots from Eventor
        void    scanCollectionStarted();
        void    scanCollectionProgress(QString progressinfo);
        void    scanCollectionFinished();

        // Crash generator to test symbol submitter
        void    generateCrash();

    private:
        void    keyPressEvent(QKeyEvent * event);
        void    closeEvent(QCloseEvent *);
        void	mouseDoubleClickEvent(QMouseEvent * event);

        bool    hasCmdLineOption( const QString& option );

    private:
        KaraokeWidget       *  m_widget;

        QueueKaraokeWidget  *  m_queueKaraokeWindow;
        QueueMusicWidget    *  m_queueMusicWindow;

        // Only when the scan is in progress
        SongDatabaseScanner *   m_songScanner;

        // To make sure we only have one Settings dialog
        SettingsDialog      *   m_settings;

        // To make sure we only have one WelcomeWizard
        WelcomeWizard       *   m_welcomeWizard;
};


extern MainWindow * pMainWindow;

#endif // MAINWINDOW_H
