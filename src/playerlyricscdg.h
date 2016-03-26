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

#ifndef PLAYERLYRICSCDG_H
#define PLAYERLYRICSCDG_H

#include <QImage>
#include <QVector>

#include "cdg.h"
#include "playerlyrics.h"

class KaraokePainter;

//
// This class implements loader and player for CD+G lyrics format
// and basically is copy-pasted from (mine) Karaoke Lyrics Editor
//
class PlayerLyricsCDG : public PlayerLyrics
{
    public:
        PlayerLyricsCDG();

        // Load the lyrics from the QIODevice (which could be an original file or an archive entry)
        // Returns true if load succeed, otherwise must return false and set m_errorMsg to the reason
        bool    load( QIODevice * file, const QString &filename = "" );

        // Must return true if the lyrics must be updated/redrawn for the specified time
        qint64  nextUpdate() const;

    private:
        // Render lyrics for current timestamp into the QImage provided. Must "render over",
        // and not mess up with the rest of the screen. True on success, false + m_errorMsg on error.
        virtual bool    render(KaraokePainter& p, qint64 time);

    private:
        typedef struct
        {
            qint64      timing;
            SubCode     subcode;
        } CDGPacket;

        void	dumpPacket( CDGPacket * packet );
        quint8	getPixel( int x, int y );
        void	setPixel( int x, int y, quint8 color );

        // CD+G commands
        void	cmdMemoryPreset( const char * data );
        void	cmdBorderPreset( const char * data );
        void	cmdLoadColorTable( const char * data, int index );
        void	cmdTileBlock( const char * data );
        void	cmdTileBlockXor( const char * data );
        void	cmdTransparentColor( const char * data );
        void	cmdScroll( const char * data, bool loop );
        void	scrollLeft( int color );
        void	scrollRight( int color );
        void	scrollUp( int color );
        void	scrollDown( int color );

        QVector<CDGPacket>  m_cdgStream;	// Parsed CD+G stream storage
        int					m_cdgStreamIndex;	// packet offset which hasn't been processed yet

        // Rendering stuff
        quint32			   m_colorTable[16];// CD+G color table; color format is A8R8G8B8
        quint8			   m_bgColor;       // Background color index
        quint8             m_borderColor;   // Border color index
        quint8			   m_cdgScreen[CDG_FULL_WIDTH*CDG_FULL_HEIGHT];	// Image state for CD+G stream

        // These values are used to implement screen shifting.  The CDG specification allows the entire
        // screen to be shifted, up to 5 pixels right and 11 pixels down.  This shift is persistent
        // until it is reset to a different value.  In practice, this is used in conjunction with
        // scrolling (which always jumps in integer blocks of 6x12 pixels) to perform
        // one-pixel-at-a-time scrolls.
        quint8				m_hOffset;
        quint8				m_vOffset;

        // The rendered image
        QImage              m_renderedCdg;
};

#endif // PLAYERLYRICSCDG_H
