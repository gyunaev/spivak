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

#include <QMap>
#include <QBuffer>
#include <QStringList>

#include "aes.h"

#include "karaokeplayable_zip.h"
#include "karaokeplayable_kfn.h"

KaraokePlayable_KFN::KaraokePlayable_KFN(const QString &basename, QStringDecoder *filenameDecoder)
    : KaraokePlayable( basename, filenameDecoder )
{
    m_kfnZip = 0;
}

bool KaraokePlayable_KFN::init()
{
    // Open the KFN file
    m_file.setFileName( m_baseFile );

    if ( !m_file.open( QIODevice::ReadOnly ) )
    {
        m_errorMsg = "Cannot open file for reading";
        return false;
    }

    try
    {
        // See http://www.ulduzsoft.com/2012/10/reverse-engineering-the-karafun-file-format-part-1-the-header/
        char name[4];

        // Check the signature
        readBytes( name, 4 );

        // Is it PK (ZIP)?
        if ( name[0] == 'P' && name[1] == 'K' )
        {
            m_file.close();
            parseOldKarafunFile();
        }
        else if ( name[0] == 'K' && name[1] == 'F' && name[2] == 'N' && name[3] == 'B' )
        {
            parseNewKarafunFile();
        }
        else
        {
            m_errorMsg = "Not a valid KFN file";
            return false;
        }
    }
    catch ( QString ex )
    {
        m_errorMsg = ex;
        return false;
    }

    return true;
}

bool KaraokePlayable_KFN::isCompound() const
{
    return true;
}

QString KaraokePlayable_KFN::absolutePath(const QString &)
{
    return QString();
}

QStringList KaraokePlayable_KFN::enumerate()
{
    return m_fakeEntries;
}

bool KaraokePlayable_KFN::extract(const QString &object, QIODevice *out )
{
    // Old karafun?
    if ( m_kfnZip )
        return m_kfnZip->extract( object, out );

    // Find a file
    int ientry;

    for ( ientry = 0; ientry < m_entries.size(); ientry++ )
        if ( m_entries[ientry].filename == object )
            break;

    if ( ientry >= m_entries.size() )
        return false;

    return extractNew( ientry, out );
}

void KaraokePlayable_KFN::parseOldKarafunFile()
{
    m_kfnZip = new KaraokePlayable_ZIP( m_baseFile, m_filenameDecoder );

    if ( !m_kfnZip->init() )
        throw QString( "Invalid ZIP file" );

    // Parse the song.ini file into this buffer
    QBuffer buf;
    buf.open( QIODevice::ReadWrite );

    // Prepare the fake enumeration
    Q_FOREACH ( QString entry, m_kfnZip->enumerate() )
    {
        // Could be in any case, and ZIP is not case-sensitive
        if ( !entry.compare( "song.ini", Qt::CaseInsensitive ) )
            m_lyricObject = entry;

        m_fakeEntries.push_back( entry );
    }
}

void KaraokePlayable_KFN::parseNewKarafunFile()
{
    char name[4];

    // Parse the file header
    while ( 1 )
    {
        readBytes( name, 4 );
        quint8 type = readByte();
        quint32 len_or_value = readDword();

        // Type 2 is variable data length
        if ( type == 2 )
        {
            QByteArray data = readBytes( len_or_value );

            // Store the AES encoding key
            if ( name[0] == 'F' && name[1] == 'L' && name[2] == 'I' && name[3] == 'D' )
            {
                if ( len_or_value != 16 )
                    throw QString("AES encryption key is not a valid length");

                memcpy( m_aesKey, data, 16 );
            }
        }

        // End of header?
        if ( name[0] == 'E' && name[1] == 'N' && name[2] == 'D' && name[3] == 'H' )
            break;
    }

    // Parse the directory
    quint32 totalFiles = readDword();

    for ( quint32 i = 0; i < totalFiles; i++ )
    {
        Entry entry;

        entry.filename = readString( readDword() );
        entry.type = readDword();
        entry.length_out = readDword();
        entry.offset = readDword();
        entry.length_in = readDword();
        entry.flags = readDword();

        m_entries.push_back( entry );
    }

    // Since all the offsets are based on the end of directory, readjust them and find the music/lyrics
    int songidxentry = -1;

    for ( int i = 0; i < m_entries.size(); i++ )
    {
        m_entries[i].offset += m_file.pos();

        // Make sure the KFN file is large enough
        if ( m_entries[i].offset + m_entries[i].length_in > m_file.size() )
            throw QString("File is too short, data cannot be extracted");

        if ( m_entries[i].type == Entry::TYPE_SONGTEXT )
        {
            songidxentry = i;
            m_lyricObject = m_entries[i].filename;
            m_fakeEntries.push_back( m_lyricObject );
        }
        else if ( m_entries[i].type == Entry::TYPE_MUSIC && m_musicObject.isEmpty() )
        {
            m_musicObject = m_entries[i].filename;
            m_fakeEntries.push_back( m_musicObject );
        }
        else
            m_fakeEntries.push_back( m_entries[i].filename );
    }

    // Extract song.ini and find the source music and background image or video
    QBuffer lyricbuf;
    lyricbuf.open( QIODevice::WriteOnly );

    if ( songidxentry == -1 || !extractNew( songidxentry, &lyricbuf ) )
        throw QString("Song.ini is not found or couldn't be read");

    // We do not need a full parser, just the simple string lookup
    const QByteArray& lines = lyricbuf.data();

    int o2, o1 = lines.indexOf( "\nSource=" );

    if ( o1 != -1 && (o2 = lines.indexOf( '\n', o1 + 8 )) != -1 )
    {
        QByteArray line = lines.mid( o1 + 8, o2 - o1 - 8 );

        // Skip first two commas
        for ( o1 = 0, o2 = 0; o1 < line.length() && o2 != 2; o1++ )
            if ( line[o1] == ',' )
                o2++;

        if ( o2 == 2 )
            m_musicObject = line.mid( o1 ).trimmed();
    }
}

