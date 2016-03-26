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

#include <QJsonDocument>
#include <QJsonObject>

#include "sqlite3.h"

#include "settings.h"
#include "database_statement.h"


Database_Statement::Database_Statement()
{
    stmt = 0;
}

Database_Statement::~Database_Statement()
{
    if ( stmt )
        sqlite3_finalize( stmt );
}

bool Database_Statement::prepare(sqlite3 *db, const QString &sql, const QStringList &args)
{
    // Prepare the sql
    if ( sqlite3_prepare_v2( db, qPrintable(sql), -1, &stmt, 0 ) != SQLITE_OK )
        return false;

    // Bind values if there are any
    for ( int i = 0; i < args.size(); i++ )
    {
        m_args.push_back( args[i].toUtf8() );
        if ( sqlite3_bind_text( stmt, i + 1, m_args.last().data(), -1, 0 ) != SQLITE_OK )
            return false;
    }

    return true;
}

qint64 Database_Statement::columnInt64(int column)
{
    return sqlite3_column_int64( stmt, column );
}

int Database_Statement::columnInt(int column)
{
    return sqlite3_column_int( stmt, column );
}

QString Database_Statement::columnText(int column)
{
    return QString::fromUtf8( (const char*) sqlite3_column_text( stmt, column ), sqlite3_column_bytes( stmt, column ) );
}

int Database_Statement::step()
{
    return sqlite3_step( stmt );
}

bool Database_Statement::prepareSongQuery(sqlite3 *db, const QString &wheresql, const QStringList &args)
{
    QString query = "SELECT rowid, path, artist, title, type, played, strftime('%s', lastplayed), strftime('%s', added), rating, language, collectionid, flags, parameters FROM songs ";
    return prepare( db, query + wheresql, args );
}

Database_SongInfo Database_Statement::getRowSongInfo()
{
    Database_SongInfo info;

    info.id = columnInt64( 0 );
    info.filePath = pSettings->replacePath( columnText( 1 ) );
    info.artist = columnText( 2 );
    info.title = columnText( 3 );
    info.type = columnText( 4 );
    info.playedTimes = columnInt( 5 );
    info.lastPlayed = columnInt64( 6 );
    info.added = columnInt64( 7 );
    info.rating = columnInt( 8 );
    info.language = columnText( 9 );
    info.collectionid = columnInt( 10 );
    info.flags = columnInt( 11 );

    // Reset possible song parameters
    info.lyricDelay = 0;

    // Parse song parameters
    QJsonDocument doc = QJsonDocument::fromJson( columnText(12).toUtf8() );

    if ( !doc.isEmpty() )
    {
        QJsonObject obj = doc.object();

        if ( obj.contains( "delay" ) )
            info.lyricDelay = obj["delay"].toInt();

        if ( obj.contains( "password" ) )
            info.password = obj["password"].toString();
    }

    return info;
}


