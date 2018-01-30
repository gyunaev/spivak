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

#include <QMimeDatabase>
#include <QMimeType>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkCookie>
#include <QHostAddress>
#include <QCryptographicHash>

#include "util.h"
#include "settings.h"
#include "logger.h"
#include "eventor.h"
#include "database.h"
#include "songqueue.h"
#include "actionhandler.h"
#include "currentstate.h"
#include "mainwindow.h"
#include "actionhandler_webserver_socket.h"

ActionHandler_WebServer_Socket::ActionHandler_WebServer_Socket(QTcpSocket *httpsock, const SongQueueItem& currentSong)
    : QObject()
{
    m_httpsock = httpsock;
    m_currentSong = "\"" + currentSong.title + "\" by " + currentSong.artist;

    // Autodelete the client once it is disconnected
    connect( m_httpsock, &QTcpSocket::disconnected, this, &ActionHandler_WebServer_Socket::deleteLater );

    // Read notification
    connect( m_httpsock, &QTcpSocket::readyRead, this, &ActionHandler_WebServer_Socket::readyRead );

    // Song addition and removal
    connect( this, SIGNAL(queueAdd(QString,int)), pActionHandler, SLOT(enqueueSong(QString,int)), Qt::QueuedConnection );
    connect( this, SIGNAL(queueRemove(int)), pActionHandler, SLOT(dequeueSong(int)), Qt::QueuedConnection );

    // Command actions
    connect( this, SIGNAL(commandAction(int)), pActionHandler, SLOT(cmdAction(int)), Qt::QueuedConnection );
}

ActionHandler_WebServer_Socket::~ActionHandler_WebServer_Socket()
{
    delete m_httpsock;
}

