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

#ifndef ACTIONHANDLER_WEBSERVER_SOCKET_H
#define ACTIONHANDLER_WEBSERVER_SOCKET_H

#include <QObject>
#include <QTcpSocket>

#include "songqueue.h"

class QTcpSocket;

class ActionHandler_WebServer_Socket : public QObject
{
    Q_OBJECT

    public:
        ActionHandler_WebServer_Socket( QTcpSocket * httpsock, const SongQueueItem& m_currentSong );
        ~ActionHandler_WebServer_Socket();

    signals:
        // This runs in a different thread, so QueuedConnection is must
        void    queueAdd( QString singer, int id );
        void    queueRemove( int id );
        void    commandAction( int id );
        void    startKaraokeScan();

    private slots:
        void    readyRead();

    private:
        // Sends an error code, and closes the socket
        void    sendError( int code );

    private:
        bool    search( QJsonDocument& document );
        bool    addsong( QJsonDocument& document);
        bool    authinfo( QJsonDocument& document);
        bool    login( QJsonDocument& document);
        bool    logout( QJsonDocument& document);
        bool    queueList( QJsonDocument& document );
        bool    queueControl( QJsonDocument& document );
        bool    listDatabase( QJsonDocument& document );
        bool    controlStatus( QJsonDocument& document );
        bool    controlAdjust( QJsonDocument& document );
        bool    controlAction( QJsonDocument& document );
        bool    collectionInfo( QJsonDocument& document );
        bool    collectionControl( QJsonDocument& document );
        bool    settingsGet( QJsonDocument& document );
        bool    settingsSet( QJsonDocument& document );

        QString escapeHTML( QString orig );

        // True if the currently logged user is a Karaoke admin
        bool    isAdministrator();

        // Generates or verifies challenge string based on IP address
        QString generateChallenge();
        bool    verifyChallenge( const QString& code );

        void    sendData( const QByteArray& data, const QByteArray &type = "application/json", const QByteArray &extraheader = QByteArray() );
        void    redirect( const QString& url );

        QTcpSocket *    m_httpsock;

        // Http request body with headers
        QByteArray      m_httpRequest;

        // Parsing header results. If url is non-empty, header is parsed
        QString         m_url;
        QString         m_loggedName;
        QString         m_method;
        unsigned int    m_contentLength;
        QString         m_currentSong;

};

#endif // ACTIONHANDLER_WEBSERVER_SOCKET_H
