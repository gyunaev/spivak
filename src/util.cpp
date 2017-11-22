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

#include <QDir>
#include <QFile>
#include <QTime>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QByteArray>

#include "util.h"
#include "logger.h"

#include "uchardet/uchardet.h"


QTextCodec * Util::detectEncoding( const QByteArray &data )
{
    QTextCodec * codec = 0;

    uchardet_t ucd = uchardet_new();

    if ( uchardet_handle_data( ucd, data.constData(), data.size() ) == 0 )
    {
        // Notify an end of data to an encoding detctor.
        uchardet_data_end( ucd );

        const char * encoding = uchardet_get_charset( ucd );

        if ( encoding[0] != '\0' )
        {
            // In vast majority of cases this encoding is detected incorrectly in place of CP1251
            if ( !strcmp( encoding, "x-mac-cyrillic") )
                encoding = "windows-1251";

            codec = QTextCodec::codecForName( encoding );
        }
    }

    uchardet_delete( ucd );
    return codec;
}

QString Util::fileExtension(const QString &filename)
{
    int p = filename.lastIndexOf( '.' );

    if ( p == -1 )
        return "";

    return filename.mid( p + 1 );
}

QString Util::tickToString(qint64 tickvalue)
{
    int minute = tickvalue / 60000;
    int second = (tickvalue-minute*60000)/1000;
    int msecond = (tickvalue-minute*60000-second*1000) / 100; // round milliseconds to 0-9 range}

    QTime ct( 0, minute, second, msecond );
    return ct.toString( "mm:ss" );
}

void Util::enumerateDirectory(const QString &rootPaths, const QStringList &extensions, QStringList &files)
{
    files.clear();

    // We do not use recursion, and use the queue-like list instead
    QStringList paths;
    paths << rootPaths;

    // And enumerate all the paths
    while ( !paths.isEmpty() )
    {
        QString current = paths.takeFirst();
        QFileInfo finfo( current );

        // Skip non-existing paths (might come from settings)
        if ( !finfo.exists() )
            continue;

        if ( finfo.isFile() )
        {
            files.push_back( finfo.absoluteFilePath() );
            continue;
        }

        // We assume the situation where we have several lyrics for a single music is more prevalent
        Q_FOREACH( QFileInfo fi, QDir( current ).entryInfoList( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot ) )
        {
            // Re-add directories into the list
            if ( fi.isDir() )
            {
                paths.push_back( fi.absoluteFilePath() );
                continue;
            }

            // No more directories in this dir, but we have files
            if ( !extensions.contains( fi.suffix(), Qt::CaseInsensitive ) )
                continue;

            files.push_back( fi.absoluteFilePath() );
        }
    }
}

/*
QString Util::convertEncoding(const QByteArray &data)
{
    QTextCodec * codec = detectEncoding( data );

    if ( !codec )
    {
        qDebug("Util: failed to detect the data encoding");
        return "";
    }

    qDebug("Util: automatically detected encoding %s", codec->name().constData() );
    return codec->toUnicode( data );
}
*/
int Util::indexOfSpace(const QString &str)
{
    for ( int i = 0; i < str.length(); i++ )
        if ( str[i].isSpace() )
            return i;

    return -1;
}

QString Util::sha512File(const QString &filename)
{
    QCryptographicHash hash( QCryptographicHash::Sha3_512 );
    QFile file( filename );

    if ( !file.open( QIODevice::ReadOnly ) )
        return QString();

    hash.addData( &file );
    return hash.result().toHex();
}

QString Util::cachedFile(const QString &filename)
{
    // Cached file could be either file next to music file with .wav extension or a file in cache directory
    QFileInfo finfo( filename );

    QString test = finfo.absolutePath() + Util::separator() + finfo.completeBaseName() + ".wav";

    if ( QFile::exists( test ) )
        return test;

    // Try the hashed version
    QString hash = sha512File( filename );

    if ( hash.isEmpty() )
        return QString();

    return pSettings->cacheDir + Util::separator() + hash + ".wav";
}