bool KaraokePlayable_KFN::extractNew(int entryidx, QIODevice *out)
{
    const Entry& entry = m_entries[entryidx];

    // The file starts here
    m_file.seek( entry.offset );

    // For a non-encrypted files we just pass everything
    if ( (entry.flags & 0x01) == 0 )
    {
        int offset = 0;

        while ( offset < entry.length_in )
        {
            char buf[32768];

            int toread = qMin( (int) sizeof(buf), entry.length_in - offset );

            if ( m_file.read( buf, toread ) != toread )
                return false;

            out->write( buf, toread );
            offset += toread;
        }

        return true;
    }

    // Now handle the encrypted content
    int total_in = 0, total_out = 0;

    // Decode using ARM AES
    mbedtls_aes_context aesctx;
    mbedtls_aes_init( &aesctx );
    mbedtls_aes_setkey_dec( &aesctx, (unsigned char*)  m_aesKey, 128 );

    // Size of the buffer must be a multiple of 16
    char buffer[65536], outbuf[65536];

    while ( total_in < entry.length_in )
    {
        int toRead = qMin( (unsigned int)  sizeof(buffer), (unsigned int) entry.length_in - total_in );
        int bytesRead = m_file.read( buffer, toRead );

        // We might need to write less than we read since the file is rounded to 16 bytes
        int toWrite = sizeof(outbuf);

        if ( bytesRead != toRead )
        {
            m_errorMsg = "File truncated";
            return false;
        }

        // Decrypt the whole buffer
        for ( int i = 0; i < bytesRead; i += 16 )
        {
            mbedtls_aes_crypt_ecb( &aesctx,
                                   MBEDTLS_AES_DECRYPT,
                                   (unsigned char*) buffer + i,
                                   (unsigned char*) outbuf + i );
        }

        //AES128_ECB_decrypt( (uint8_t *) buffer, (uint8_t *) m_aesKey, (uint8_t *) outbuf, bytesRead );

        out->write( outbuf, toWrite );
        total_out += toWrite;
        total_in += bytesRead;
    }

    mbedtls_aes_free( &aesctx );
    return true;
}

quint8 KaraokePlayable_KFN::readByte()
{
    quint8 byte;

    if ( m_file.read( (char*) &byte, 1) != 1 )
        throw QString("premature end of file");

    return byte;
}

quint16	KaraokePlayable_KFN::readWord()
{
    quint8 b1 = readByte();
    quint8 b2 = readByte();

    return b2 << 8 | b1;
}

quint32	KaraokePlayable_KFN::readDword()
{
    quint8 b1 = readByte();
    quint8 b2 = readByte();
    quint8 b3 = readByte();
    quint8 b4 = readByte();

    return b4 << 24 | b3 << 16 | b2 << 8 | b1;
}

QByteArray KaraokePlayable_KFN::readBytes( unsigned int len )
{
    QByteArray arr = m_file.read( len );

    if ( arr.size() != (int) len )
        throw QString("premature end of file");

    return arr;
}

QString KaraokePlayable_KFN::readString( unsigned int len )
{
    QByteArray arr = readBytes( len );

    return decodeFilename( arr );
}

void KaraokePlayable_KFN::readBytes( char * buf, unsigned int len )
{
    if ( m_file.read( buf, len ) != len )
        throw QString("premature end of file");
}
