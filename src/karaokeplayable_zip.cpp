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

#include <QStringList>
#include <QBuffer>
#include <QVector>

// For open/close and _wopen/_close
#if defined (Q_OS_WIN)
    #include <io.h>
    #include <fcntl.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif


#include "karaokeplayable_zip.h"
#include "zip.h"

KaraokePlayable_ZIP::KaraokePlayable_ZIP(const QString &baseFile, QTextCodec *filenameDecoder)
    : KaraokePlayable( baseFile, filenameDecoder )
{
    m_zip = 0;
}

KaraokePlayable_ZIP::~KaraokePlayable_ZIP()
{
    if ( m_zip )
        zip_close( m_zip );
}

bool KaraokePlayable_ZIP::init()
{
    // We use zip_fdopen instead of zip_open because latter does not support Unicode file names.
    // We also can't use QFile + handle since zip_fdopen takes over the handle, and it is impossible
    // to have QFile not closing it on delete (the functions which allows disabling it doesn't take the file name)
#if defined (Q_OS_WIN)
    QVector<wchar_t> wcharbuf( m_baseFile.length() +  1);
    m_baseFile.toWCharArray( wcharbuf.data() );

    int fd = _wopen( wcharbuf.constData(), _O_BINARY | _O_RDONLY );
#else
    int fd = open( QFile::encodeName( m_baseFile ).constData(), O_RDONLY );
#endif

    if ( fd < 0 )
    {
        m_errorMsg = "Could not open file";
        return false;
    }

    // Open the ZIP archive: http://www.nih.at/libzip/zip_fdopen.html
    int errcode;
    m_zip = zip_fdopen( fd, 0, &errcode );

    if ( !m_zip  )
    {
#if defined (Q_OS_WIN)
        _close( fd );
#else
        close( fd );
#endif
        m_errorMsg = QString( "Error %1 opening ZIP archive") .arg( errcode );
        return false;
    }

    return true;
}

bool KaraokePlayable_ZIP::isCompound() const
{
    return true;
}

QStringList KaraokePlayable_ZIP::enumerate()
{
    QStringList filelist;

    // http://www.nih.at/libzip/zip_get_num_entries.html
    int files = zip_get_num_entries( m_zip, ZIP_FL_UNCHANGED );

    if ( files == 0 )
    {
        m_errorMsg = "archive has no files";
        return filelist;
    }

    for ( int i = 0; i < files; i++ )
    {
        // Retrieve the file size
        struct zip_stat fileinfo;

        // http://www.nih.at/libzip/zip_stat_index.html
        if ( zip_stat_index( m_zip, i, 0, &fileinfo) != 0 )
        {
            m_errorMsg = "file info not found";
            return QStringList();
        }

        filelist.push_back( decodeFilename( fileinfo.name ) );
    }

    return filelist;
}

QString KaraokePlayable_ZIP::absolutePath(const QString &)
{
    return QString();
}

bool KaraokePlayable_ZIP::extract(const QString &object, QIODevice *out)
{
    // Retrieve the file size
    struct zip_stat fileinfo;

    // http://www.nih.at/libzip/zip_stat.html
    if ( zip_stat( m_zip, object.toUtf8().constData(), 0, &fileinfo) != 0 )
    {
        m_errorMsg = zip_strerror( m_zip );
        return false;
    }

    // Make sure the size field is valid
    if ( (fileinfo.valid & ZIP_STAT_SIZE) == 0 || (fileinfo.valid & ZIP_STAT_INDEX) == 0 )
    {
        m_errorMsg = "not a valid file";
        return false;
    }

    // Open the file
    struct zip_file * zgh = zip_fopen_index( m_zip, fileinfo.index, 0 );

    if ( !zgh )
    {
        m_errorMsg = "cannot open zip file";
        return false;
    }

    // Extract the content
    char buf[32768];
    unsigned int offset = 0;

    while ( offset < fileinfo.size )
    {
        int toread = qMin( (qint64) sizeof(buf), (qint64) fileinfo.size - offset );
        int ret = zip_fread( zgh, buf, toread );

        if ( ret != toread )
        {
            m_errorMsg = "extraction failed";
            zip_fclose( zgh );
            return false;
        }

        out->write( buf, ret );
        offset += ret;
    }

    zip_fclose( zgh );
    return true;
}
