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

#include <QFile>
#include <QDir>
#include <QUuid>
#include <QVariant>
#include <QJsonDocument>
#include <QTextStream>

#include "sqlite3.h"

#include "actionhandler.h"
#include "currentstate.h"
#include "settings.h"
#include "database.h"
#include "database_statement.h"
#include "util.h"
#include "logger.h"

static const int CURRENT_DB_SCHEMA_VERSION = 1;

Database * pDatabase;


Database::Database(QObject *parent)
    : QObject( parent )
{
    m_sqlitedb = 0;
}

Database::~Database()
{
    if ( m_sqlitedb )
        sqlite3_close_v2( m_sqlitedb );
}

bool Database::init()
{
    if ( sqlite3_open_v2( pSettings->songdbFilename.toUtf8().data(),
                           &m_sqlitedb,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                           0 ) != SQLITE_OK )
    {
        pActionHandler->error( QString("Error opening sqlite database: %1") .arg( sqlite3_errmsg(m_sqlitedb) ) );
        return false;
    }

    if ( !recreateSongTable() )
        return false;

    // Setting table
    if ( !execute( "CREATE TABLE IF NOT EXISTS settings( version INTEGER, identifier TEXT, lastupdated INT)" ) )
        return false;

    // Verify/update version
    if ( verifyDatabaseVersion() )
        return false;

    return true;
}

bool Database::songById(int id, Database_SongInfo &info)
{
    Database_Statement stmt;

    if ( !stmt.prepareSongQuery( m_sqlitedb, QString("WHERE rowid=%1") .arg(id) ) )
        return false;

    if ( stmt.step() != SQLITE_ROW )
        return false;

    info = stmt.getRowSongInfo();
    return true;
}

bool Database::songByPath(const QString &path, Database_SongInfo &info)
{
    Database_Statement stmt;

    if ( !stmt.prepareSongQuery( m_sqlitedb, "WHERE path=?", QStringList() << pSettings->replacePath( path ) ) )
        return false;

    if ( stmt.step() != SQLITE_ROW )
        return false;

    info = stmt.getRowSongInfo();
    return true;
}

void Database::updatePlayedSong(int id, int newdelay, int newrating)
{
    if ( execute( "BEGIN TRANSACTION" ) )
    {
        // Update rating and last played
        execute( QString("UPDATE songs SET played=played+1,lastplayed=DATETIME(),rating=%1 WHERE rowid=%2") .arg(newrating) .arg(id) );

        // Update delay if it is nonzero or if it is already present
        QJsonObject params = getSongParams( id );

        if ( newdelay != 0 || params.contains( "delay") )
        {
            params[ "delay" ] = newdelay;
            setSongParams( id, params );
        }

        execute( "COMMIT TRANSACTION" );
    }
}

bool Database::browseInitials( QList<QChar>& artistInitials)
{
    artistInitials.clear();

    Database_Statement stmt;

    if ( !stmt.prepare( m_sqlitedb, "SELECT DISTINCT(SUBSTR(artist, 1, 1)) from songs ORDER BY artist") )
        return false;

    while ( stmt.step() == SQLITE_ROW )
        artistInitials.append( stmt.columnText( 0)[0] );

    return !artistInitials.empty();
}

bool Database::browseArtists(const QChar &artistInitial, QStringList &artists)
{
    artists.clear();

    Database_Statement stmt;

    if ( !stmt.prepare( m_sqlitedb, "SELECT DISTINCT(artist) FROM songs WHERE artist LIKE ? ORDER BY artist", QStringList() << QString("%1%%") .arg(artistInitial) ) )
        return false;

    while ( stmt.step() == SQLITE_ROW )
        artists.append( stmt.columnText(0) );

    return !artists.empty();
}

bool Database::browseSongs(const QString &artist, QList<Database_SongInfo> &results)
{
    results.clear();

    Database_Statement stmt;

    if ( !stmt.prepareSongQuery( m_sqlitedb, "WHERE artist=? ORDER BY title", QStringList() << artist ) )
        return false;

    while ( stmt.step() == SQLITE_ROW )
        results.append( stmt.getRowSongInfo() );

    return !results.empty();
}

