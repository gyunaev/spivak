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

#ifndef LYRICSLOADER_H
#define LYRICSLOADER_H

#include <QStringDecoder>
#include <QIODevice>
#include <QString>
#include <QMap>
#include <QList>


// Contains a single lyric entry as present in the lyrics file
class Lyric
{
    public:
        Lyric( qint64 start_, const QString& text_ = "", qint64 duration_ = -1, int pitch_ = -1 );

    public:
        // those are OR flags, ORed the original pitch
        enum
        {
            PITCH_FREESTYLE = 0x80000000,
            PITCH_GOLDEN = 0x40000000,
        };

        qint64	start;      // Only if not end of block
        qint64	duration;   // If not set by the parser, calculated later
        QString	text;		// May be empty, this indicates end of block - in this case start/duration is not valid
        int		pitch;		// PITCH_FREESTYLE if not set, used for Ultrastar
};



// This class defines some "callback" functions which are called when necessary
class LyricsLoaderCallback
{
    public:
        LyricsLoaderCallback() { }
        virtual ~LyricsLoaderCallback() { }

        // This function must detect the used text codec for the data, and return it, or return 0 (we will fall back to UTF-8)
        virtual QStringDecoder * detectTextCodec( const QByteArray& ) { return 0; }

        // This function is called when the lyric file is password-protected, the client must return the non-empty password
        // or return an empty string if no password is provided
        virtual QString     getPassword() { return ""; }
};


// Public API for karaoke lyrics loader
class LyricsLoader
{
    public:
        enum Property
        {
            PROP_INVALID,
            PROP_TITLE,
            PROP_ARTIST,
            PROP_MUSICFILE,
            PROP_BACKGROUND,
            PROP_VIDEO,
            PROP_VIDEO_STARTOFFSET,
            PROP_DETECTED_ENCODING,
            PROP_LYRIC_SOURCE,  // from [sr: ] lrc tag, contains the lyric source (web, forum, self-made etc)
        };

        // Lyrics parameters
        typedef QMap< Property, QString > Properties;
        typedef QList<Lyric>   Container;

        LyricsLoader( Properties& properties, Container& container );

        // Parses the lyrics, filling up the output container. Returns true
        // on success, false if there is an rror, see errorMsg() contains reason.
        //
        // It gets the lyrics file name and QIODevice because the file may be extracted (from ZIP or KFN)
        bool parse(const QString& lyricsfile, QIODevice * file, LyricsLoaderCallback * callback = 0 );

        static bool    isSupportedFile( const QString& filename );

        static void dump( const Container& output );

        // In case of error this will return the reason
        QString errorMsg() const { return m_errorMsg; }

    protected:
        Container&   m_lyricsOutput;
        Properties&  m_props;

        QString     m_errorMsg;      
};

#endif // LYRICSLOADER_H
