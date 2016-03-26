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

#ifndef PLAYERLYRICTEXTLINE_H
#define PLAYERLYRICTEXTLINE_H

#include <QFont>
#include <QPainter>
#include <QFontMetrics>

#include "libkaraokelyrics/lyricsloader.h"

class KaraokePainter;

// Encapsulates a single lyric text line, together with duration, timing and pitch info
// Draws inself in certain state depending on time (not yet, current, disappearing)
class PlayerLyricTextLine
{
    public:
        // Empty constructor - paragraph break, draws nothing
        PlayerLyricTextLine();
        PlayerLyricTextLine( const LyricsLoader::Container& line );

        // When is the next time something needs to be (re)drawn assuming no FF/RW
        qint64  startTime() const;
        qint64  endTime() const;

        // Draw the lyrics according to time. Returns next time when the lyrics need
        // to be updated
        qint64  draw( KaraokePainter& p, qint64 time, int yoffset );

        // Draw the disappearing animation with specific percentage(0-100)
        void    drawDisappear(KaraokePainter& p, int percentage, int yoffset );

        // Empty set means screen space, and will not have draw/nextTime called
        bool    isEmpty() const { return m_line.empty(); }

        // Should return the longest  possible width (i.e. if the width increases when the words
        // are being played, should return the width as if the longest part is being played)
        int     screenWidth( const QFontMetrics& font );

        // Dump the text line, optionally with the time marker
        void    dump( qint64 time = -1 ) const;

        const QString& fullLine() const { return m_text; }

    private:
        LyricsLoader::Container     m_line;

        QString                     m_text;
        qint64                      m_start;
        qint64                      m_end;
        qint64                      m_duration;
};

#endif // PLAYERLYRICTEXTLINE_H
