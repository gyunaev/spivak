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

#include "libkaraokelyrics/lyricsparser.h"
#include "playerlyrictext_line.h"
#include "settings.h"
#include "karaokepainter.h"

PlayerLyricTextLine::PlayerLyricTextLine()
{
    m_start = 0;
    m_end = 0;
    m_duration = 0;
}

PlayerLyricTextLine::PlayerLyricTextLine(const LyricsLoader::Container &line)
{
    /*if ( pSettings->playerLyricsTextEachCharacter )
    {
        // Split it per-character
        foreach ( Lyric l, line )
        {
            int perchar = l.duration / l.text.length();

            for ( int i = 0; i < l.text.length(); i++ )
                m_line.push_back( Lyric( l.start + perchar * i, QString(l.text[i]), perchar, l.pitch ) );
        }
    }
    else*/
        m_line = line;

    foreach ( Lyric l, line )
        m_text += l.text;

    if ( !m_line.isEmpty() )
    {
        m_start = m_line.first().start;
        m_end = m_line.last().start + m_line.last().duration;
        m_duration = m_end - m_start;
    }
    else
    {
        m_start = 0;
        m_end = 0;
        m_duration = 0;
    }
}

qint64 PlayerLyricTextLine::startTime() const
{
    if ( m_line.isEmpty() )
        return -1;

    return m_line.first().start;
}

qint64 PlayerLyricTextLine::endTime() const
{
    if ( m_line.isEmpty() )
        return -1;

    return m_line.last().start + m_line.last().duration;
}

qint64 PlayerLyricTextLine::draw( KaraokePainter& p, qint64 time, int yoffset )
{
    if ( m_line.isEmpty() )
        return 0;

    int x = (p.textRect().width() - p.fontMetrics().boundingRect( m_text ).width()) / 2 + p.textRect().x();
\
    if ( time < m_start )
        p.drawOutlineText( x, yoffset, pSettings->playerLyricsTextBeforeColor, m_text );
    else if ( time > m_end )
        p.drawOutlineText( x, yoffset, pSettings->playerLyricsTextAfterColor, m_text );
    else
    {
        // The string is partially colored
        double spotwidth = qMax( 0.02, (double) p.font().pointSize() / 1250 );

        for ( int i = 0; i < m_line.size(); i++ )
        {
            double percentage = (double) (time - m_line[i].start) / (double) m_line[i].duration;

            if ( percentage < spotwidth  )
                p.drawOutlineText( x, yoffset, pSettings->playerLyricsTextBeforeColor, m_line[i].text );
            else if ( percentage + spotwidth > 1.0 )
                p.drawOutlineText( x, yoffset, pSettings->playerLyricsTextAfterColor, m_line[i].text );
            else
                p.drawOutlineTextGradient( x, yoffset, percentage, m_line[i].text );

            x += p.fontMetrics().width( m_line[i].text );
        }
    }

    return 0;
}

void PlayerLyricTextLine::drawDisappear(KaraokePainter& p, int percentage, int yoffset)
{
    if ( m_line.isEmpty() )
        return;

    float scale_level = 1.0F - ((float) percentage / 100.0F);

    p.save();
    p.scale( scale_level, scale_level );

    int x = (p.textRect().width() - p.fontMetrics().boundingRect( m_text ).width()) / 2 + p.textRect().x();

    // Draw the text string
    for ( int i = 0; i < m_line.size(); i++ )
    {
        p.drawOutlineText( x, yoffset, pSettings->playerLyricsTextAfterColor, m_line[i].text );
        x += p.fontMetrics().width( m_line[i].text );
    }

    p.restore();
}

int PlayerLyricTextLine::screenWidth(const QFontMetrics &font)
{
    return font.boundingRect( m_text ).width();
}


void PlayerLyricTextLine::dump(qint64) const
{
    for ( int i = 0; i < m_line.size(); i++ )
    {
        qDebug("%s%s - %s: '%s' ", i == 0 ? "" : "    ",
               qPrintable( LyricsParser::timeAsText( m_line[i].start) ),
               m_line[i].duration == -1 ? "?" : qPrintable( LyricsParser::timeAsText( m_line[i].start + m_line[i].duration ) ),
               qPrintable( m_line[i].text ) );
    }
}
