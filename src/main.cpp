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

#include "mainwindow.h"
#include "logger.h"
#include "crashhandler.h"


#include "midisyntheser.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#if defined (USE_BREAKPAD)
    CrashHandler crashhandler;

    // Are we invoked through crash handler?
    if ( crashhandler.isHandlingCrash() )
        return 0;

    // Register it
    crashhandler.registerHandler();
#endif

    // Set data for QSettings
    QCoreApplication::setOrganizationName("ulduzsoft");
    QCoreApplication::setOrganizationDomain("ulduzsoft.com");
    QCoreApplication::setApplicationName("spivak");

    // Init logger after crash settings
    Logger::init();

    QScopedPointer<MainWindow> p (new MainWindow());
    p.data()->show();

    a.exec();
    return 0;
}
