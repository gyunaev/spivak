/**************************************************************************
 *  Ulduzsoft Karaoke PLayer - cross-platform desktop karaoke player      *
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
#if 0

#include <QDebug>

#include <QMutex>
#include <QImage>
#include <QTimer>
#include <QThread>
#include <QVideoSurfaceFormat>
#include <QVideoFrame>
#include <QAbstractVideoSurface>
#include <QMediaMetaData>
#include <QApplication>
#include <QFile>

#include "logger.h"
#include "actionhandler.h"
#include "karaokepainter.h"
#include "mediaplayer_qt.h"
#include "settings.h"
#include "playernotification.h"


// A short story here. QMediaPlayer on Win7+ can't handle certain ID3v2 tags in the beginning of MP3 files - this
// results in situations where it refuses to play them altogether (even though Windows media plays them).
// To work around this issue, we open the MP3 file ourselves, and if it contains the MP3 tag, we hide it from
// QMediaPlayer by emulating a QIODevice based on this file, which will artificially shorten the file. This device
// we pass to QMediaPlayer. Again, this is Windows only, other platforms have no such issues.
class UniversalPlayerID3StripedMediaFile : public QIODevice
{
    public:
        UniversalPlayerID3StripedMediaFile( const QString& filename )
            : QIODevice(), m_file( filename )
        {
        }

        bool hasID3v23Tag()
        {
            // Open ourselves and the target file
            if ( !open( QIODevice::ReadOnly ) || !m_file.open( QIODevice::ReadOnly ) )
                return false;

            // See http://id3.org/id3v2.4.0-structure
            QByteArray header = m_file.read( 10 );

            if ( header[0] != 'I' || header[1] != 'D' || header[2] != '3' )
                return false;

            // 7,8 and 9 contain the header size in sync-safe encoding
            m_tagOffset = ((quint32) header[6]) << 21 | ((quint32) header[7]) << 14 | ((quint32) header[8]) << 7 | ((quint32) header[9]);

            if ( m_tagOffset > m_file.size() )
                return false; // something is wrong

            Logger::debug( "UniversalPlayer: ID3v2v4 detected, tag lenght %d, enabling workaround", m_tagOffset );
            m_file.seek( m_tagOffset );

            return true;
        }

        qint64	pos() const    { return m_file.pos() - m_tagOffset; }

        bool	seek(qint64 pos)
        {
            QIODevice::seek( pos );
            return m_file.seek( pos + m_tagOffset );
        }

        qint64	size() const      {  return m_file.size() - m_tagOffset; }

        bool isSequential()  { return false; }

        qint64	readData(char * data, qint64 maxSize)  { return m_file.read( data, maxSize ); }

        qint64	writeData(const char *, qint64 )  { return -1; }

    private:
            QFile   m_file;
            int     m_tagOffset;
};


// Background video surface implementation
class UniversalPlayerVideoSurface : public QAbstractVideoSurface
{
    public:
        UniversalPlayerVideoSurface()
        {
            m_frameFormat = QImage::Format_Invalid;
            m_valid = 0;
            m_flipped = false;
        }

    // Draws the frame on the image
    void draw( KaraokePainter& p )
    {
        QMutexLocker m( &m_mutex );

        if ( m_valid )
        {
            if ( !m_lastFrame.map(QAbstractVideoBuffer::ReadOnly) )
               return;

            QImage img( m_lastFrame.bits(), m_lastFrame.width(), m_lastFrame.height(), m_lastFrame.bytesPerLine(), m_frameFormat );

            // Some codecs (notably on Windows) return RGB32 images with is supposed to be 0xFF R G B (in little-endian), but in reality is 0x00 R G B.
            // QPainter draws those as transparent, apparently refusing to honor the format. Not sure if it is decoder or QPainter bug.
            // Hence we work it around here.
            if ( img.format() == QImage::Format_RGB32 && img.bits()[3] == 0x00 )
            {
#ifdef Q_LITTLE_ENDIAN
                uchar * p = img.bits() + 3;
#else
                uchar * p = img.bits();
#endif
                uchar * end = img.bits() + img.byteCount();

                // Walk through the whole image, setting Alpha byte to 255
                while ( p < end )
                {
                    *p = 255;
                    p += 4;
                }
            }

            // This is an unlikely case, so we'd rather have two code blocks to exclude unnecessary operations
            if ( m_flipped )
            {
                const QTransform oldTransform = p.transform();
                p.scale(1, -1);
                p.translate(0, -p.size().height());
                p.drawImage( p.rect(), img, m_viewport );
                p.setTransform(oldTransform);
            }
            else
                p.drawImage( p.rect(), img, m_viewport );

            m_lastFrame.unmap();
        }
    }

    // Overridden from QAbstractVideoSurface
    virtual bool start( const QVideoSurfaceFormat &format )
    {
        m_mutex.lock();

        m_flipped = ( format.scanLineDirection() == QVideoSurfaceFormat::BottomToTop );
        m_frameFormat = QVideoFrame::imageFormatFromPixelFormat( format.pixelFormat() );
        m_viewport = format.viewport();
        m_valid = 1;

        // We want to unlock it before calling the parent function, which may call present() and deadlock
        m_mutex.unlock();

        return QAbstractVideoSurface::start( format );
    }

    virtual void stop()
    {
        m_mutex.lock();
        m_valid = 0;
        m_lastFrame = QVideoFrame();
        m_mutex.unlock();

        QAbstractVideoSurface::stop();
    }

    // This one is overridden from QAbstractVideoSurface
    virtual bool present( const QVideoFrame &frame )
    {
        if ( surfaceFormat().pixelFormat() != frame.pixelFormat() || surfaceFormat().frameSize() != frame.size())
        {
             setError(IncorrectFormatError);
             stop();

             return false;
        }

        QMutexLocker m( &m_mutex );
        m_lastFrame = frame;

        return true;
    }

    // This one is overridden from QAbstractVideoSurface
    virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
    {
        if ( handleType == QAbstractVideoBuffer::NoHandle )
        {
            return QList<QVideoFrame::PixelFormat>()
                    << QVideoFrame::Format_RGB32
                    << QVideoFrame::Format_ARGB32
                    << QVideoFrame::Format_ARGB32_Premultiplied
                    << QVideoFrame::Format_RGB565
                    << QVideoFrame::Format_RGB555;
        }
        else
        {
            return QList<QVideoFrame::PixelFormat>();
        }
    }

    bool isValid() const
    {
        QMutexLocker m( &m_mutex );
        return m_valid;
    }

    private:
        QImage::Format      m_frameFormat;
        QVideoFrame         m_lastFrame;
        QRect               m_viewport;
        mutable QMutex      m_mutex;
        bool                m_flipped;
        bool                m_valid;
};



UniversalPlayer_QMediaPlayer::UniversalPlayer_QMediaPlayer( QObject *parent )
    : QObject(parent)
{
    m_surface = 0;
    m_drawEnabled = 0;
    m_player = 0;
    m_isPaused = false;
    m_hasErrors = false;
    m_crossfadeVolumeStep = 0;
    m_crossfadeDelay = 0;
    m_crossfadeOrigVolume = 0;
    m_deviceMusicFile = 0;
}

UniversalPlayer_QMediaPlayer::~UniversalPlayer_QMediaPlayer()
{
    if ( m_player )
    {
        m_player->stop();
        m_player->deleteLater();
    }

    delete m_deviceMusicFile;
}

bool UniversalPlayer_QMediaPlayer::loadAudio( const QString &musicfile )
{
    m_mediaFile = musicfile;
    newMediaPlayer( false, false );

#if defined (Q_OS_WIN)
    // If this is an MP3 file, open it and see if we have the ID3v2.4 tag there -
    // QMediaPlayer on Windows7 can't somehow handle those (although Windows Media player can,
    // so this is not WMF codec issue)
    m_deviceMusicFile = new UniversalPlayerID3StripedMediaFile( musicfile );

    if ( m_deviceMusicFile->hasID3v23Tag() )
    {
        m_player->setMedia( QMediaContent(musicfile), m_deviceMusicFile );
    }
    else
    {
        delete m_deviceMusicFile;
        m_deviceMusicFile = 0;

        m_player->setMedia( QMediaContent( QUrl::fromLocalFile( musicfile ) ) );
    }
#else
    m_player->setMedia( QMediaContent( QUrl::fromLocalFile( musicfile ) ) );
#endif

    return true;
}

bool UniversalPlayer_QMediaPlayer::loadVideo(const QString &videofile, bool muted )
{
    m_mediaFile = videofile;
    newMediaPlayer( true, muted );

    // Prepare the video output surface if we do not have it yet
    if ( !m_surface )
        m_surface = new UniversalPlayerVideoSurface();

    m_player->setVideoOutput( m_surface );
    m_player->setMedia( QMediaContent( QUrl::fromLocalFile( videofile ) ) );

    return true;
}

void UniversalPlayer_QMediaPlayer::draw(KaraokePainter &p)
{
    if ( m_drawEnabled )
    {
        QMutexLocker m( &m_drawMutex );

        // This value may change after the mutex is locked!
        if ( m_drawEnabled )
            m_surface->draw( p );
    }
}

void UniversalPlayer_QMediaPlayer::play()
{
    m_player->play();
    emit started( duration() );

    if ( m_surface )
        m_drawEnabled = 1;

    m_isPaused = false;
}

void UniversalPlayer_QMediaPlayer::stop()
{
    m_drawMutex.lock();
    m_drawEnabled = 0;
    m_drawMutex.unlock();

    if ( m_player )
        m_player->stop();
}

void UniversalPlayer_QMediaPlayer::pauseOrResume(bool pausing)
{
    m_isPaused = pausing;

    if ( m_crossfadeDelay != 0 )
    {
        // On resume m_player->volume() is 0
        if ( pausing )
            m_crossfadeOrigVolume = m_player->volume();

        // We are fading from m_current_volume to zero in pSettings->musicCollectionCrossfadeTime
        // and we will call us every 200ms
        int called_times = qMax( 1, (m_crossfadeDelay * 1000) / 200 );
        m_crossfadeVolumeStep = m_crossfadeOrigVolume / called_times;

        if ( pausing )
            m_crossfadeVolumeStep = -m_crossfadeVolumeStep;
    }

    if ( m_crossfadeVolumeStep != 0 )
        fadeVolume();

    if ( pausing )
    {
        // Pause it right away only if the fading is disabled
        if ( m_crossfadeVolumeStep == 0 )
            m_player->pause();
    }
    else
        m_player->play();   // Resume to fadein or just plain
}

void UniversalPlayer_QMediaPlayer::seekTo(qint64 time)
{
    m_player->setPosition( time );
}

qint64 UniversalPlayer_QMediaPlayer::position()
{
    return m_player->position();
}

void UniversalPlayer_QMediaPlayer::setVolume(int volume)
{
    // We do not allow the volume change if we're crossfading or paused
    if ( !m_player->isMuted() && m_crossfadeVolumeStep == 0 )
        m_player->setVolume( volume );
}

int UniversalPlayer_QMediaPlayer::volume()
{
    if ( m_player->isMuted() )
        return 0;

    return m_player->volume();
}

qint64 UniversalPlayer_QMediaPlayer::duration()
{
    return m_player->duration();
}

bool UniversalPlayer_QMediaPlayer::isValid() const
{
    return m_player != 0;
}

bool UniversalPlayer_QMediaPlayer::isPaused() const
{
    return m_isPaused;
}

void UniversalPlayer_QMediaPlayer::infoFromFile(QString &artist, QString &title)
{
    artist = m_player->metaData( QMediaMetaData::AlbumArtist ).toString();

    if ( artist.isEmpty() )
    {
        QStringList composer = m_player->metaData( QMediaMetaData::ContributingArtist ).toStringList();

        if ( composer.isEmpty() )
            composer = m_player->metaData( QMediaMetaData::Author ).toStringList();

        if ( !composer.isEmpty() )
            artist = composer[0];
    }

    title = m_player->metaData( QMediaMetaData::Title ).toString();
}

void UniversalPlayer_QMediaPlayer::setFadeDuration(unsigned int duration)
{
    m_crossfadeDelay = duration;
}

void UniversalPlayer_QMediaPlayer::slotError(QMediaPlayer::Error err )
{
    if ( !m_hasErrors )
    {
        m_hasErrors = true;
        Logger::error( "UniversalMedia: error %d reported for media %s: %s", err, qPrintable(m_mediaFile), qPrintable(m_player->errorString()) );
        m_errorMsg = m_player->errorString();
        emit error();
    }
}

void UniversalPlayer_QMediaPlayer::slotMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    //Logger::debug("UniversalPlayer_QMediaPlayer: media %s status changed: %d", qPrintable(m_mediaFile), status );

    if ( status == QMediaPlayer::EndOfMedia )
        emit finished();

    if ( status == QMediaPlayer::LoadedMedia )
    {
        m_errorMsg.clear();
        emit loaded();
    }
}

void UniversalPlayer_QMediaPlayer::fadeVolume()
{
    if ( !m_player )
        return;

    // Apply the crossfade step
    int newvol = qMax( 0, qMin( m_player->volume() + m_crossfadeVolumeStep, (int) m_crossfadeOrigVolume ) );
    m_player->setVolume( newvol );

    // If we reached out to zero, pause the player
    if ( newvol <= 0 )
        m_player->pause();
    else
    {
        // If we got the full volume, no more crossfading
        if ( newvol >= (int) m_crossfadeOrigVolume )
        {
            m_crossfadeVolumeStep = 0;
            return;
        }

        // Rinse and repeat
        QTimer::singleShot( 200, this, SLOT(fadeVolume()) );
    }
}

void UniversalPlayer_QMediaPlayer::newMediaPlayer( bool video, bool muted )
{
    if ( m_player )
        delete m_player;

    if ( m_deviceMusicFile )
    {
        delete m_deviceMusicFile;
        m_deviceMusicFile = 0;
    }

    m_hasErrors = false;

    // Use Qt media player
    if ( video )
        m_player = new QMediaPlayer( 0, QMediaPlayer::VideoSurface );
    else
        m_player = new QMediaPlayer( 0 );

    connect( m_player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(slotError(QMediaPlayer::Error)) );
    connect( m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(slotMediaStatusChanged(QMediaPlayer::MediaStatus)) );

    if ( muted )
        m_player->setMuted( true );
}

#endif