void ActionHandler_WebServer_Socket::readyRead()
{
    // Read the HTTP request
    m_httpRequest += m_httpsock->readAll();

    // Do we have a full HTTP header at least? If not, wait for more data
    int bodyidx = m_httpRequest.indexOf( "\r\n\r\n");

    if ( bodyidx == -1 )
        return;

    // So we have at least the header. Parse it if we haven't yet
    if ( m_url.isEmpty() )
    {
        QStringList header = QString::fromUtf8( m_httpRequest.left( bodyidx ) ).split( "\r\n" );

        // First line is formatted differently
        QRegExp regex( "^([A-Za-z]+)\\s+(.+)\\s+HTTP/\\d\\.\\d" );

        if ( header.isEmpty() || regex.indexIn( header[0] ) == -1 )
        {
            // Bad method
            sendError( 400 );
            return;
        }

        // Get the method and URL
        m_method = regex.cap( 1 );
        m_url = regex.cap( 2 );

        // Parse the headers, get all we need
        int content_length = 0;
        bool requires_expect_100 = false;

        for ( int i = 1; i < header.size(); i++ )
        {
            QRegExp regex( "^([A-Za-z0-9-]+):\\s+(.+)" );

            if ( regex.indexIn( header[i] ) == -1 )
            {
                Logger::error( "WebServer: bad header line %s, aborting", qPrintable(header[i]) );
                sendError( 400 );
                return;
            }

            QString hdr = regex.cap( 1 );
            QString value = regex.cap( 2 );

            if ( hdr.compare( "cookie", Qt::CaseInsensitive) == 0 )
            {
                // Parse the cookies.
                // Unfortunately QNetworkCookie::parseCookies does not support multiple cookies on a single line.
                // Thus we have to preprocess it ourselves
                foreach ( QString rawcookie, value.split( ';' ) )
                {
                    QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies( rawcookie.toLatin1() );

                    if ( cookies.length() != 1 )
                    {
                        qWarning( "Cookie parser error: invalid number returned" );
                        continue;
                    }

                    // Only one cookie here
                    QNetworkCookie cookie = cookies.first();

                    // Security issue: currently no checks are performed, so the user could modify cookie.
                    // In home use scenario however this is ok.
                    if ( cookie.name() == "token" && !cookie.value().isEmpty() )
                        m_loggedName = QByteArray::fromBase64( cookie.value() );
                }
            }
            else if ( hdr.compare( "content-length", Qt::CaseInsensitive) == 0 )
                content_length = value.toInt();
            else if ( hdr.compare( "expect", Qt::CaseInsensitive) == 0 && value.startsWith( "100") )
                requires_expect_100 = true;
            else if ( hdr.compare( "host", Qt::CaseInsensitive ) == 0 && !pSettings->httpForceUseHost.isEmpty() )
            {
                // This is useful if player machine has open WiFi and is used as captive portal
                if ( value.compare( pSettings->httpForceUseHost, Qt::CaseInsensitive ) != 0 )
                {
                    redirect( "http://" + pSettings->httpForceUseHost + "/login.html" );
                    return;
                }
            }
        }

        if ( content_length < 0 )
        {
            sendError( 400 );
            return;
        }

        m_contentLength = content_length;

        // Expect: 100 tells us that the data is not yet sent, and we need to tell the client to send it
        if ( requires_expect_100 )
        {
            m_httpsock->write( "HTTP/1.1 100 Continue\r\n\r\n" );
            return;
        }
    }

    // If we have content-length, ensure we got all the content
    if ( m_contentLength > 0 && m_httpRequest.size() - (bodyidx + 4) < (int) m_contentLength )
        return;

    QByteArray requestbody = m_httpRequest.mid( bodyidx + 4 );
    Logger::debug( "WwwServer: %s %s, logged: %s, body %s", qPrintable(m_method), qPrintable(m_url), qPrintable(m_loggedName), requestbody.constData() );

    // If we're asked for an HTML page which is not login.html,
    // and we are not logged in, redirect it to load login.html
    // (even without logging in we should allow loading css/js)
    if ( m_loggedName.isEmpty() && m_url.endsWith( ".html") && m_url != "/login.html" )
    {
        redirect( "/login.html" );
        return;
    }

    if ( m_url == "/" )
    {
        redirect( "/index.html" );
        return;
    }

    // Handle API requests
    if ( m_url.startsWith( "/api" ) )
    {
        if ( m_method.compare( "post", Qt::CaseInsensitive ) != 0 )
        {
            Logger::error( "WwwServer: API request using non-POST method" );
            sendError( 400 );
            return;
        }

        // Attempt to decode the JSON from the socket
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson( requestbody, &error);

        // Ensure that the document is valid
        if(error.error != QJsonParseError::NoError)
        {
            Logger::error( "WwwServer: invalid JSON data received: %s", requestbody.constData() );
            sendError( 400 );
            return;
        }

        bool res = false;

        // what do we need to call?
        if ( m_url == "/api/search" )
            res = search( document );
        else if ( m_url == "/api/addsong" )
            res = addsong( document );
        else if ( m_url == "/api/queue/list" )
            res = queueList(  document );
        else if ( m_url == "/api/browse" )
            res = listDatabase( document );
        else if ( m_url == "/api/queue/remove" )
            res = queueControl( document );
        else if ( m_url == "/api/control/status" )
            res = controlStatus( document );
        else if ( m_url == "/api/control/adjust" )
            res = controlAdjust( document );
        else if ( m_url == "/api/control/action" )
            res = controlAction( document );
        else if ( m_url == "/api/collection/info" )
            res = collectionInfo( document );
        else if ( m_url == "/api/auth/status" )
            res = authinfo( document );
        else if ( m_url == "/api/auth/login" )
            res = login( document );
        else if ( m_url == "/api/auth/logout" )
            res = logout( document );

        // Those are administrator only functions
        if ( isAdministrator() )
        {
            if ( m_url == "/api/collection/action" )
                res = collectionControl( document );
            else if ( m_url == "/api/settings/get" )
                res = settingsGet( document );
            else if ( m_url == "/api/settings/set" )
                res = settingsSet( document );
        }

        if ( !res )
        {
            Logger::error( "WwwServer: invalid API requested or parameters are wrong: %s %s", qPrintable(m_url), requestbody.constData() );
            sendError( 500 );
        }
    }
    else
    {

        // File serving - first look up in wwwroot
        QFile f;

        if ( !pSettings->httpDocumentRoot.isEmpty() )
        {
            f.setFileName( pSettings->httpDocumentRoot + m_url );
            f.open( QIODevice::ReadOnly );
        }

        if ( !f.isOpen() )
        {
            // Now look up in resources
            f.setFileName( ":/html" + m_url );
            f.open( QIODevice::ReadOnly );
        }

        if ( !f.isOpen() )
        {
            if ( pSettings->httpDocumentRoot.isEmpty() )
                Logger::debug( "WebServer: requested path %s is not found in resources", qPrintable(m_url) );
            else
                Logger::debug( "WebServer: requested path %s is not found in resources or %s", qPrintable(m_url), qPrintable(pSettings->httpDocumentRoot) );

            sendError( 404 );
            return;
        }

        QByteArray data = f.readAll();
        QByteArray type = QMimeDatabase().mimeTypeForFile( f.fileName() ).name().toUtf8();

        Logger::debug( "WebServer: serving content %s from %s, type %s", qPrintable(m_url), f.fileName().startsWith( ':' ) ? "resources" : qPrintable(f.fileName()), qPrintable(type) );
        sendData( data, type );
    }
}

