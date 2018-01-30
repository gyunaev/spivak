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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QThread>
#include <QHostInfo>

#include "songqueue.h"

class QTcpServer;
class QNetworkSession;


// A built-in web server running in a dedicated thread (so it doesn't block our main thread)
class ActionHandler_WebServer : public QThread
{
    Q_OBJECT

    public:
        ActionHandler_WebServer( QObject * parent );

        QString webURL() const;

    private slots:
        void    dnsLookupFinished(QHostInfo hinfo);
        void    newHTTPconnection();
        void    sessionOpened();
        void    karaokeStarted(SongQueueItem song );

    private:
        void run();

        unsigned short      m_listenPort;

        QNetworkSession *   m_networkSession;
        QTcpServer      *   m_httpServer;
        SongQueueItem       m_currentSong;
};

#endif // WEBSERVER_H
