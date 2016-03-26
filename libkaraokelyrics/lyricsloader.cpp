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

#include <QFile>

#include "lyricsloader.h"
#include "lyricsparser.h"
#include "lyricsparser_midi.h"
#include "lyricsparser_kfn.h"
#include "lyricsparser_kok.h"
#include "lyricsparser_lrc.h"
#include "lyricsparser_texts.h"
#include "lyricsparser_lyric.h"


Lyric::Lyric(qint64 start_, const QString &text_, qint64 duration_, int pitch_)
{
    start = start_;
    text = text_;
    duration = duration_;
    pitch = pitch_;
}

LyricsLoader::LyricsLoader(Properties &properties, Container &container)
    : m_lyricsOutput(container), m_props(properties)
{
}

bool LyricsLoader::parse(const QString &lyricsfile, QIODevice * file, LyricsLoaderCallback *callback )
{
    try
    {
        LyricsParser * parser = 0;

        if ( lyricsfile.endsWith( ".mid", Qt::CaseInsensitive)
        || lyricsfile.endsWith( ".midi", Qt::CaseInsensitive)
        || lyricsfile.endsWith( ".kar", Qt::CaseInsensitive) )
        {
            parser = new LyricsParser_MIDI( callback );
        }
        else if ( lyricsfile.endsWith( ".lrc", Qt::CaseInsensitive) )
            parser = new LyricsParser_LRC( callback );
        else if ( lyricsfile.endsWith( ".kok", Qt::CaseInsensitive) )
            parser = new LyricsParser_KOK( callback );
        else if ( !lyricsfile.compare(  "song.ini", Qt::CaseInsensitive) )
            parser = new LyricsParser_KFN( callback );
        else if ( lyricsfile.endsWith( ".txt", Qt::CaseInsensitive) )
            parser = new LyricsParser_Texts( callback );
        else if ( lyricsfile.endsWith( ".lyric", Qt::CaseInsensitive) )
            parser = new LyricsParser_Lyric( callback );
        else
        {
            m_errorMsg = "No parser for this format";
            return false;
        }

        // Parse the lyrics
        parser->parse( file, m_lyricsOutput, m_props );
        delete parser;

        // Pass through the lyrics first, adjust timing and collect the data for encoding detection
        for ( int i = 0; i < m_lyricsOutput.size(); i++ )
        {
            // Keep the duration if already present
            if ( m_lyricsOutput[i].duration != -1 )
                continue;

            // Do we have next lyrics
            // If not, calculate it as min( time to next, 500ms)
            if ( i < m_lyricsOutput.size() - 1 )
                m_lyricsOutput[i].duration = qMin( m_lyricsOutput[i+1].start - m_lyricsOutput[i].start, (qint64) 500 );
            else
                m_lyricsOutput[i].duration = 500;
        }

        // Trim the empty values at the end, if any
        while ( !m_lyricsOutput.isEmpty() && m_lyricsOutput.last().text.isEmpty() )
            m_lyricsOutput.takeLast();

        if ( m_lyricsOutput.isEmpty() )
        {
            m_errorMsg = "Lyrics file has no lyrics";
            return false;
        }

        return true;
    }
    catch ( const char * err )
    {
        m_errorMsg = err;
    }
    catch ( const QString& err )
    {
        m_errorMsg = err;
    }

    return false;
}

bool LyricsLoader::isSupportedFile(const QString &file)
{
    // song.ini is a karafun lyric file
    static const char * extlist[] = { ".mid", ".midi", ".kar", ".txt", ".lrc", ".kok", "song.ini", ".lyric", 0 };

    for ( int i = 0; extlist[i]; i++ )
        if ( file.endsWith( extlist[i], Qt::CaseInsensitive ) )
            return true;

    return false;
}

void LyricsLoader::dump(const Container &output)
{
    Q_FOREACH ( const Lyric& l, output )
    {
        if ( l.text.isEmpty() )
            printf("\n");
        else
            printf("%s - %s: '%s' ", qPrintable( LyricsParser::timeAsText( l.start) ),
                   l.duration == -1 ? "?" : qPrintable( LyricsParser::timeAsText( l.start + l.duration ) ),
                   qPrintable( l.text ) );
    }

    printf("\n");
}