void ActionHandler_WebServer_Socket::sendError(int code)
{
    QByteArray errormsg = "HTTP/1.0 " + QByteArray::number( code ) + " error\r\n\r\n";

    m_httpsock->write( errormsg );
    m_httpsock->flush();
    m_httpsock->disconnectFromHost();
}

void ActionHandler_WebServer_Socket::sendData(const QByteArray &data, const QByteArray &type, const QByteArray &extraheader)
{
    QByteArray header = "HTTP/1.1 200 ok\r\n"
              "Content-Length: " + QByteArray::number( data.length() ) + "\r\n"
            + "Content-Type: " + type + "\r\n"
            + "Connection: Close\r\n"
            + "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";

    if ( !extraheader.isEmpty() )
        header += extraheader;

    header += "\r\n";

    m_httpsock->write( header );
    m_httpsock->write( data );
    m_httpsock->disconnectFromHost();
}

void ActionHandler_WebServer_Socket::redirect(const QString &url)
{
    // Redirect to the login page
    QByteArray header = "HTTP/1.1 302 Moved\r\n"
              "Location: " + url.toLatin1() + "\r\n"
              "Expires: Thu, 01 Jan 1970 00:00:01 GMT"
              "\r\n\r\n";

    m_httpsock->write( header );
    m_httpsock->disconnectFromHost();
}

bool ActionHandler_WebServer_Socket::search( QJsonDocument& document )
{
    QJsonObject obj = document.object();

    if ( !obj.contains( "query" ) )
        return false;

    Logger::debug("WebServer: searching database for %s", qPrintable(obj["query"].toString()));

    QList< Database_SongInfo > results;
    QJsonArray out;

    if ( pDatabase->search( obj["query"].toString(), results ) )
    {
        Q_FOREACH( const Database_SongInfo& res, results )
        {
            QJsonObject rec;

            rec[ "id" ] = res.id;
            rec[ "artist" ] = escapeHTML( res.artist );
            rec[ "title"] = escapeHTML( res.title );
            rec[ "type"] = res.type;
            rec[ "rating"] = res.rating;
            rec[ "language"] = escapeHTML( res.language );

            out.append( rec );
        }
    }

    sendData( QJsonDocument( out ).toJson() );
    return true;
}

bool ActionHandler_WebServer_Socket::addsong( QJsonDocument& document )
{
    if ( pSettings->httpEnableAddQueue )
    {
        QJsonObject obj = document.object();

        if ( !obj.contains( "id" ) || m_loggedName.isEmpty() )
            return false;

        int id = obj["id"].toInt();
        QString singer = m_loggedName;

        Database_SongInfo info;

        if ( !pDatabase->songById( id, info ) || singer.isEmpty() )
        {
            Logger::debug("WebServer: failed to add song %d: %s", singer.isEmpty() ? "singer is empty" : "song not found" );

            QJsonObject out;
            out["result"] = 0;
            out["reason"] = "Song not found";

            sendData( QJsonDocument( out ).toJson() );
            return true;
        }

        Logger::debug("WebServer: %s added song %d: %s", qPrintable(singer), id, qPrintable( info.filePath ) );
        emit queueAdd( singer, id );

        QJsonObject out;
        out["result"] = 1;
        out["title"] = escapeHTML( info.title );
        out["artist"] = escapeHTML( info.artist );

        sendData( QJsonDocument( out ).toJson() );
    }
    else
    {
        QJsonObject out;
        out["result"] = 0;
        out["reason"] = "Disabled in configuration";
        sendData( QJsonDocument( out ).toJson() );

        Logger::debug("WebServer: adding songs to database is disabled in configuration");
    }

    return true;
}

