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

#ifndef SONGDATABASE_H
#define SONGDATABASE_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QJsonObject>
#include <QStringList>

#include "songdatabasescanner.h"
#include "database_songinfo.h"


struct sqlite3;


// Initial design used SQLite, but since even the largest collections were below 2Mb in size,
// this made no sense. Only when we move to Android this might switch back to SQLite.
class Database : public QObject
{
    Q_OBJECT

    public:
        Database( QObject * parent );
        ~Database();

        // Initializes a new (empty) database, or loads an existing database
        bool    init();

        // Search for a substring in artists and titles
        bool    search( const QString& substr, QList<Database_SongInfo>& results, unsigned int limit = 1000 );

        // Queries the song by ID
        bool    songById( int id, Database_SongInfo& info );

        // Queries the song by path
        bool    songByPath( const QString& path, Database_SongInfo& info );

        // Updates the song playing stats and delay
        void    updatePlayedSong( int id, int newdelay, int newrating );

        // For browsing the database
        bool    browseInitials( QList<QChar> &artistInitials );
        bool    browseArtists( const QChar& artistInitial, QStringList& artists );
        bool    browseSongs( const QString& artist, QList<Database_SongInfo>& results );

        // Add/update database entries from the list in a single transaction
        bool    updateDatabase( const QList<SongDatabaseScanner::SongDatabaseEntry> entries );
        bool    updateLastScan();

        // Empty the database
        bool    clearDatabase();

        // Reset the last update timestamp
        void    resetLastDatabaseUpdate();

        // Goes through all collections and removes the songs which are missing from disk. Takes a while, run in a separate thread!
        bool    cleanupCollections();

        // Gets the database information to current state
        void    getDatabaseCurrentState();

        // Get last update timestamp
        qint64  lastDatabaseUpdate() const;

    private:
        // Returns the song or artist count
        qint64  getSongCount() const;
        qint64  getArtistCount() const;

        bool    verifyDatabaseVersion();
        bool    recreateSongTable();
        bool    execute( const QString& sql, const QStringList& args = QStringList() );

        // Get and set various song parameters which are rare to be present (password, delay etc)
        QJsonObject getSongParams( int id );
        bool    setSongParams( int id, const QJsonObject& params );

    private:
        // Database handle
        sqlite3 *       m_sqlitedb;
};

extern Database * pDatabase;

#endif // SONGDATABASE_H
