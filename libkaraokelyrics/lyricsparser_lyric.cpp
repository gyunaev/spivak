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

#include "lyricsparser_lyric.h"

LyricsParser_Lyric::LyricsParser_Lyric(LyricsLoaderCallback *callback )
    : LyricsParser(callback)
{

}

void LyricsParser_Lyric::parse(QIODevice *file, LyricsLoader::Container &output, LyricsLoader::Properties &properties)
{
    QByteArray lyric = file->readAll();

    if ( lyric.isEmpty() )
        throw ("Can't read the file");

    // Decrypting the content first - the encryption is incredibly weak
    // See http://www.ulduzsoft.com/2016/01/reverse-engineering-lyric-file-format-handling-encryption-with-a-known-plaintext-attack/
    const unsigned char key[] = { 0xC7, 0xE0, 0xEF, 0xF3, 0xF1, 0xF2, 0xE8, 0x20, 0xEA, 0xE0, 0xF0, 0xE0, 0xEE, 0xEA, 0xE5,
                                  0x20, 0xED, 0xE0, 0x20, 0xEA, 0xEE, 0xEC, 0xEF, 0xFC, 0xFE, 0xF2, 0xE5, 0xF0, 0xE5, 0x21  };

    // Yes, that's the whole decryption
    for ( int i = 0; i < lyric.size(); i++ )
        lyric[i] = lyric[i] ^ key[i % sizeof(key) ];

    // Now parse the format into the map, and assemble the string for the encoding detector
    QMap< unsigned int, QByteArray > lyrics;
    QByteArray lyricsForEncoding;

    for ( int i = 0; i < lyric.size(); )
    {
        if ( i + 6 > lyric.size() )
            throw ("Lyric file too short (incomplete?)");

        // signed/unsigned
        const unsigned char * ulyric = (const unsigned char *) lyric.constData();

        // First 4 bytes is timing in big-endian format
        unsigned int timing = (ulyric[i] << 24) | (ulyric[i+1] << 16) | (ulyric[i+2] << 8) | (ulyric[i+3]);

        // Followed by the 2-byte string len
        unsigned int len = (ulyric[i+4]<<8) | (ulyric[i+5]);

        // Do we have enough room for the string?
        if ( len > 0 && (int) (i + 6 + len) >= lyric.size() )
            throw ("Lyric file too short (incomplete?)");

        // We have everything
        if ( len == 0 )
        {
            lyrics.insert( timing, "\n" );
            lyricsForEncoding.append( "\n" );
        }
        else
        {
            lyrics.insert( timing, lyric.mid( i+6, len ) );
            lyricsForEncoding.append( lyric.mid( i+6, len ) );
        }

        // Move to the next record
        i = i + 6 + len;
    }

    // Detect the encoding
    QTextCodec * codec = detectEncoding( lyricsForEncoding, properties );

    // And now we can fill up the lyrics container
    for ( QMap< unsigned int, QByteArray >::iterator it = lyrics.begin(); it != lyrics.end(); ++it )
    {
        qint64 timing = it.key() * 20;

        // If this is an empty lyric, modify the previous lyric's duration if we have it
        if ( it.value().isEmpty() )
        {
            if ( !output.isEmpty() )
                output.last().duration = timing - output.last().start;

            continue;
        }

        QString text = codec->toUnicode( it.value() );

        if ( text.startsWith('/') )
        {
            // Remove this character
            text.remove( 0, 1 );

            // Add the empty lyric at the end if we already have something
            if ( !output.isEmpty() )
                output.append( Lyric(timing ) );
        }

        output.append( Lyric( timing, text ) );
    }
}