bool ActionHandler_WebServer_Socket::authinfo(QJsonDocument &)
{
    QJsonObject out;

    if ( !m_loggedName.isEmpty() )
    {
        out["loggedin"] = true;
        out["name"] = m_loggedName;
    }
    else
    {
        out["loggedin"] = false;

        // Is the HTTP access secured by code?
        if ( !pSettings->httpAccessCode.isEmpty() )
        {
            out["secured"] = true;
            out["challenge"] = generateChallenge();
        }
        else
            out["secured"] = false;
    }

    sendData( QJsonDocument( out ).toJson() );
    return true;
}

bool ActionHandler_WebServer_Socket::login(QJsonDocument &document)
{
    QJsonObject obj = document.object();

    if ( !obj.contains( "name" ) )
        return false;

    // If we run a secured server and don't have an access code, or it is not valid, we return the challenge and deny login
    if ( !pSettings->httpAccessCode.isEmpty()
         && ( !obj.contains( "code" ) || !verifyChallenge( obj.value( "code" ).toString() ) ) )
    {
        QJsonObject out;
        out["loggedin"] = false;
        out["secured"] = true;
        out["challenge"] = generateChallenge();

        sendData( QJsonDocument( out ).toJson() );
        return true;
    }

    // Here we always log the user in
    m_loggedName = obj.value( "name" ).toString();

    QJsonObject out;
    out["loggedin"] = true;
    out["source"] = obj.value( "source" );
    out["name"] = m_loggedName;

    // Set the cookie to base64 encoding of name
    QNetworkCookie cookie;
    cookie.setName( "token" );
    cookie.setValue( m_loggedName.toUtf8().toBase64() );
    cookie.setPath( "/" );

    sendData( QJsonDocument( out ).toJson(), "application/json", "Set-Cookie: " + cookie.toRawForm() + "\r\n" );
    return true;
}

bool ActionHandler_WebServer_Socket::logout(QJsonDocument &)
{
    QJsonObject out;

    out["loggedin"] = false;
    out["name"] = m_loggedName;

    QNetworkCookie cookie;
    cookie.setName( "token" );
    cookie.setPath( "/" );

    sendData( QJsonDocument( out ).toJson(), "application/json", "Set-Cookie: " + cookie.toRawForm() + "\r\n" );
    return true;
}

bool ActionHandler_WebServer_Socket::queueList(QJsonDocument &)
{
    // Get the song list and currently scheduled song
    QList<SongQueueItem> queue = pSongQueue->exportQueue();
    unsigned int current = pSongQueue->currentItem();

    QJsonArray out;

    for ( int i = current; i < queue.size(); i++ )
    {
        QJsonObject rec;

        rec[ "id" ] = queue[i].id;
        rec[ "singer" ] = escapeHTML( queue[i].singer );
        rec[ "title"] = escapeHTML( queue[i].title );
        rec[ "state"] = escapeHTML( queue[i].stateText() );

        if ( queue[i].singer == m_loggedName )
            rec[ "removable"] = true;

        out.append( rec );
    }

    sendData( QJsonDocument( out ).toJson() );
    return true;
}

bool ActionHandler_WebServer_Socket::queueControl(QJsonDocument &document)
{
    if ( pSettings->httpEnableAddQueue )
    {
        QJsonObject obj = document.object();

        if ( !obj.contains( "id" ) )
            return false;

        int id = obj.value( "id" ).toInt();
        emit queueRemove( id );
        Logger::debug("WebServer: queued the removal of ID %d from queue", id );
    }

    return queueList(document);
}

