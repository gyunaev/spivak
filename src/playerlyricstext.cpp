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

#include <QImage>
#include <QPainter>
#include <QFontMetrics>

#include "logger.h"
#include "playerlyricstext.h"
#include "settings.h"
#include "karaokepainter.h"
#include "libkaraokelyrics/lyricsloader.h"
#include "util.h"


class PlayerLyricsTextCallback : public LyricsLoaderCallback
{
    public:
        PlayerLyricsTextCallback( const QString& lyricFile ) : LyricsLoaderCallback()
        {
            m_lyricFile = lyricFile;
        }

        // This function must detect the used text codec for the data, and return it, or return 0 (we will fall back to UTF-8)
        virtual QStringDecoder * detectTextCodec( const QByteArray& data )
        {
            QStringDecoder * enc = Util::detectEncoding( data );

            if ( !enc )
                enc = new QStringDecoder( qPrintable( pSettings->fallbackEncoding ) );

            return enc;
        }

        // This function is called when the lyric file is password-protected, the client must return the non-empty password
        // or return an empty string if no password is provided
        virtual QString getPassword()
        {
            return "";
        }

    private:
        QString     m_lyricFile;
};


PlayerLyricsText::PlayerLyricsText(const QString& artist, const QString& title )
    : PlayerLyrics()
{
    m_nextUpdateTime = 0;
    m_currentLine = 0;
    m_longestLine = 0;
    m_artist = artist;
    m_title = title;
    m_titleAnimationValue = 0.0;

    m_renderFont = pSettings->playerLyricsFont;
    m_renderFont.setWeight( QFont::Bold );
}

bool PlayerLyricsText::load(QIODevice * file, const QString& filename )
{
    LyricsLoader::Container lyrics;
    PlayerLyricsTextCallback callback( filename );

    LyricsLoader loader( m_properties, lyrics );

    if ( !loader.parse( filename, file, &callback ) )
    {
        m_errorMsg = loader.errorMsg();
        return false;
    }

    if ( m_properties.contains( LyricsLoader::PROP_DETECTED_ENCODING ) )
        Logger::debug( "Autodetected lyrics text encoding: %s", qPrintable( m_properties[ LyricsLoader::PROP_DETECTED_ENCODING ] ) );
    else
        Logger::debug( "Automatic lyrics text encoding detection failed; falling back to %s", qPrintable( pSettings->fallbackEncoding ) );

    // Convert them into sentences, and find the longest lyrics line when rendered by a lyric font
    QFontMetrics fm( m_renderFont );
    int maxwidthvalue = 0;
    int linestartidx = -1;

    for ( int i = 0; i <= lyrics.size(); i++ )
    {
        if ( i == lyrics.size() || lyrics[i].text.isEmpty() )
        {
            // If the first lyric is empty (weird), ignore it
            if ( m_lines.isEmpty() && linestartidx == -1 )
                continue;

            // If the empty lines follow, just add them as-is
            if ( linestartidx == -1  )
            {
                m_lines.push_back( PlayerLyricTextLine() );
                continue;
            }

            // Add the lyric line data
            m_lines.push_back( PlayerLyricTextLine( lyrics.mid( linestartidx, i - linestartidx ) ) );

            // And calculate the rendering width
            int render_width = m_lines.last().screenWidth( fm );

            if ( render_width > maxwidthvalue )
            {
                maxwidthvalue = render_width;
                m_longestLine = m_lines.size() - 1;
            }

            linestartidx = -1;
        }
        else
        {
            if ( linestartidx == -1 )
                linestartidx = i;
        }
    }

    if ( m_lines.isEmpty() )
        return false;

    m_nextUpdateTime = m_lines.first().startTime();
    m_showTitleTime = 0;

    // We can have those from the database, which is more reliable
    if ( m_artist.isEmpty() || m_title.isEmpty() )
    {
        if ( m_properties.contains( LyricsLoader::PROP_ARTIST ) )
            m_artist = m_properties[ LyricsLoader::PROP_ARTIST ];

        if ( m_properties.contains( LyricsLoader::PROP_TITLE ) )
            m_title = m_properties[ LyricsLoader::PROP_TITLE ];
    }

    // We can have those from the database, which is more reliable
    if ( !m_artist.isEmpty() && !m_title.isEmpty() )
    {
        if ( firstLyricStart() > MIN_INTERVAL_AFTER_TITLE + MIN_DURATION_TITLE )
        {
            m_nextUpdateTime = 0;
            m_showTitleTime = qMin( firstLyricStart() - MIN_INTERVAL_AFTER_TITLE, (qint64) MAX_DURATION_TITLE );
        }
    }


    //foreach ( const PlayerLyricTextLine& t, m_lines )
//        t.dump();

    //qDebug("Longest line for rendering is %d", m_longestLine );
    //m_lines[m_longestLine].dump();

    return true;
}

qint64 PlayerLyricsText::nextUpdate() const
{
    return m_nextUpdateTime;
}


void PlayerLyricsText::calculateFontSize()
{
    // How much maximum height do we have per line?
    int maxheight = m_usedImageSize.height() / (pSettings->playerLyricsFontFitLines + 1);

    int fontsize = KaraokePainter::largestFontSize( pSettings->playerLyricsFont,
                                                    pSettings->playerLyricsFontMaxSize,
                                                    m_usedImageSize.width(),
                                                    m_lines[m_longestLine].fullLine(),
                                                    maxheight );

    //qDebug("Chosen minimum size: %d", fontsize );
    m_renderFont.setPointSize( fontsize );
}