bool Database::updateDatabase(const QList<SongDatabaseScanner::SongDatabaseEntry> entries)
{
    if ( !execute( "BEGIN TRANSACTION" ) )
        return false;

    Q_FOREACH( const SongDatabaseScanner::SongDatabaseEntry& e, entries )
    {
        // We use a separate search field since sqlite is not necessary built with full Unicode support (nor we want it to be)
        QString search = e.artist.toUpper() + " " + e.title.toUpper();
        QString path = e.filePath;

        // For non-local collections append the music to file path if we have it
        if ( pSettings->collections[e.colidx].type != CollectionProvider::TYPE_FILESYSTEM && !e.musicPath.isEmpty() )
            path += "|" + e.musicPath;

        QStringList params;
        params << path << e.artist << e.title << e.type << search << e.language;

        // "path TEXT PRIMARY KEY, artist TEXT, title TEXT, type TEXT, search TEXT, played INT, lastplayed INT, added INT, rating INT, language INT, flags INT, collectionid INT, parameters TEXT"
        if ( !execute( QString("INSERT OR REPLACE INTO songs VALUES( ?, ?, ?, ?, ?, 0, 0, DATETIME(), 0, ?, %1, %2, '' )") .arg( e.flags) .arg(e.colidx), params ) )
        {
            execute( "ROLLBACK TRANSACTION" );
            return false;
        }
    }

    return execute( "COMMIT TRANSACTION" );
}

bool Database::updateLastScan()
{
    return execute( "UPDATE settings SET lastupdated=DATETIME()" );
}

bool Database::clearDatabase()
{
    if ( !execute( "DROP TABLE songs") )
        return false;

    recreateSongTable();
    execute( "UPDATE settings SET lastupdated=0" );
    getDatabaseCurrentState();
    return true;
}

qint64 Database::lastDatabaseUpdate() const
{
    Database_Statement stmt;

    if ( stmt.prepare( m_sqlitedb, "SELECT version,identifier,strftime('%s', lastupdated) FROM settings" ) && stmt.step() == SQLITE_ROW )
        return stmt.columnInt64( 2 );

    return 0;
}

void Database::resetLastDatabaseUpdate()
{
    execute( "UPDATE settings SET lastupdated=0" );
}

qint64 Database::getSongCount() const
{
    Database_Statement songstmt;

    if ( songstmt.prepare( m_sqlitedb, "SELECT COUNT(rowid) FROM songs" ) && songstmt.step() == SQLITE_ROW )
        return songstmt.columnInt64( 0 );

    return 0;
}

qint64 Database::getArtistCount() const
{
    Database_Statement songstmt;

    if ( songstmt.prepare( m_sqlitedb, "SELECT COUNT(DISTINCT artist) from songs" ) && songstmt.step() == SQLITE_ROW )
        return songstmt.columnInt64( 0 );

    return 0;
}

bool Database::cleanupCollections()
{
    Database_Statement stmt;

    if ( !execute( "BEGIN TRANSACTION" ) )
        return false;

    if ( !stmt.prepare( m_sqlitedb, "SELECT rowid,path FROM songs" ) )
        return false;

    while ( stmt.step() == SQLITE_ROW )
    {
        QString path = stmt.columnText( 1 );

        if ( !QFile::exists( path ) )
        {
            Logger::debug( "Collection cleanup: removing non-existing song %s", qPrintable(path) );

            if ( !execute( QString("DELETE FROM songs WHERE rowid=%1").arg( stmt.columnInt(0) ) ) )
            {
                execute( "ROLLBACK TRANSACTION" );
                return false;
            }
        }
    }

    return execute( "COMMIT TRANSACTION" );
}

void Database::getDatabaseCurrentState()
{
    pCurrentState->m_databaseSongs = getSongCount();
    pCurrentState->m_databaseArtists = getArtistCount();

    qint64 updated = lastDatabaseUpdate();

    pCurrentState->m_databaseUpdatedDateTime = updated > 0 ?
                QDateTime::fromMSecsSinceEpoch( updated ).toString( "yyyy-MM-dd hh:mm:ss")
                  : tr("never");
}


