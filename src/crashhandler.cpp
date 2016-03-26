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
#include <QStandardPaths>
#include <QDir>

#include "feedbackdialog.h"
#include "crashhandler.h"

// See https://blog.inventic.eu/2012/08/qt-and-google-breakpad/

#if defined (Q_OS_WIN)

#include "client/windows/handler/exception_handler.h"

static google_breakpad::ExceptionHandler * crashHandler;
static WCHAR * commandline;

static bool win_callback_minidump( const wchar_t* dump_dir, const wchar_t* minidump_id, void*, EXCEPTION_POINTERS*, MDRawAssertionInfo*, bool )
{
    // Append the directory and the file name into the prepared buffer
    wcscat( commandline, dump_dir );
    wcscat( commandline, L"/" );
    wcscat( commandline, minidump_id );
    wcscat( commandline, L".dmp\"" );

    // And start our own process with the relevant arguments
    PROCESS_INFORMATION pinfo;
    STARTUPINFO si;

    ZeroMemory( &si, sizeof(STARTUPINFO) );
    si.cb = sizeof(STARTUPINFO);

    CreateProcess( 0, commandline, 0, 0, FALSE, 0, 0, 0, &si, &pinfo );
    return true;
}

static bool platformRegisterHandler()
{
    commandline = new WCHAR[ 2 * MAX_PATH ];

    // toWCharArray does not null-terminate
    int length = qApp->applicationFilePath().toWCharArray( commandline );
    commandline[length] = L'\0';

    // Append the command-line
    wcscat( commandline, L" --crashreport \"");

    // Make the temp path UTF16
    WCHAR temppath[ MAX_PATH ];

    length = QStandardPaths::writableLocation( QStandardPaths::TempLocation ).toWCharArray( temppath );
    temppath[length] = L'\0';

    // Allocate the crash handler
    crashHandler = new google_breakpad::ExceptionHandler( temppath, 0, win_callback_minidump, 0, true );

    return true;
}


#elif defined (Q_OS_LINUX)

#include "client/linux/handler/exception_handler.h"

static google_breakpad::ExceptionHandler * crashHandler;
static char * commandline;

static bool linux_callback_minidump( const google_breakpad::MinidumpDescriptor &md, void *, bool )
{
    qDebug("handler called: %s", md.path() );
    // This is crash handler, no heap/stack use here!
    if ( fork() == 0 )
    {
        // we're in a child process
        execle( commandline, "--crashreport", md.path(), (char *)NULL, environ );
        exit(1);
    }

    return true;
}


static bool platformRegisterHandler()
{
    // Init the command line
    commandline = strdup( qApp->applicationFilePath().toUtf8().constData() );

    // Allocate the crash handler
    google_breakpad::MinidumpDescriptor md( QStandardPaths::writableLocation( QStandardPaths::TempLocation ).toStdString() );
    crashHandler = new google_breakpad::ExceptionHandler( md, /*FilterCallback*/ 0, linux_callback_minidump, /*context*/ 0, true, -1 );

    return true;
}

#elif defined (Q_OS_MAC)

#include "client/mac/handler/exception_handler.h"

static google_breakpad::ExceptionHandler * crashHandler;
static char * commandline;

static bool callback_minidump_mac( const char *dump_dir, const char *minidump_id, void *, bool )
{
    // This is crash handler, no heap/stack use here!
    if ( fork() == 0 )
    {
        // we're in a child process
        execle( commandline, "--crashreport", dump_dir, minidump_id, (char *)NULL );
        exit(1);
    }

    return true;
}

static bool platformRegisterHandler()
{
    commandline  = strdup( qApp->applicationFilePath().toUtf8().constData() );

    // Create the crash handler
    crashHandler = new google_breakpad::ExceptionHandler( QStandardPaths::writableLocation( QStandardPaths::TempLocation ).toUtf8().constData(), 0, callback_minidump_mac, 0, true, 0 );

    return true;
}
#else
    #error no crash handler
#endif




CrashHandler::CrashHandler()
{
}

bool CrashHandler::isHandlingCrash()
{
    for ( int i = 0; i < qApp->arguments().size(); i++ )
    {
        if ( qApp->arguments()[i] == "--crashreport" && i + 1 < qApp->arguments().size() )
        {
            QString crashfile;

            if ( qApp->arguments().size() > i + 2 )
                crashfile = qApp->arguments()[i + 1] + QDir::separator() + qApp->arguments()[i + 2];
            else
                crashfile = qApp->arguments()[i + 1];

            if ( !crashfile.endsWith( ".dmp") )
                crashfile += ".dmp";

            FeedbackDialog dlg( 0, crashfile );
            dlg.exec();
            return true;
        }
    }

    return false;
}

void CrashHandler::registerHandler()
{
    platformRegisterHandler();
}
