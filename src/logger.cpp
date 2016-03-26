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

#include <stdarg.h>
#include <stdio.h>

#include <QDir>
#include <QDateTime>
#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>

#include "logger.h"

static Logger * pLogger;

Logger::Logger()
{
}

void Logger::add( const char * type, const char *str)
{
    qDebug("%s %s", type, str );

    QMutexLocker m( &m_logMutex );

    if ( m_logfile.isOpen() )
    {
        QDateTime now = QDateTime::currentDateTime();
        QString prefix = QString("%1 %2 ") .arg( now.toString( "yyyy-MM-dd hh:mm:ss" )) .arg(type);
        m_logfile.write( qPrintable(prefix) );
        m_logfile.write( str );
        m_logfile.write( "\n" );
        m_logfile.flush();
    }
}

bool Logger::init()
{
    pLogger = new Logger();

    QStringList paths;

    paths << QApplication::applicationDirPath() << QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) << QStandardPaths::writableLocation( QStandardPaths::TempLocation );

    Q_FOREACH( const QString& path, paths )
    {
        QString logpath = path + QDir::separator() + "spivakplayer.log";

        pLogger->m_logfile.setFileName( logpath );

        if ( pLogger->m_logfile.open( QIODevice::WriteOnly ) )
            break;
    }

    if ( !pLogger->m_logfile.isOpen() )
    {
        QMessageBox::critical( 0, "Cannot write log file",
                               QString("Cannot write log file to %1: %2")
                               .arg( pLogger->m_logfile.fileName() ) .arg( pLogger->m_logfile.errorString()) );
        return false;
    }

    return true;
}

void Logger::debug(const QString &str)
{
    pLogger->add( "DEBUG", str.toUtf8() );
}

void Logger::debug(const char *fmt, ... )
{
    va_list vl;
    char buf[1024];

    va_start( vl, fmt );
    vsnprintf( buf, sizeof(buf) - 1, fmt, vl );
    va_end( vl );

    pLogger->add( "DEBUG", buf );
}

void Logger::error(const QString &str)
{
    pLogger->add( "ERROR", str.toUtf8() );
}

void Logger::error(const char *fmt, ... )
{
    va_list vl;
    char buf[1024];

    va_start( vl, fmt );
    vsnprintf( buf, sizeof(buf) - 1, fmt, vl );
    va_end( vl );

    pLogger->add( "ERROR", buf );
}
