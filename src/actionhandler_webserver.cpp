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

#include <QNetworkConfigurationManager>
#include <QNetworkInterface>
#include <QNetworkSession>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSettings>

#include "actionhandler.h"
#include "actionhandler_webserver.h"
#include "actionhandler_webserver_socket.h"
#include "mainwindow.h"
#include "eventor.h"
#include "logger.h"
#include "settings.h"

ActionHandler_WebServer::ActionHandler_WebServer( QObject * parent )
    : QThread( parent )
{
    m_listenPort = 0;
    m_networkSession = 0;
    m_httpServer = 0;

    // Everything should be owned by our thread
    moveToThread( this );

    // Learn when a new song is started, so we can remember this information
    connect( pEventor, &Eventor::karaokeStarted, this, &ActionHandler_WebServer::karaokeStarted, Qt::QueuedConnection );
}

void ActionHandler_WebServer::run()
{
    QNetworkConfigurationManager manager;

    if ( manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired )
    {
        // Get saved network configuration
        QSettings settings;
        const QString id = settings.value( "network/DefaultConfiguration", "" ).toString();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier( id );

        if (( config.state() & QNetworkConfiguration::Discovered ) != QNetworkConfiguration::Discovered)
            config = manager.defaultConfiguration();

        // Initiate a new session
        m_networkSession = new QNetworkSession(config, this);
        connect( m_networkSession, SIGNAL(opened()), this, SLOT(sessionOpened()) );
        m_networkSession->open();
    }
    else
    {
        sessionOpened();
    }

    // Run the thread loop
    exec();
}

void ActionHandler_WebServer::dnsLookupFinished(QHostInfo hinfo)
{
    QString name;

    if ( hinfo.error() == QHostInfo::NoError )
        name = hinfo.hostName();
    else if ( !hinfo.addresses().isEmpty() )
        name = hinfo.addresses().first().toString();

    if ( name.isEmpty() || m_listenPort == 0 )
    {
        emit pEventor->webserverUrlChanged( "" );
        return;
    }

    // No need for port if it is 80
    if ( m_listenPort == 80 )
        emit pEventor->webserverUrlChanged( QString( "http://%1" ).arg( name ) );
    else
        emit pEventor->webserverUrlChanged( QString( "http://%1:%2" ).arg( name ).arg( m_listenPort ) );
}

void ActionHandler_WebServer::sessionOpened()
{
    // Save the used configuration
    if ( m_networkSession )
    {
        QNetworkConfiguration config = m_networkSession->configuration();

        QString id;

        if (config.type() == QNetworkConfiguration::UserChoice)
            id = m_networkSession->sessionProperty( QLatin1String("UserChoiceConfiguration") ).toString();
        else
            id = config.identifier();

        QSettings settings;
        settings.setValue( "network/DefaultConfiguration", id );
    }

    // If we're reconnecting, delete the old one
    delete m_httpServer;
    m_httpServer = new QTcpServer(this);
    connect( m_httpServer, &QTcpServer::newConnection, this, &ActionHandler_WebServer::newHTTPconnection );

    // Attempt to listen on port 80, and if we can't, on backup port
    if ( m_httpServer->listen( QHostAddress::Any, 80 ) )
        m_listenPort = 80;
    else if ( m_httpServer->listen( QHostAddress::Any, pSettings->httpListenPort ) )
        m_listenPort = pSettings->httpListenPort;
    else
    {
        Logger::error( "WebServer: Unable to listen on port 80 nor %d", pSettings->httpListenPort );
        return;
    }

    Logger::debug( "WebServer: listening at port %d", m_listenPort );

    // Enumerate all local IP addresses
    foreach ( const QHostAddress &address, QNetworkInterface::allAddresses() )
    {
        if ( address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost) )
        {
            QHostInfo::lookupHost( address.toString(), this, SLOT(dnsLookupFinished(QHostInfo)) );
            break;
        }
    }
}

void ActionHandler_WebServer::karaokeStarted(SongQueueItem song)
{
    m_currentSong = song;
}

void ActionHandler_WebServer::newHTTPconnection()
{
    Logger::debug( "WebServer: new HTTP connection" );

    // Handle the new connection - it will be deleted by the connection itself via deleteLater
    new ActionHandler_WebServer_Socket( m_httpServer->nextPendingConnection(), m_currentSong );
}
