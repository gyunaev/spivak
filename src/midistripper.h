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

#ifndef MIDISTRIPPER_H
#define MIDISTRIPPER_H

#include <QFile>

// This class strips the MIDI file from the things which commonly confuse or outright corrupt
// software MIDI synthesers:
// 1. SysEx events
// 2. RIFF header on a MIDI file
class MIDIStripper
{
    public:
        static QByteArray  stripMIDI( const QString& filename );

    private:
        MIDIStripper();
        void            process( const QString& filename );

        // Byteorder-safe readers which also remember the retrieved data and throw exceptions
        quint8          readByte();
        quint16         readWord();
        quint32         readDword();
        int 			readVarLen();

        // This consumes the data, 'reading' it (and storing it in a chunk)
        void			consumeData( unsigned int length );

        // Dword storage
        void            storeBEdword( char * addr, quint32 value );

        // Input stream for read... ops above
        QFile           m_input;

        // Output MIDI stream
        QByteArray      m_outMidi;

        // Keeps all the data read in current chunk
        QByteArray      m_chunk;

        // Keeps the current track data (for ease and lenght calculation)
        QByteArray      m_track;
};

#endif // MIDISTRIPPER_H