bool ActionHandler_WebServer_Socket::listDatabase(QJsonDocument &document)
{
    QJsonObject obj = document.object();
    QJsonArray out;
    QString type;

    if ( obj.contains( "artist" ) )
    {
        QString artist = obj["artist"].toString();

        // List songs by artist or by letter
        if ( artist.length() == 1 )
        {
            QStringList artists;

            pDatabase->browseArtists( artist[0], artists );

            Q_FOREACH( const QString& a, artists )
            {
                out.append( escapeHTML( a ) );
            }

            type = "artists";
        }
        else
        {
            QList<Database_SongInfo> results;

            pDatabase->browseSongs( artist, results );

            Q_FOREACH( const Database_SongInfo& res, results )
            {
                QJsonObject rec;

                rec[ "id" ] = res.id;
                rec[ "artist" ] = escapeHTML( res.artist );
                rec[ "title"] = escapeHTML( res.title );
                rec[ "type"] = res.type;
                rec[ "rating"] = res.rating;
                rec[ "language"] = escapeHTML( res.language );

                out.append( rec );
            }
        }

        type = "songs";
    }
    else
    {
        // List letters
        QList<QChar> initials;

        pDatabase->browseInitials( initials );

        Q_FOREACH( QChar ch, initials )
        {
            out.append( QString(ch) );
        }

        type = "initials";
    }

    QJsonObject outobj;
    outobj["results"] = out;
    outobj["type"] = type;

    sendData( QJsonDocument( outobj ).toJson() );
    return true;
}

bool ActionHandler_WebServer_Socket::controlStatus(QJsonDocument &)
{
    QJsonObject outobj;

    if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_PLAYING || pCurrentState->playerState == CurrentState::PLAYERSTATE_PAUSED )
    {
        if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_PAUSED )
            outobj["state"] = "paused";
        else
            outobj["state"] = "playing";

        outobj["pos"] = Util::tickToString( pCurrentState->playerPosition );
        outobj["duration"] = Util::tickToString( pCurrentState->playerDuration );
        outobj["volume"] = pCurrentState->playerVolume;
        outobj["delay"] = pCurrentState->playerLyricsDelay;

        if ( pCurrentState->playerCapabilities & MediaPlayer::CapChangePitch )
            outobj["pitch"] = pCurrentState->playerPitch;
        else
            outobj["pitch"] = "disabled";

        if ( pCurrentState->playerCapabilities & MediaPlayer::CapChangeTempo )
            outobj["tempo"] = pCurrentState->playerTempo + 50;
        else
            outobj["tempo"] = "disabled";

        if ( pCurrentState->playerCapabilities & MediaPlayer::CapVoiceRemoval )
            outobj["voiceremoval"] = pCurrentState->playerVoiceRemovalEnabled;
        else
            outobj["voiceremoval"] = "disabled";

        outobj["song"] = escapeHTML( m_currentSong );
    }
    else
        outobj["state"] = "stopped";

    outobj["type"] = "controlstatus";

    sendData( QJsonDocument( outobj ).toJson() );
    return true;

}

bool ActionHandler_WebServer_Socket::controlAdjust(QJsonDocument &document)
{
    QJsonObject obj = document.object();

    if ( !obj.contains( "a" ) || !obj.contains( "v" ) )
        return false;

    QString action = obj.value( "a" ).toString();
    int p  = obj.value( "v" ).toInt();
    int useaction = -1;

    if ( action == "volume" )
    {
        if ( p > 0 )
            useaction = ActionHandler::ACTION_PLAYER_VOLUME_UP;
        else
            useaction = ActionHandler::ACTION_PLAYER_VOLUME_DOWN;
    }
    else if ( action == "tempo" )
    {
        if ( p > 0 )
            useaction = ActionHandler::ACTION_PLAYER_TEMPO_INCREASE;
        else
            useaction = ActionHandler::ACTION_PLAYER_TEMPO_DECREASE;
    }
    else if ( action == "pitch" )
    {
        if ( p > 0 )
            useaction = ActionHandler::ACTION_PLAYER_PITCH_INCREASE;
        else
            useaction = ActionHandler::ACTION_PLAYER_PITCH_DECREASE;

    }
    else if ( action == "voice" )
    {
        useaction = ActionHandler::ACTION_PLAYER_TOGGLE_VOICE_REMOVAL;
    }
    else if ( action == "delay" )
    {
        if ( p > 0 )
            useaction = ActionHandler::ACTION_LYRIC_EARLIER;
        else
            useaction = ActionHandler::ACTION_LYRIC_LATER;
    }

    if ( useaction == -1 )
        return false;

    // Send the action to the thread
    emit commandAction( useaction );

    return controlStatus( document );
}

