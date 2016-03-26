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

#ifndef LYRICSPARSER_H
#define LYRICSPARSER_H

#include <QIODevice>
#include <QTextCodec>
#include <QByteArray>
#include <QStringList>

#include "lyricsloader.h"


// Base for the loader of lyrics of various formats.
// Mostly used to offer shared functions which are used among -
// those objects are transitional, and only created when lyrics
// of a specific format are parsed.
class LyricsParser
{
    public:
        enum Type
        {
            TYPE_UNKNOWN,
            TYPE_MID,
            TYPE_LRC,
            TYPE_TXT,   // may be Ultrastar or Power Karaoke
            TYPE_KFN,
            TYPE_KOK
        };

        LyricsParser( LyricsLoaderCallback *callback );
        virtual ~LyricsParser();

        // Timing to text converter
        static QString timeAsText( quint64 timing );

        // Parses the lyrics, filling up the output container. Throws an error
        // if there are any issues during parsing, otherwise fills up output.
        virtual void parse( QIODevice * file, LyricsLoader::Container& output, LyricsLoader::Properties& properties ) = 0;

    protected:
        // This does not handle encoding
        QByteArrayList  loadAndSplit( QIODevice * file );
        QByteArray      load( QIODevice * file );

        // Returns the lyric type by the file extension
        static Type getTypeByExtension( const QString& extension );

        // Called from lyric detectors
        QTextCodec  *   detectEncoding( const QByteArray& data, LyricsLoader::Properties &properties );

        // Pasword query
        QString         askLyricsPassword();

    private:
        // This one would be handling encoding
        LyricsLoaderCallback *  m_callback;
};

#endif // LYRICSPARSER_H
