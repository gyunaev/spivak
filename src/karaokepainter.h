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

#ifndef KARAOKEPAINTER_H
#define KARAOKEPAINTER_H

#include <QImage>
#include <QPainter>


// Extends QPainter with our specific functionality
class KaraokePainter : public QPainter
{
    public:
        KaraokePainter( QImage * img );

        // Paint area locations
        QSize   size() const { return m_rect.size(); }
        QRect   rect() const { return m_rect; }

        // Returns the notification area rect
        QRect   notificationRect() const;

        // Returns the text area rect (to render text lyrics)
        QRect   textRect() const;

        // Sets clipping/translation to draw on the main area (no notifications)
        void    setClipAreaMain();

        // Returns the largest font size the textline could fit the current area lenght-wise
        int     largestFontSize( const QString& textline, int maxheight = -1 );

        // Same but also using the specific font and specific width
        static int     largestFontSize(const QFont& font, int maxfontsize, int width, const QString& textline, int maxheight = -1 );

        // Returns the tallest font size which could fit into the specified height
        static int     tallestFontSize( QFont &font, int height );

        // Draws text with the specified color, but with black outline
        void    drawOutlineText( int x, int y, const QColor& color, const QString& text );

        // Draws text with the lyrics colors from-to-spot, and have a spotlight at specific percentage with specific width
        void    drawOutlineTextGradient(int x, int y, double percentage, const QString &text, int alpha = 255 );

        // Draws text x-centered at percentage y-wise (such as "70% of y") with the specified color, but with black outline and gradient
        void    drawCenteredOutlineTextGradient( int ypercentage, double percentage, const QString &text, int alpha = 255 );

        // Draws text x-centered at percentage y-wise (such as "70% of y") with the specified color, but with black outline
        void    drawCenteredOutlineText( int ypercentage, const QColor& color, const QString& text );

    private:
        void    drawOutlineTextInternal(int x, int y, const QColor &color, const QString &text, QBrush brush );

        QRect       m_rect;
        QImage  *   m_image;
        QRect       m_textRect;
};

#endif // KARAOKEPAINTER_H
