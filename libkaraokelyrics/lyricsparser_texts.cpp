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

#include "lyricsparser_texts.h"

LyricsParser_Texts::LyricsParser_Texts(LyricsLoaderCallback *callback )
    : LyricsParser( callback )
{
}

void LyricsParser_Texts::parse(QIODevice *file, LyricsLoader::Container &output, LyricsLoader::Properties &properties)
{
    QByteArray lyrics = load( file );

    // TXT could be UltraStar or PowerKaraoke
    if ( lyrics.trimmed().startsWith( '#' ) )
        parseUStar( lyrics, output, properties );
    else
        parsePowerKaraoke( lyrics, output, properties );
}

bool LyricsParser_Texts::isUltrastarLyrics(QIODevice *file, QString *mp3file)
{
    LyricsParser_Texts parser( 0 );
    LyricsLoader::Container output;
    LyricsLoader::Properties properties;

    try
    {
        parser.parse( file, output, properties );
    }
    catch ( ... )
    {
        return false;
    }

    // TXT could be UltraStar or PowerKaraoke, if we dont have this prop this aint ultrastar
    if ( !properties.contains( LyricsLoader::PROP_MUSICFILE ) )
        return false;

    *mp3file = properties[ LyricsLoader::PROP_MUSICFILE ];
    return true;
}

void LyricsParser_Texts::parseUStar( const QByteArray& data, LyricsLoader::Container &output, LyricsLoader::Properties &properties)
{
    bool header = true;
    bool relative = false;
    double bpm = -1;
    int gap = -1;
    double msecs_per_beat = 0;
    int last_time_ms = 0;

    // Generate the content for encoding detection
    QByteArray lyricsForEncoding;

    Q_FOREACH( const QByteArray& line, data.split('\n') )
    {
        if ( line.startsWith( '#' ) )
            continue;

        if ( line.startsWith( '-' ) )
            lyricsForEncoding.append( '\n' );

        // Ultrastar entries are space-separated, and the one with text has 4+ entries
        int offset = 0, pos = 0;

        // : 20 4 17 с мо
        for ( ; offset < line.length(); offset ++ )
        {
            if ( line[offset] == ' ' )
            {
                pos++;

                if ( pos == 4 )
                    break;
            }
        }

        if ( offset >= line.length() )
            continue;

        lyricsForEncoding.append( line.mid( offset + 1 ) );
    }

    // Detect the encoding
    QTextCodec * codec = detectEncoding( lyricsForEncoding, properties );
    QStringList text = codec->toUnicode( data ).split( '\n' );

    // See http://www.ultrastarstuff.com/html/tutorialtxtfile.html
    for ( int i = 0; i < text.size(); i++ )
    {
        QString line = text[i];

        if ( header )
        {
            QRegExp regex( "^#([a-zA-Z0-9]+):\\s*(.*)$" );

            if ( regex.indexIn( line ) != -1 )
            {
                QString tag = regex.cap( 1 );
                QString value = regex.cap( 2 );
                LyricsLoader::Property tagid = LyricsLoader::PROP_INVALID;

                if ( tag == "TITLE" )
                    tagid = LyricsLoader::PROP_TITLE;
                else if ( tag == "ARTIST" )
                    tagid = LyricsLoader::PROP_ARTIST;
                else if ( tag == "MP3FILE" || tag == "MP3" )
                    tagid = LyricsLoader::PROP_MUSICFILE;
                else if ( tag == "BPM" )
                    bpm = value.toDouble();
                else if ( tag == "GAP" )
                    gap = qRound( value.toDouble() );   // May be a double value
                else if ( tag == "RELATIVE" )
                    relative = value.compare( "yes" );
                else if ( tag == "BACKGROUND" )
                    tagid = LyricsLoader::PROP_BACKGROUND;
                else if ( tag == "VIDEO" )
                    tagid = LyricsLoader::PROP_VIDEO;
                else if ( tag == "VIDEOGAP" )
                    tagid = LyricsLoader::PROP_VIDEO_STARTOFFSET;

                if ( tagid != LyricsLoader::PROP_INVALID )
                    properties[ tagid ] = value;
            }
            else
            {
                // Tag not found; either header ended, or invalid file
                if ( bpm == -1 || gap == -1 )
                    throw("BPM and/or GAP is missing.");

                msecs_per_beat = (60.0 / bpm / 4.0) * 1000.0;
                header = false;
            }
        }

        // We may fall-through, so no else
        if ( !header )
        {
            if ( line[0] != 'E' && line[0] != ':' && line[0] != '*' && line[0] != 'F' && line[0] != '-' )
                throw( "Not a valid format");

            // End?
            if ( line[0] == 'E' )
                break;

            // Could be either end of line, or lyrics
            int timing;

            if ( line[0] == '-' )
            {
                // End of line
                timing = gap + (relative ? last_time_ms : 0) + line.mid( 2 ).toInt() * msecs_per_beat;

                output.push_back( Lyric( timing ) );
            }
            else
            {
                // Lyrics
                QRegExp regex( "^[*Ff:]\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s*(.*)?$" );

                if ( regex.indexIn( line ) == -1 )
                    throw("Invalid UltraStar file format");

                timing = gap + (relative ? last_time_ms : 0) + regex.cap( 1 ).toInt() * msecs_per_beat;

                Lyric lyr( timing );
                lyr.pitch = regex.cap( 3 ).toInt();

                if ( line[0] == 'F' )
                    lyr.pitch |= Lyric::PITCH_FREESTYLE;
                else if ( line[0] == '*' )
                    lyr.pitch |= Lyric::PITCH_GOLDEN;

                lyr.duration = regex.cap( 2 ).toInt() * msecs_per_beat;

                if ( regex.captureCount() > 3 )
                    lyr.text = regex.cap( 4 );

                output.push_back( lyr );
            }

            last_time_ms = timing;
        }
    }
}

static int powerKaraokeTime( QString time )
{
    int timing = 0;

    if ( time.contains(":") )
    {
        QStringList parts = time.split( ":" );
        timing = parts[0].toInt() * 60000;
        time = parts[1];
    }

    timing += (int) (time.toFloat() * 1000 );
    return timing;
}


void LyricsParser_Texts::parsePowerKaraoke( const QByteArray& data, LyricsLoader::Container &output, LyricsLoader::Properties &properties )
{
    // This format has no special markers, so we can feed it as-is
    QTextCodec * codec = detectEncoding( data, properties );

    QStringList text = codec->toUnicode( data ).split( '\n' );

    // For the PowerKaraoke format there is no header, just times.
    QRegExp regex("^([0-9.:]+) ([0-9.:]+) (.*)");

    // Analyze each line
    for ( int i = 0; i < text.size(); i++ )
    {
        QString line = text[i];

        if ( line.isEmpty() )
            continue;

        // Try to match the sync first
        if ( line.indexOf( regex ) == -1 )
            throw QString( "Invalid PowerKaraoke file" );

        int start = powerKaraokeTime( regex.cap( 1 ) );
        //int end = powerKaraokeTime( regex.cap( 2 ) );
        QString text = regex.cap( 3 ).trimmed();

        Lyric lyr( start );

        if ( text.endsWith( "\\n" ) )
        {
            text.chop( 2 );
            lyr.text = text;
            output.push_back( lyr );

            // and end of line
            output.push_back( Lyric( start ) );
        }
        else if ( !text.endsWith( "-" ) )
        {
            text += " ";
            lyr.text = text;
            output.push_back( lyr );
        }
        else
        {
            text.chop( 1 );
            lyr.text = text;
            output.push_back( lyr );
        }
    }
}
