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

#ifndef LYRICSPARSER_ULTRASTAR_H
#define LYRICSPARSER_ULTRASTAR_H

#include <QStringList>
#include "lyricsparser.h"

// This class parses both Ultrastar and PowerKaraoke lyrics
class LyricsParser_Texts : public LyricsParser
{
    public:
        LyricsParser_Texts( LyricsLoaderCallback *callback );

        // Parses the lyrics, filling up the output container. Throws an error
        // if there are any issues during parsing, otherwise fills up output.
        void parse( QIODevice * file, LyricsLoader::Container& output, LyricsLoader::Properties& properties );

        static bool isUltrastarLyrics( QIODevice * file, QString * mp3file );

    private:
        void parseUStar( const QByteArray& text, LyricsLoader::Container& output, LyricsLoader::Properties& properties );
        void parsePowerKaraoke( const QByteArray& text, LyricsLoader::Container& output, LyricsLoader::Properties& properties );
};

#endif // LYRICSPARSER_ULTRASTAR_H
