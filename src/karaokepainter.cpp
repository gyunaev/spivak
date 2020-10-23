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

#include <QPainterPath>

#include "karaokepainter.h"
#include "settings.h"

// Screen composition:
static const unsigned int TOP_SCREEN_RESERVED_STATUS = 5;

// Those are percents, not pixels
static const unsigned int TEXTLYRICS_SPACING_LEFTRIGHT = 5;
static const unsigned int TEXTLYRICS_SPACING_TOPBOTTOM = 3;


KaraokePainter::KaraokePainter(QImage *img)
    : QPainter( img )
{
    m_image = img;
    m_rect = img->rect();

    // More clipping
    int xspacing = (m_rect.width() * TEXTLYRICS_SPACING_LEFTRIGHT) / 100;
    int yspacing = (m_rect.height() * TEXTLYRICS_SPACING_TOPBOTTOM) / 100;

    int textwidth = m_rect.width() - xspacing * 2;
    int textheight = m_rect.height() - yspacing * 2;

    m_textRect = QRect( xspacing, yspacing, textwidth, textheight );
}


QRect KaraokePainter::notificationRect() const
{
    int toparea = (m_image->height() * TOP_SCREEN_RESERVED_STATUS) / 100;
    return QRect( 0, 0, m_image->width(), toparea );
}

QRect KaraokePainter::textRect() const
{
    return m_textRect;
}

void KaraokePainter::setClipAreaMain()
{
    // Keep the top off the renderer
    int h = notificationRect().height() + 1;
    m_rect = QRect( 0, 0, m_image->width(), m_image->height() - h );
    translate( 0, h );
}

int KaraokePainter::largestFontSize(const QFont &font, int maxsize, int width, const QString &textline, int maxheight )
{
    int minsize = 8;
    int cursize = 10;
    QFont testfont( font );

    // We are trying to find the maximum font size which fits by doing the binary search
    while ( maxsize - minsize > 1 )
    {
        cursize = minsize + (maxsize - minsize) / 2;
        testfont.setPointSize( cursize );
        QSize size = QFontMetrics( testfont ).size( Qt::TextSingleLine, textline );
        //qDebug("%d-%d: trying font size %d to draw on %d: width %d, height: %d/%d", minsize, maxsize, cursize, width, fm.width(textline), fm.height(), maxheight );

        if ( size.width() < width && (maxheight == -1 || size.height() <= maxheight ) )
            minsize = cursize;
        else
            maxsize = cursize;
    }

    //qDebug("Chosen font size %d for font %s, width %d", cursize, qPrintable(font.family()), width );
    return cursize;
}

int KaraokePainter::tallestFontSize( QFont &font, int height )
{
    int maxsize = 128;
    int minsize = 8;
    int cursize;

    // We are trying to find the maximum font size which fits by doing the binary search
    while ( maxsize - minsize > 1 )
    {
        cursize = minsize + (maxsize - minsize) / 2;
        font.setPointSize( cursize );
        QFontMetrics fm( font );
        //qDebug("trying font size %d to draw on %d: height %d %d", cursize, height, fm.lineSpacing(), fm.height() );

        if ( fm.lineSpacing() < height )
            minsize = cursize;
        else
            maxsize = cursize;
    }

    return cursize;
}

int KaraokePainter::largestFontSize(const QString &textline , int maxheight)
{
    return largestFontSize( font(), pSettings->playerLyricsFontMaxSize, textRect().width(), textline, maxheight );
}

void KaraokePainter::drawOutlineText(int x, int y, const QColor &color, const QString &text)
{
    drawOutlineTextInternal( x, y, color, text, color );
}

void KaraokePainter::drawOutlineTextGradient(int x, int y, double percentage, const QString &text, int alpha )
{
    QLinearGradient gradient( x, y, x + fontMetrics().width( text ), y );

    // Adjust colors with alpha
    QColor before = pSettings->playerLyricsTextBeforeColor;
    QColor after = pSettings->playerLyricsTextAfterColor;
    QColor spot = pSettings->playerLyricsTextSpotColor;

    before.setAlpha( alpha );
    after.setAlpha( alpha );
    spot.setAlpha( alpha );

    double spotwidth = qMax( 0.02, (double) font().pointSize() / 1250 );

    gradient.setColorAt( 0, after );
    gradient.setColorAt( 1, before );

    gradient.setColorAt( qMax( percentage - spotwidth, 0.0 ), after );
    gradient.setColorAt( percentage, spot );
    gradient.setColorAt( qMin( percentage + spotwidth, 1.0), before );

    // Color here is only used for alpha
    drawOutlineTextInternal( x, y, before, text, gradient );
}

void KaraokePainter::drawCenteredOutlineTextGradient(int ypercentage, double percentage, const QString &text, int alpha)
{
    int x = (m_textRect.width() - fontMetrics().width( text ) ) / 2 + m_textRect.x();
    int y = (m_textRect.height() * ypercentage / 100 ) + fontMetrics().ascent() + m_textRect.y();

    drawOutlineTextGradient( x, y, percentage, text, alpha );
}

void KaraokePainter::drawOutlineTextInternal(int x, int y, const QColor &color, const QString &text, QBrush brush )
{
    QPainterPath path;
    path.addText( x, y, font(), text );

    QColor black( Qt::black );
    black.setAlpha( color.alpha() );

    QPen pen( black );
    pen.setWidth( qMax( 1, font().pointSize() / 30 ) );

    setPen( pen );
    setBrush( brush );
    drawPath( path );
}


void KaraokePainter::drawCenteredOutlineText(int ypercentage, const QColor &color, const QString &text)
{
    int x = (m_textRect.width() - fontMetrics().width( text ) ) / 2 + m_textRect.x();
    int y = (m_textRect.height() * ypercentage / 100 ) + fontMetrics().height() + m_textRect.y();

    drawOutlineText( x, y, color, text );
}
