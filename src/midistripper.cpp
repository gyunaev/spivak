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

#include <QApplication>
#include <QFile>

#include "logger.h"
#include "midistripper.h"


QByteArray MIDIStripper::stripMIDI(const QString &filename)
{
    try
    {
        MIDIStripper m;
        m.process( filename );

        return m.m_outMidi;
    }
    catch( const char * err )
    {
        Logger::error( "MIDIStripper: error parsing MIDI file %s: %s", qPrintable(filename), err);
        return QByteArray();
    }
}

MIDIStripper::MIDIStripper()
{
}

unsigned char MIDIStripper::readByte()
{
    unsigned char byte;

    if ( m_input.read( (char*) &byte, 1 ) != 1 )
        throw( "Cannot read byte: premature end of file" );

    m_chunk.append( byte );
    return byte;
}

unsigned short MIDIStripper::readWord()
{
    unsigned char byte1 = readByte();
    unsigned char byte2 = readByte();

    return byte1 << 8 | byte2;
}


unsigned int MIDIStripper::readDword()
{
    unsigned char byte1 = readByte();
    unsigned char byte2 = readByte();
    unsigned char byte3 = readByte();
    unsigned char byte4 = readByte();

    return byte1 << 24 | byte2 << 16 | byte3 << 8 | byte4;

}

int MIDIStripper::readVarLen()
{
    int l = 0, c;

    c = readByte();

    if ( !(c & 0x80) )
        return l | c;

    l = (l | (c & 0x7f)) << 7;
    c = readByte();

    if ( !(c & 0x80) )
        return l | c;

    l = (l | (c & 0x7f)) << 7;
    c = readByte();

    if ( !(c & 0x80) )
        return l | c;

    l = (l | (c & 0x7f)) << 7;
    c = readByte();

    if ( !(c & 0x80) )
        return l | c;

    l = (l | (c & 0x7f)) << 7;
    c = readByte();

    if ( !(c & 0x80) )
        return l | c;

    throw( "Cannot read variable field" );
}

void MIDIStripper::consumeData(unsigned int length)
{
    while ( length > 0 )
    {
        readByte();
        length--;
    }
}

void MIDIStripper::storeBEdword(char *addr, quint32 value)
{
    unsigned char * p = (unsigned char *) addr;

    p[0] = (value >> 24) & 0xFF;
    p[1] = (value >> 16) & 0xFF;
    p[2] = (value >> 8) & 0xFF;
    p[3] = value & 0xFF;
}


void MIDIStripper::process(const QString &filename)
{
    m_input.setFileName( filename );

    if ( !m_input.open( QIODevice::ReadOnly ) )
        throw( "Can't open file for reading");

    // Parse MIDI file: https://www.csie.ntu.edu.tw/~r92092/ref/midi/

    // Bytes 0-4: header
    unsigned int header = readDword();

    // If we get MS RIFF header, skip it - some synthesers don't accept such MIDI files
    if ( header == 0x52494646 )
    {
        m_input.seek( 20 );

        // Get rid of the RIFF header
        m_chunk.clear();

        header = readDword();
    }

    // MIDI header
    if ( header != 0x4D546864 )
        throw( "Not a MIDI file" );

    // Bytes 5-8: header length
    unsigned int header_length = readDword();

    // Bytes 9-10: format
    unsigned short format = readWord();

    if ( format > 2 )
        throw( "Unsupported format" );

    // Bytes 11-12: tracks
    unsigned short tracks = readWord();

    // Bytes 13-14: divisious
    unsigned short divisions = readWord();

    if ( divisions > 32768 )
        throw( "Unsupported division" );

    // Number of tracks is always 1 if format is 0
    if ( format == 0 )
        tracks = 1;

    // "Nevertheless, any MIDI file reader should be able to cope with larger header-chunks, to allow for future expansion."
    // However not all MIDI readers do so. Hence we 'fix' the header in this case
    if ( header_length > 6 )
    {
        storeBEdword( m_chunk.data() + 4, 6 );

        m_outMidi = m_chunk;

        // Consume the rest, and discard them
        consumeData( header_length - 6 );
    }
    else
        m_outMidi = m_chunk;

    // Parse all tracks
    for ( int track = 0; track < tracks; track++ )
    {
        m_track.clear();
        int laststatus = 0;
        int lastchannel = 0;

        // Skip malformed files
        if ( readDword() != 0x4D54726B )
            throw( "Malformed track header" );

        // Next track position
        int tracklen = readDword();
        unsigned int nexttrackstart = tracklen + m_input.pos();

        // Parse track until end of track event
        while ( m_input.pos() < nexttrackstart )
        {
            bool skipChunk = false;
            m_chunk.clear();

            // field length
            int clocks = readVarLen();
            Q_UNUSED( clocks );
            unsigned char msgtype = readByte();

            //
            // Meta event
            //
            if ( msgtype == 0xFF )
            {
                // META type
                readByte();

                unsigned int metalength = readVarLen();

                // Consume this data - we keep meta events
                consumeData( metalength );
            }
            else if ( msgtype== 0xF0 || msgtype == 0xF7 )
            {
                // SysEx event
                unsigned int length = readVarLen();
                consumeData( length );

                // We ignore those events
                skipChunk = true;
            }
            else
            {
                // Regular MIDI event
                if ( msgtype & 0x80 )
                {
                    // Status byte
                    laststatus = ( msgtype >> 4) & 0x07;
                    lastchannel = msgtype & 0x0F;

                    if ( laststatus != 0x07 )
                        msgtype = readByte() & 0x7F;
                }

                switch ( laststatus )
                {
                case 0:  // Note off
                    readByte();
                    break;

                case 1: // Note on
                    readByte();
                    break;

                case 2: // Key Pressure
                case 3: // Control change
                case 6: // Pitch wheel
                    readByte();
                    break;

                case 4: // Program change
                case 5: // Channel pressure
                    break;

                default:
                    if ( (lastchannel & 0x0F) == 2 ) // Sys Com Song Position Pntr
                        readWord();
                    else if ( (lastchannel & 0x0F) == 3 ) // Sys Com Song Select(Song #)
                        readByte();
                    break;
                }
            }

            // This might not work well if Sysex has non-zero clocks, but we will see if it ever happens
            if ( !skipChunk )
                m_track += m_chunk;
        }

        // Track ended, and we can now write it down
        int trackoffset = m_outMidi.size();
        m_outMidi.resize( trackoffset + 8 );
        m_outMidi.append( m_track );

        // Track header and length
        storeBEdword( m_outMidi.data() + trackoffset, 0x4D54726B );
        storeBEdword( m_outMidi.data() + trackoffset + 4, m_track.size() );
    }
}
