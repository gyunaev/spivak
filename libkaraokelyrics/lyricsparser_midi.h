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

#ifndef LYRICSPARSER_MIDI_H
#define LYRICSPARSER_MIDI_H

#include "lyricsparser.h"

class LyricsParser_MIDI : public LyricsParser
{
    public:
        LyricsParser_MIDI( LyricsLoaderCallback *callback );

        // Parses the lyrics, filling up the output container. Throws an error
        // if there are any issues during parsing, otherwise fills up output.
        void parse( QIODevice * file, LyricsLoader::Container& output, LyricsLoader::Properties& properties );

    private:
        // Byteorder-safe readers
        unsigned char 	readByte();
        unsigned short	readWord();
        unsigned int	readDword();
        int 			readVarLen();
        void			readData( void * buf, unsigned int length );

        unsigned int 	currentPos() const;
        void			setPos( unsigned int offset );

        QByteArray      m_mididata;
        int             m_currentOffset;
};

#endif // LYRICSPARSER_MIDI_H
