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

#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QTextCodec>

#include "settings.h"

class Util
{
    public:
        // Finds the first index of any space character, i.e. indexOf( QRegExp("\\s")) but without regexp
        static int indexOfSpace( const QString& str );

        // Calculates and returns SHA-512
        static QString sha512File( const QString& filename );

        // Returns the path to a cached file for a specific file name, based on file content checksum
        static QString cachedFile( const QString& filename );

        // Detects the encoding of a data
        static QTextCodec * detectEncoding( const QByteArray& data );

        // Enumerates the directories recursively, and fills up a QStringList with the files matching a specific extension
        static void enumerateDirectory( const QString& rootPath, const QStringList& extensions, QStringList& files );

        // Get last file extension (i.e. for .tar.gz it will get gz). Returns "" if no extension
        static QString  fileExtension( const QString& filename );

        // We can see if we need QDir::separator later
        static QString  separator() { return "/"; }

        // Convert ticks (int64 ms) to a proper time string
        static QString tickToString( qint64 tickvalue );

    private:
        Util();
};

#endif // UTIL_H
