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

#ifndef KARAOKEOBJECT_H
#define KARAOKEOBJECT_H

#include <QString>
#include <QIODevice>

// Base class encapsulating various Karaoke song objects such as:
//
// - Regular on-disk files
// - Archives (such as ZIP) - not just MP3+G but also Ultrastar etc
// - Complex formats such as KFN
//
class KaraokePlayable
{
    public:
        virtual ~KaraokePlayable();

        // baseFile is a file which was enqueued. It may be music file (we need to find lyrics), lyrics file (we need to find music)
        // or a compound file (we need to find both)
        static KaraokePlayable * create( const QString& baseFile, QTextCodec * filenameDecoder = 0 );

        static bool analyze( const QString& file );

        // Finds matching music and lyrics. Returns true if both are found, false otherwise
        bool        parse();

        // Music and lyric object names; must both be set (maybe to the same value) if parse() returned true
        QString     musicObject() const;
        QString     lyricObject() const;

        // If this is not empty, this means a song background image is present too
        QString     backgroundImageObject() const;

        // Opens the object as QIODevice, which the caller must delete after it is not used anymore.
        QIODevice * openObject( const QString& object );

        // Must be reimplemented in subclass.
        // Should return true if the file is a compound object (i.e. ZIP), i.e. enumerate returns files IN the object, not NEXT to it
        virtual bool        isCompound() const = 0;

        // Must be reimplemented in subclass.
        // Should return full absolute path if the object is on file system. Otherwise an empty string; DO NOT EXTRACT here.
        // Should return the path even if it is not valid/file does not exist.
        // Basically for base "base/file/foo" and "baz" object should return "base/file/baz"
        virtual QString     absolutePath( const QString& object ) = 0;

        // Must be reimplemented in subclass.
        // Returns a list of files in the archive/next to the file (same directory); empty array in case of error
        virtual QStringList enumerate() = 0;

        // Must be reimplemented in subclass.
        // Should extract the object into the out QIODevice; returns true if succeed, false on error. THis will only be called if
        // absolutePath() returned an empty string.
        virtual bool        extract( const QString& object,  QIODevice *out ) = 0;

        // Karaoke file type support
        static bool isSupportedMusicFile( const QString& filename );
        static bool isSupportedLyricFile( const QString& filename );
        static bool isMidiFile( const QString& filename );
        static bool isVideoFile( const QString& filename );
        static bool isImageFile( const QString& filename );

        // This returns true if the file is karaoke and can be played without a marching file (i.e. not a music+CDG combination)
        static bool isSupportedCompleteFile(const QString &filename);

    protected:
        // Converts filename from archive to the Unicode string
        QString decodeFilename( const QByteArray& filename );

        KaraokePlayable( const QString& baseFile, QTextCodec *filenameDecoder );

        // Must be reimplemented in subclass.
        // Initializes the object
        virtual bool  init() = 0;

        // Original file
        QString     m_baseFile;

        // Objects
        QString     m_musicObject;
        QString     m_lyricObject;
        QString     m_backgroundImageObject;
        QString     m_backgroundVideoObject;

        QString     m_errorMsg;

        // Text decoder for filenames
        QTextCodec* m_filenameDecoder;
};

#endif // KARAOKEOBJECT_H
