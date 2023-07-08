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

#ifndef KARAOKEPLAYABLE_KFN_H
#define KARAOKEPLAYABLE_KFN_H

#include <QFile>
#include <QStringList>

#include "karaokeplayable.h"

class KaraokePlayable_ZIP;

// Parser for KaraFun files (both old and new versions)
class KaraokePlayable_KFN : public KaraokePlayable
{
    public:
        KaraokePlayable_KFN(const QString& basename, QStringDecoder *filenameDecoder );

        // Initializes the object
        bool        init();

        // Should return true if the file is a compound object (i.e. ZIP), i.e. enumerate returns files IN the object, not NEXT to it
        bool        isCompound() const;

        // Returns a list of files in the archive/next to the file (same directory) EXCLUDING the base object;
        // Should return an empty array in case of error
        QStringList enumerate();

        // Should return full absolute path if the object is on file system. Otherwise an empty string; DO NOT EXTRACT here.
        // Should return the path even if it is not valid/file does not exist.
        // Basically for base "base/file/foo" and "baz" object should return "base/file/baz"; ZIP etc would always return empty string
        QString     absolutePath(const QString&);

        // Should extract the object into the out QIODevice; returns true if succeed, false on error. THis will only be called if
        // absolutePath() returned an empty string.
        bool        extract( const QString& object,  QIODevice *out );

    private:
        // Since KFN has unique lyrics format, we parse it internally, and replace
        // the song.ini file in the KFN by a fake LRC file. And later, when requested,
        // we convert internally lyrics into this format and return it.
        //
        // Fake entries are stored in this list, which is returned in enumerate() call
        QStringList m_fakeEntries;

        // Those parse methods for old and new format should throw QString in case of error
        void        parseOldKarafunFile();
        void        parseNewKarafunFile();

        // If this is the old KFN format (which is basically ZIP archive), this pointer is used
        KaraokePlayable_ZIP *   m_kfnZip;

        // If this is a new KFN file, the following stuff is used

        // Directory entry
        class Entry
        {
            public:
                enum
                {
                    TYPE_SONGTEXT = 1,
                    TYPE_MUSIC = 2,
                    TYPE_IMAGE = 3,
                    TYPE_FONT = 4,
                    TYPE_VIDEO = 5
                };

                QString filename;	// the original file name in the original encoding
                int type;			// the file type; see TYPE_
                int length_in;		// the file length in the KFN file
                int length_out;		// the file lenght on disk; if the file is encrypted it is the same or smaller than length_in
                int offset;			// the file offset in the KFN file starting from the directory end
                int flags;			// the file flags; 0 means "not encrypted", 1 means "encrypted"
        };


        // KFN parser routines
        quint8          readByte();
        quint16         readWord();
        quint32         readDword();
        void            readBytes( char * buf, unsigned int len );
        QByteArray      readBytes( unsigned int len );
        QString         readString( unsigned int len );
        bool            extractNew( int entryidx, QIODevice *out );

        void            parseSongIni( const QByteArray& songini );

    private:
        // Pointer to the KFN file
        QFile			m_file;

        // If the file is encrypted, contains its encryption key
        char            m_aesKey[16];

        // Parsed entries in the file
        QList<Entry>	m_entries;
};


#endif // KARAOKEPLAYABLE_KFN_H
