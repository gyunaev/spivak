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

#ifndef LYRICSRENDERER_H
#define LYRICSRENDERER_H

#include <QString>
#include <QColor>
#include <QIODevice>

#include "libkaraokelyrics/lyricsloader.h"


class KaraokePainter;

//
// This class is a base class for the lyric renderer of all kind (text and graphics, in future maybe video too)
// All implemented renderers must take care of:
// - Loading the lyrics (with necessary preprocessing if needed), either from disk or from an archive;
// - Rendering the lyrics for a specific timeframe in a proper state - it should expect the new time to be far ahead or behind;
// - Being able to tell whether the lyrics need to be redrawn between the last and current timeframe
//
class PlayerLyrics
{
    public:
        PlayerLyrics();
        virtual ~PlayerLyrics();

        // Load the lyrics from the QIODevice (which could be an original file or an archive entry)
        // Returns true if load succeed, otherwise must return false and set m_errorMsg to the reason
        virtual bool    load( QIODevice * file, const QString &filename = "" ) = 0;

        // Must return next time when the lyrics would be updated. If returns -1, no more updates
        virtual qint64  nextUpdate() const = 0;

        // Render the lyrics; also sets the m_lastRenderedTime
        bool    draw( KaraokePainter& p );

        // Error message
        QString errorMsg() const { return m_errorMsg; }

        // Set/get lyric time delay
        void    setDelay( int delayms );
        int     delay() const;

        // Properties
        LyricsLoader::Properties properties() const;

    protected:
        // Render lyrics for current timestamp into the QImage provided. Must "render over",
        // and not mess up with the rest of the screen. True on success, false + m_errorMsg on error.
        virtual bool    render( KaraokePainter& p, qint64 time ) = 0;

    protected:
        // Any error should set this member
        QString         m_errorMsg;

        // Last time lyrics renderer was called (in ms)
        qint64          m_lastRenderedTime;

        // Lyric delay
        QAtomicInt      m_timeDelay;

        // Lyric properties
        LyricsLoader::Properties    m_properties;
};

#endif // LYRICSRENDERER_H
