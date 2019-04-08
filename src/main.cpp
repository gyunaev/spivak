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
#include <QScopedPointer>
#include <QCommandLineParser>
#include <QRect>
#include <QScreen>

#if defined (Q_OS_WIN)
    #include <windows.h>
#endif

#include "actionhandler.h"
#include "mainwindow.h"
#include "logger.h"


#include "midisyntheser.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set data for QSettings
    QCoreApplication::setOrganizationName("ulduzsoft");
    QCoreApplication::setOrganizationDomain("ulduzsoft.com");
    QCoreApplication::setApplicationName("spivak");

    // Init logger after crash settings
    Logger::init();

    QScopedPointer<MainWindow> p (new MainWindow());
    p.data()->show();

    QCommandLineParser parser;

    const QCommandLineOption fullScreenOption(
                QStringList() << "fs" << "fullScreen",
                "Toggle full screen"
                );

    const QCommandLineOption fileNameOption(
                QStringList() << "file" << "fileName",
                "File name to play",
                "File name to play"
                );

    const QCommandLineOption displayOption(
                QStringList() << "d" << "displayNumber",
                "Display number",
                "Screen"
                );

    parser.addOption( fullScreenOption );
    parser.addOption( fileNameOption );
    parser.addOption( displayOption );
    parser.addHelpOption();

#if defined (Q_OS_WIN)
    const QCommandLineOption showConsoleOption(
                QStringList() << "c" << "console",
                "Show debug console"
                );

    parser.addOption( showConsoleOption );
#endif

    parser.process(a);

#if defined (Q_OS_WIN)
    // Show console on Windows
    if ( parser.isSet( showConsoleOption ) )
    {
        AllocConsole();

        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

        COORD coordInfo;
        coordInfo.X = 130;
        coordInfo.Y = 9000;

        SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coordInfo);
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),ENABLE_QUICK_EDIT_MODE| ENABLE_EXTENDED_FLAGS);
    }
#endif

    const int displayNumber = parser.value(displayOption).toInt();

    if ( displayNumber && displayNumber < a.screens().size() )
    {

        const QRect resolution = a.screens()[displayNumber]
                ->availableGeometry();

        p.data()->move(
                    resolution.x(),
                    resolution.y()
                    );
    }

    if ( parser.isSet(fullScreenOption) )
    {
        p.data()->toggleFullscreen();
    }

    const QString fileName = parser.value(fileNameOption);

    if(!fileName.isEmpty()) {
        p.data()->enqueueSong(fileName);
        QObject::connect(
                    pActionHandler,
                    &ActionHandler::actionKaraokePlayerStop,
                    qApp,
                    &QApplication::quit
                    );
    }

    return a.exec();
}
