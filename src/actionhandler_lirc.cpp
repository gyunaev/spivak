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

#include "settings.h"
#include "logger.h"
#include "util.h"
#include "actionhandler_lirc.h"

ActionHandler_LIRC::ActionHandler_LIRC(QObject *parent) :
    QObject(parent)
{
    Logger::debug( "LIRC: initializing" );
    readMap();

    // Connect first, as we otherwise would miss the signals
    connect( &m_socket, SIGNAL(connected()), this, SLOT(connected()) );
    connect( &m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()) );
    connect( &m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()) );
    connect( &m_socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(error(QLocalSocket::LocalSocketError)) );

    lircConnect();
}

void ActionHandler_LIRC::connected()
{
    Logger::debug( "LIRC: connected" );
}

void ActionHandler_LIRC::disconnected()
{
    Logger::debug( "LIRC: disconnected" );

    // Reconnect
    lircConnect();
}

void ActionHandler_LIRC::error(QLocalSocket::LocalSocketError )
{
    Logger::debug( "LIRC: error connecting to the socket: %s", qPrintable( m_socket.errorString()) );
}

void ActionHandler_LIRC::readyRead()
{
    m_buffer.append( m_socket.readAll() );

    // Proceed the entries in the buffer
    int pos;

    while ( (pos = m_buffer.indexOf( '\n' ) ) != -1 )
    {
        QByteArray str = m_buffer.left( pos );

        //Logger::debug( QString("LIRC: received from remote %1") .arg( str.data() ) );
        m_buffer.remove( 0, pos + 1 );

        // Parse the data
        QByteArrayList data = str.split( ' ' );

        if ( data.size() != 4 )
            continue;

        // Skip followup commands
        if ( data[1] != "00" )
            continue;

        const char * keyname = data[2].data();
        int eventcode = ActionHandler::actionByName( keyname );

        if ( eventcode == -1 )
        {
            if ( !m_eventMap.contains( keyname ) )
            {
                Logger::debug( QString("LIRC: no translation found for key '%1', ignoring") .arg( keyname ) );
                continue;
            }

            eventcode = m_eventMap[keyname];
            Logger::debug( QString("LIRC: translation: key %1 is translated to action %2") .arg( keyname ) .arg(eventcode) );
        }
        else
            Logger::debug( QString("LIRC: key %1 generates action %2") .arg( keyname ) .arg(eventcode) );

        emit lircEvent( eventcode );
    }
}

void ActionHandler_LIRC::readMap()
{
    QFile f( pSettings->lircMappingFile );

    if ( !f.open( QIODevice::ReadOnly ) )
    {
        pActionHandler->warning( tr("Can't open LIRC mapping file %1: remote might not work") .arg( pSettings->lircMappingFile ) );
        return;
    }

    while ( !f.atEnd() )
    {
        QByteArray line = f.readLine();

        if ( line.isEmpty() || line.startsWith( '#') )
            continue;

        // REMOTE <command>
        int i = Util::indexOfSpace( line );

        if ( i == -1 )
            continue;

        QByteArray cmd = line.left( i );
        QByteArray event = line.mid( i + 1 ).trimmed();
        int eventcode = ActionHandler::actionByName( event.data() );

        if ( eventcode == -1 )
        {
            pActionHandler->warning( tr("Invalid event name %1") .arg( event.data() ) );
            continue;
        }

        m_eventMap[ cmd ] = eventcode;
    }
}

void ActionHandler_LIRC::lircConnect()
{
#ifdef Q_OS_WIN
    int sep = pSettings->lircDevicePath.indexOf( ':' );

    if ( sep != -1 )
        m_socket.connectToHost( pSettings->lircDevicePath.mid( 0, sep ), pSettings->lircDevicePath.mid( sep + 1 ).toInt() );
    else
        Logger::debug( "LIRC: invalid device address" );
#else
    m_socket.connectToServer( pSettings->lircDevicePath );
#endif
}
