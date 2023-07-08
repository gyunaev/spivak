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

#include "lyricsparser_kok.h"

LyricsParser_KOK::LyricsParser_KOK(LyricsLoaderCallback *callback )
    : LyricsParser( callback )
{
}

void LyricsParser_KOK::parse(QIODevice *file, LyricsLoader::Container &output, LyricsLoader::Properties &properties)
{
    QByteArray data = load( file );

    // This format has no special markers, so we can feed it as-is - just replace the marks (this is why we copied it)
    QByteArray encdetectdata = data;

    auto codec = std::make_unique<QStringDecoder*>( detectEncoding( encdetectdata.replace( ';', ' ' ), properties ) );

    // J'ai ;7,9050634;tra;8,0651993;vail;8,2144914;lé;8,3789922;
    Q_FOREACH ( QString line, QString( (*codec)->decode( data ) ).split( '\n' ) )
    {
        if ( line.isEmpty() )
            continue;

        QStringList entries = line.split( ";" );

        // Must be even
        if ( (entries.size() % 2) != 0 )
            throw("Invalid number of entries");

        for ( int i = 0; i < entries.size() / 2; i++ )
        {
            QString text = entries[2 * i];
            QString timing = entries[2 * i + 1].replace( ",", "." );

            int timevalue = (int) ( timing.toFloat() * 1000 );
            if ( i == entries.size() / 2 - 1 )
                text.append( '\n' );

            output.push_back( Lyric( timevalue, text ) );
        }
    }
}

