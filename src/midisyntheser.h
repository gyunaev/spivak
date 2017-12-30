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

#ifndef MIDISYNTHESER_H
#define MIDISYNTHESER_H

#include <QFile>
#include <QIODevice>

#include "libsonivox/include/eas.h"

// This class represents a playable WAV file which is rendered through the MIDI
// synthesis while being played.
class MIDISyntheser : public QIODevice
{
    public:
        MIDISyntheser( QObject * parent = 0 );

        //bool    open( const QString& midiFileName );
        bool    open( const QByteArray& midiData );

        void    close();

        // Those are inherited from QIODevice
        qint64	pos() const;
        bool	seek(qint64 pos);
        qint64	size() const;
        bool    isSequential() const;
        bool    atEnd() const;
        qint64  bytesAvailable() const;

        qint64	readData(char * data, qint64 maxSize);
        qint64	writeData(const char *, qint64 );

    private:
        // I/O callbacks
        static int EAS_FILE_readAt( void *handle, void *buf, int offset, int size );
        static int EAS_FILE_size( void *handle );

        bool    fillBuffer();
        void    fillWAVheader();

        QByteArray      m_midiData;

        EAS_FILE        m_easFileIO;
        EAS_HANDLE      m_easHandle;
        EAS_DATA_HANDLE m_easData;

        QByteArray      m_audioBuffer;
        unsigned int    m_audioAvailable;
        bool            m_audioEnded;
        unsigned int    m_mixBufferSize;

        // File emulation parameters
        unsigned int    m_currentPosition;
        unsigned int    m_totalSize;
};

#endif // MIDISYNTHESER_H
