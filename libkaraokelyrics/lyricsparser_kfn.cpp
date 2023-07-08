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

#include <stdio.h>

#include "lyricsparser_kfn.h"

LyricsParser_KFN::LyricsParser_KFN(LyricsLoaderCallback *callback )
    : LyricsParser( callback )
{
}

void LyricsParser_KFN::parse(QIODevice * file, LyricsLoader::Container& output, LyricsLoader::Properties &properties)
{
    // Parse the song.ini and fill up the sync and text arrays
    QByteArrayList texts;
    QList< int > syncs;

    // Analyze each line
    int idx_text = 0, idx_sync = 0;

    QByteArrayList lines = load( file ).split( '\n' );
    bool inGeneral = false;

    for ( int i = 0; i < lines.size(); i++ )
    {
        QByteArray line = lines[i];

        // Section detector
        if ( line.startsWith( '[' ) && line.endsWith( ']') )
        {
            inGeneral = (line == "[General]");
            continue;
        }

        if ( inGeneral )
        {
            if ( line.startsWith( "Title=" ) )
            {
                QString v = QString::fromUtf8( line.mid(6) ).trimmed();

                if ( !v.isEmpty() )
                    properties[ LyricsLoader::PROP_TITLE ] = v;
            }

            if ( line.startsWith( "Artist=" ) )
            {
                QString v = QString::fromUtf8( line.mid(7) ).trimmed();

                if ( !v.isEmpty() )
                    properties[ LyricsLoader::PROP_ARTIST ] = v;
            }
        }

        // Try to match the sync first
        char matchbuf[128];
        sprintf( matchbuf, "Sync%d=", idx_sync );

        if ( line.startsWith( matchbuf ) )
        {
            idx_sync++;

            // Syncs are split by comma
            QByteArrayList values = line.mid( strlen(matchbuf) ).split( ',' );

            for ( int v = 0; v < values.size(); v++ )
                syncs.push_back( values[v].toInt() );
        }

        // Now the text
        sprintf( matchbuf, "Text%d=", idx_text );

        if ( line.startsWith( matchbuf ) )
        {
            idx_text++;
            QByteArray textvalue = line.mid( strlen(matchbuf) );

            if ( !textvalue.isEmpty() )
            {
                // Text is split by word and optionally by the slash
                QByteArrayList values = textvalue.split(' ');

                for ( int v = 0; v < values.size(); v++ )
                {
                    QByteArrayList morevalues = values[v].split( '/' );

                    for ( int vv = 0; vv < morevalues.size(); vv++ )
                        texts.push_back( morevalues[vv] );

                    // We split by space, so add it at the end of each word
                    texts.last() = texts.last() + " ";
                }
            }

            // Line matched, so make it a line
            if ( texts.size() > 2 && texts[ texts.size() - 2 ] != "\n" )
                texts.push_back( "\n" );
        }
    }

    int curr_sync = 0;
    bool has_linefeed = false;
    int lines_no_block = 0;
    int lastsync = -1;

    // The original timing marks are not necessarily sorted, so we add them into a map
    // and then output them from that map
    QMap< int, QByteArray > sortedLyrics;

    for ( int i = 0; i < texts.size(); i++ )
    {
        if ( texts[i] == "\n" )
        {
            if ( lastsync == -1 )
                continue;

            if ( has_linefeed )
                lines_no_block = 0;
            else if ( ++lines_no_block > 6 )
            {
                lines_no_block = 0;
                sortedLyrics[ lastsync ] += "\n";
            }

            has_linefeed = true;
            sortedLyrics[ lastsync ] += "\n";
            continue;
        }
        else
            has_linefeed = false;

        // Get the time if we have it
        if ( curr_sync >= syncs.size() )
            continue;

        lastsync = syncs[ curr_sync++ ];
        sortedLyrics.insert( lastsync, texts[i] );
    }

    // Generate the content for encoding detection
    QByteArray lyricsForEncoding;

    for ( QMap< int, QByteArray >::const_iterator it = sortedLyrics.begin(); it != sortedLyrics.end(); ++it )
        lyricsForEncoding.append( it.value() );

    // Detect the encoding
    auto codec = std::make_unique<QStringDecoder*>( detectEncoding( lyricsForEncoding, properties ) );

    for ( QMap< int, QByteArray >::const_iterator it = sortedLyrics.begin(); it != sortedLyrics.end(); ++it )
    {
        qint64 timing = it.key() * 10;
        Lyric lyr( timing, (*codec)->decode( it.value() ) );

        // Last line?
        if ( lyr.text.endsWith( '\n') )
        {
            // Remove the \n
            lyr.text.chop( 1 );
            output.push_back( lyr );

            // and add an empty lyric
            output.push_back( Lyric( timing ) );
        }
        else
        {
            output.push_back( lyr );
        }
    }
}