bool Database::verifyDatabaseVersion()
{
    Database_Statement stmt;

    if ( !stmt.prepare( m_sqlitedb, "SELECT version,identifier,strftime('%s', lastupdated) FROM settings" ) || stmt.step() != SQLITE_ROW )
    {
        QString identifier = QUuid::createUuid().toString().mid( 1, 36 );

        // Set the settings
        if ( !execute( QString("INSERT INTO settings VALUES( %1, ?, 0)") .arg( CURRENT_DB_SCHEMA_VERSION ), QStringList() << identifier ) )
            return false;

        Logger::debug( "Initialized new karaoke song database at %s", qPrintable(pSettings->songdbFilename) );
    }

    return true;
}

bool Database::recreateSongTable()
{
    if ( !execute( "CREATE TABLE IF NOT EXISTS songs"
        "( path TEXT PRIMARY KEY, "
           "artist TEXT, "
           "title TEXT, "
           "type TEXT, "
           "search TEXT, "
           "played INT, "
           "lastplayed INT, "
           "added INT, "
           "rating INT, "
           "language TEXT, "
           "flags INT, "
           "collectionid INT, "
           "parameters TEXT )" )
    || !execute( "CREATE INDEX IF NOT EXISTS idxSearch ON songs(search)" )
    || !execute( "CREATE INDEX IF NOT EXISTS idxPath ON songs(artist)" ) )
        return false;

    return true;
}

bool Database::execute( const QString &sql, const QStringList& args )
{
    Database_Statement stmt;

    if ( !stmt.prepare( m_sqlitedb, sql, args  ) )
    {
        pActionHandler->error( QString("Error executing database command: %1\n%2").arg( sqlite3_errmsg( m_sqlitedb ) ) .arg(sql) );
        return false;
    }

    int res;

    // Eat all the results
    while ( (res = stmt.step()) == SQLITE_ROW )
        ;

    if ( res == SQLITE_DONE )
        return true;
    else
    {
        pActionHandler->error( QString("Error executing database command: %1\n%2").arg( sqlite3_errmsg( m_sqlitedb ) ) .arg(sql) );
        return false;
    }
}


bool Database::search(const QString &substr, QList<Database_SongInfo> &results, unsigned int limit)
{
    results.clear();

    Database_Statement stmt;

    // Tokenize and process the search substring
    QStringList searchdata;
    QString query;

    Q_FOREACH( QString s, substr.split( " ", QString::SkipEmptyParts ) )
    {
        searchdata << "% " + s.toUpper() + "%";

        if ( !query.isEmpty() )
            query += " AND ' ' || search || ' ' LIKE ?";
        else
            query = "WHERE ' ' || search || ' ' LIKE ?";
    }

    // Credits for the word boundary search: http://stackoverflow.com/questions/16450568/query-sqlite-to-like-but-whole-words
    query += " ORDER BY artist,title";

    //if ( !stmt.prepareSongQuery( m_sqlitedb, "WHERE ' ' || search || ' ' LIKE ? ORDER BY artist,title", QStringList() << searchstr ) )
    if ( !stmt.prepareSongQuery( m_sqlitedb, query, searchdata ) )
        return false;

    while ( stmt.step() == SQLITE_ROW )
    {
        Database_SongInfo info = stmt.getRowSongInfo();
        results.append( info );

        if ( --limit == 0 )
            break;
    }

    return !results.empty();
}

QJsonObject Database::getSongParams(int id)
{
    Database_Statement stmt;

    if ( stmt.prepare( m_sqlitedb, QString("SELECT parameters FROM songs WHERE rowid=%1").arg(id) ) && stmt.step() == SQLITE_ROW )
    {
        QJsonDocument doc = QJsonDocument::fromJson( stmt.columnText(0).toUtf8() );

        if ( !doc.isEmpty() )
            return doc.object();
    }

    return QJsonObject();
}

bool Database::setSongParams(int id, const QJsonObject &params)
{
    return execute( QString("UPDATE songs SET parameters=? WHERE rowid=%1").arg(id), QStringList() <<  QJsonDocument( params ).toJson() );
}