bool ActionHandler_WebServer_Socket::controlAction(QJsonDocument &document)
{
    QJsonObject obj = document.object();

    if ( !obj.contains( "a" ) )
        return false;

    QString action = obj.value( "a" ).toString();

    if ( action == "stop" )
        emit commandAction( ActionHandler::ACTION_PLAYER_STOP );
    else if ( action == "prev" )
        emit commandAction( ActionHandler::ACTION_QUEUE_PREVIOUS );
    else if ( action == "next" )
        emit commandAction( ActionHandler::ACTION_QUEUE_NEXT );
    else if ( action == "playpause" )
    {
        if ( pCurrentState->playerState == CurrentState::PLAYERSTATE_PLAYING || pCurrentState->playerState == CurrentState::PLAYERSTATE_PAUSED )
            emit commandAction( ActionHandler::ACTION_PLAYER_PAUSERESUME );
        else
            emit commandAction( ActionHandler::ACTION_PLAYER_START );
    }
    else
        return false;

    return controlStatus( document );
}

bool ActionHandler_WebServer_Socket::collectionInfo(QJsonDocument &)
{
    QJsonObject outobj;

    outobj["songs"] = (int) pCurrentState->m_databaseSongs;
    outobj["artists"] = (int) pCurrentState->m_databaseArtists;
    outobj["updated"] = pCurrentState->m_databaseUpdatedDateTime;

    sendData( QJsonDocument( outobj ).toJson() );
    return true;
}

bool ActionHandler_WebServer_Socket::collectionControl(QJsonDocument &document)
{
    QJsonObject obj = document.object();

    if ( !obj.contains( "a" ) )
        return false;

    // We connect it now as we would use it only in this member
    connect( this, SIGNAL( startKaraokeScan()), pMainWindow, SLOT(karaokeDatabaseStartScan()), Qt::QueuedConnection );

    QString action = obj.value( "a" ).toString();

    if ( action == "rescan" )
    {
        QJsonObject outobj;

        if ( pMainWindow->karaokeDatabaseIsScanning() )
        {
            outobj["success"] = false;
            outobj["error"] = "scan in progress";
        }
        else
        {
            emit startKaraokeScan();
            outobj["success"] = true;
        }

        sendData( QJsonDocument( outobj ).toJson() );
        return true;
    }

    return false;
}

bool ActionHandler_WebServer_Socket::settingsGet(QJsonDocument &)
{
    QJsonObject out;
    out["settings"] = pSettings->toJson();

    sendData( QJsonDocument( out ).toJson() );
    return true;
}

bool ActionHandler_WebServer_Socket::settingsSet(QJsonDocument &document)
{
    QJsonObject obj = document.object();

    if ( !obj.contains( "settings" ) || !obj["settings"].isObject() )
        return false;

    pSettings->fromJson( obj["settings"].toObject() );
    sendData( "{ \"success\" : true }" );
    return true;
}

// We do not take the reference since we modify the string
QString ActionHandler_WebServer_Socket::escapeHTML( QString orig )
{
    // toHtmlEscaped() does not escape '
    return orig.replace( '&', "&amp;" )
            .replace( '<', "&lt;" )
            .replace( '>', "&gt;" )
            .replace( '"', "&quot;" )
            .replace( '\'', "&#039;" );
}

bool ActionHandler_WebServer_Socket::isAdministrator()
{
    // FIXME
    return true;
}

QString ActionHandler_WebServer_Socket::generateChallenge()
{
    // Generates challenge string based on IP address to kind of encrypt the access code.
    // This is not secure (only against script kiddies), and in future should be replaced
    // by IP-based session - which unfortunately needs tight integration with DHCP server
    // to be secured (otherwise someone can eavesdrop on a user logging in, spoof the creds,
    // kick the user out of network, spoof MAC and IP, and use the same creds to log in)
    QString challenge = m_httpsock->peerAddress().toString().toLatin1().toBase64();
    return challenge;
}

bool ActionHandler_WebServer_Socket::verifyChallenge(const QString &code)
{
    QByteArray valid = QCryptographicHash::hash( (generateChallenge() + pSettings->httpAccessCode).toUtf8(), QCryptographicHash::Md5 ).toHex();

    if ( code.compare( valid, Qt::CaseInsensitive ) == 0 )
        return true;

    qDebug("Incorrect challenge entereed: %s, must be: %s", qPrintable(code), valid.constData() );
    return false;
}