void PlayerLyricsText::renderTitle(KaraokePainter &p , qint64 time )
{
    int screen_10percent = p.textRect().height() / 10;

    // Artist name starts at 10% and can take the 30% of the screen
    QFont artistFont( m_renderFont );
    artistFont.setPointSize( p.largestFontSize( m_artist, 3 * screen_10percent ) );

    // Title starts at 50% and can take the 40% of the screen
    QFont titleFont( m_renderFont );
    titleFont.setPointSize( p.largestFontSize( m_title, 4 * screen_10percent ) );

    // Use the smallest size so we can fit for sure
    int alpha = 255;

    if ( m_showTitleTime - time < TITLE_FADEOUT_TIME )
    {
        alpha = (m_showTitleTime - time ) * 255 / TITLE_FADEOUT_TIME;
        m_nextUpdateTime = 0;
    }

    // Draw artist and title
    p.setFont( artistFont );
    p.drawCenteredOutlineTextGradient( 10, m_titleAnimationValue, m_artist, alpha  );

    p.setFont( titleFont );
    p.drawCenteredOutlineTextGradient( 50, 1.0 - m_titleAnimationValue, m_title, alpha  );

    m_titleAnimationValue += 0.025;

    if ( m_titleAnimationValue >= 1.0 )
        m_titleAnimationValue = 0.0;
}

qint64 PlayerLyricsText::firstLyricStart() const
{
    return m_lines.first().startTime();
}

void PlayerLyricsText::drawNotification(KaraokePainter &p, qint64 timeleft)
{
    p.setPen( Qt::black );
    p.setBrush( Qt::white );
    p.drawRect( p.textRect().x(), p.textRect().y(), timeleft * p.textRect().width() / MAX_NOTIFICATION_DURATION, qMax( p.textRect().height() / 50, 10 ) );
}


bool PlayerLyricsText::render(KaraokePainter &p, qint64 timems)
{
    // Title time?
    if ( m_showTitleTime > 0 && timems < m_showTitleTime )
    {
        renderTitle( p, timems );
        return true;
    }

    if ( p.textRect() != m_usedImageSize )
    {
        m_usedImageSize = p.textRect();
        calculateFontSize();
    }

    int current;

    // Current lyric lookup:
    for ( current = 0; current < m_lines.size(); current++ )
    {
        // For empty lines we look up for the next time
        if ( m_lines[current].isEmpty() )
            continue;

        // Line ended already
        if ( m_lines[current].endTime() < timems )
            continue;

        // We found it
        break;
    }

    // Do we need to draw the notification?
    qint64 notification_duration = 0;

    // We show preamble if:
    if ( current < m_lines.size()   // this is not the last lyric
         && timems < m_lines[current].startTime() // it hasn't started yet;
         && m_lines[current].startTime() - timems <= MAX_NOTIFICATION_DURATION // time to start is less than 5s
         && ( current == 0 || m_lines[current].startTime() - m_lines[current-1].endTime() > MIN_TIME_DIFF_FOR_PREAMBLE ) ) // it is first lyric, or there is more than 5s between end and start
    {
        notification_duration = m_lines[current].startTime() - timems;
    }

    // Draw until we have screen space
    QFontMetrics fm( m_renderFont );
    p.setFont( m_renderFont );

    // Draw current line(s)
    int yoffset = p.textRect().y();
    int ybottom = p.textRect().height() - yoffset;
    yoffset += + fm.height();

    int scroll_passed = 0, scroll_total = 0;

    // If we're playing after the first line, start scrolling it so for the moment the next line it is due
    // it would be at the position of the second line.
    if ( current > 0 && current < m_lines.size() )
    {
        // Should we scroll?
        if ( timems > m_lines[current].startTime() )
        {
            scroll_total = m_lines[current].endTime() - m_lines[current].startTime();
            scroll_passed = timems - m_lines[current].startTime();
            yoffset -= scroll_passed * fm.height() / scroll_total;
        }

        // Draw one more line in front (so our line is always the second one)
        current--;
    }

    if ( current == m_lines.size() )
        return true;

    // Draw notification/preamble
    if ( notification_duration != 0 )
        drawNotification( p, notification_duration );

    // If a current line is empty, track down once
    if ( m_lines[current].isEmpty() && current > 0 )
        current--;

    while ( yoffset < ybottom && current < m_lines.size() )
    {
        if ( !m_lines[current].isEmpty() )
        {
            if ( scroll_total > 0 )
            {
                // Animate out the first line
                int percentage = (100 * scroll_passed) / scroll_total;
                m_lines[current].drawDisappear( p, percentage, yoffset );
                scroll_total = 0;
            }
            else
            {
                m_lines[current].draw( p, timems, yoffset );
            }
        }

        current++;
        yoffset += fm.height();
    }

    return true;
}

QString PlayerLyricsText::exportAsText() const
{
    QString out;

    Q_FOREACH ( const PlayerLyricTextLine& t, m_lines )
        out += t.fullLine() + "\n";

    return out;
}
