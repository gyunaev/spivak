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

#include <QDir>
#include <QMap>
#include <QBuffer>
#include <QFileInfo>
#include <QThread>
#include <QDateTime>
#include <QApplication>

#include "logger.h"
#include "karaokeplayable.h"
#include "songdatabasescanner.h"
#include "database.h"
#include "playerlyricstext.h"
#include "pluginmanager.h"
#include "collectionprovider.h"
#include "eventor.h"
#include "util.h"


class SongDatabaseScannerWorkerThread : public QThread
{
    public:
        enum Type
        {
            THREAD_SCANNER,
            THREAD_PROCESSOR,
            THREAD_SUBMITTER,
            THREAD_CLEANUP
        };

        SongDatabaseScannerWorkerThread( SongDatabaseScanner * scanner, Type type ) : QThread( 0 )
        {
            m_type = type;
            m_scanner = scanner;
        }

        void run()
        {
            switch ( m_type )
            {
                case THREAD_SCANNER:
                    m_scanner->scanCollectionsThread();
                    break;

                case THREAD_PROCESSOR:
                    m_scanner->processingThread();
                    break;

                case THREAD_SUBMITTER:
                    m_scanner->submittingThread();
                    break;

                case THREAD_CLEANUP:
                    m_scanner->cleanupThread();
                    break;

                default:
                    abort();
            }
        }

    private:
        SongDatabaseScanner * m_scanner;
        Type                  m_type;
};



SongDatabaseScanner::SongDatabaseScanner(QObject *parent)
    : QObject(parent)
{
    m_langDetector = 0;
    m_providerStatus = -1;

    m_updateTimer.setInterval( 500 );
    m_updateTimer.setTimerType( Qt::CoarseTimer );

    connect( &m_updateTimer, SIGNAL(timeout()), this, SLOT(updateScanProgress() ) );
}

SongDatabaseScanner::~SongDatabaseScanner()
{
    if ( !m_threadPool.isEmpty() )
        stopScan();

    if ( m_langDetector )
        pPluginManager->releaseLanguageDetector();
}

bool SongDatabaseScanner::startScan()
{
    // Make a copy in case the settings change during scanning
    m_collection = pSettings->collections;

    // Do we need the language detector?
    bool need_lang_detector = false;

    for ( QMap<int,CollectionEntry>::const_iterator it = m_collection.begin();
          it != m_collection.end();
          ++it )
    {
        if ( it->detectLanguage )
            need_lang_detector = true;
    }

    if ( need_lang_detector )
    {
        m_langDetector = pPluginManager->loadLanguageDetector();

        if ( !m_langDetector )
        {
            Logger::debug( "Failed to load the language detection library, language detection is not available" );
            return false;
        }
    }

    // Create the directory scanner thread
    m_threadPool.push_back( new SongDatabaseScannerWorkerThread( this, SongDatabaseScannerWorkerThread::THREAD_SCANNER ) );

    // Create the submitter thread
    m_threadPool.push_back( new SongDatabaseScannerWorkerThread( this, SongDatabaseScannerWorkerThread::THREAD_SUBMITTER ) );

    // And two processor threads
    for ( int i = 0; i < 2; i++ )
    {
        m_threadPool.push_back( new SongDatabaseScannerWorkerThread( this, SongDatabaseScannerWorkerThread::THREAD_PROCESSOR ) );
        m_threadsRunning++;
    }

    // Add cleanup thread
    m_threadPool.push_back( new SongDatabaseScannerWorkerThread( this, SongDatabaseScannerWorkerThread::THREAD_CLEANUP ) );
    m_threadsRunning++;

    m_finishScanning = 0;

    // Start the update timer
    m_updateTimer.start();

    // Emit the Started signal
    emit pEventor->scanCollectionStarted();

    // And start all the threads
    Q_FOREACH( QThread * thread, m_threadPool )
        thread->start();

    return true;
}

void SongDatabaseScanner::stopScan()
{  
    // Signal all running threads to stop
    m_abortScanning = 1;
    m_finishScanning = 1;

    // Wait for all them and clean them up
    Q_FOREACH( QThread * thread, m_threadPool )
    {
        thread->wait();
        delete thread;
    }

    m_threadPool.clear();
}

void SongDatabaseScanner::updateScanProgress()
{
    // Stop the update timer if we're done
    if ( m_finishScanning )
        m_updateTimer.stop();

    QString progress = tr("Collection scan: %1 directories scanned, %2 karaoke files found, %3 processed, %4 submitted")
                                .arg( m_stat_directoriesScanned )
                                .arg( m_stat_karaokeFilesFound )
                                .arg( m_stat_karaokeFilesProcessed )
                                .arg( m_stat_karaokeFilesSubmitted );

    // If m_stringProgress is non-empty it overrides the progress
    emit pEventor->scanCollectionProgress( m_stringProgress.isEmpty() ? progress : m_stringProgress );
}

void SongDatabaseScanner::providerFinished(int, QString errmsg)
{
    if ( !errmsg.isEmpty() )
    {
        Logger::debug( "SongDatabaseScanner: couldn't retrieve the index file: %s", qPrintable( errmsg ) );
        m_providerStatus = 1;
        m_stringProgress = errmsg;
    }
    else
    {
        m_providerStatus = 0;
        m_stringProgress.clear();
    }
}

void SongDatabaseScanner::providerProgress(int id, int percentage)
{
    m_stringProgress = tr("Downloading index... %1%" ).arg( percentage );
}

void SongDatabaseScanner::scanCollectionsThread()
{
    qint64 lastupdate = pDatabase->lastDatabaseUpdate() * 1000;

    Logger::debug( "SongDatabaseScanner: scanCollectionsThread started" );

    if ( lastupdate > 0 )
        Logger::debug( "SongDatabaseScanner: collection thread will ignore the timestamps earlier than %s", qPrintable( QDateTime::fromMSecsSinceEpoch( lastupdate ).toString( "yyyy-mm-dd hh:mm:ss") ) );

    for ( QMap<int,CollectionEntry>::const_iterator it = m_collection.begin();
          it != m_collection.end();
          ++it )
    {
        if ( m_finishScanning != 0 )
            break;

        m_stringProgress.clear();
        Logger::debug( "SongDatabaseScanner: scanning collection %s", qPrintable( it->name ) );

        // Find the collection provider for this type
        CollectionProvider * provider = CollectionProvider::createProviderForID( it->id );

        if ( !provider )
            continue;

        connect( provider, SIGNAL(finished(int,QString)), this, SLOT(providerFinished(int,QString)) );
        connect( provider, SIGNAL(progress(int,int)), this, SLOT(providerProgress(int,int)) );

        m_providerStatus = -1;

        // Download the index file into this array
        QByteArray indexdata;
        QBuffer buf( &indexdata );
        buf.open( QIODevice::WriteOnly );

        // Initiate the download and wait until finished signal is issued
        provider->download( 1, it->rootPath + "/index.dat", &buf );

        // Some providers are always synchronous so check it first
        while ( m_providerStatus == -1 )
        {
            qApp->processEvents( QEventLoop::ExcludeUserInputEvents, 500 );
        }

        // If for this collection we already have the index file, just use it
        if ( m_providerStatus == 0 )
        {
            Logger::debug( "SongDatabaseScanner: successfully downloaded the collection index");

            // Parse the index file and send it directly into submission thread
            parseCollectionIndex( *it, indexdata );
            delete provider;
            continue;
        }

        // Not so much luck, now we need to proceed with file system enumeration
        // and this only makes sense with local collection
        if ( !provider->isLocalProvider() )
        {
            delete provider;
            Logger::error( "Skipped collection %s: provider is not local and no index found",
                           qPrintable( it->name) );
            continue;
        }

        // We do not use recursion, and use the queue-like list instead
        QStringList paths;
        paths << it->rootPath;

        // And enumerate all the paths
        while ( !paths.isEmpty() )
        {
            m_stat_directoriesScanned++;
            QString current = paths.takeFirst();
            QFileInfo finfo( current );

            if ( m_finishScanning != 0 )
                break;

            // Skip non-existing paths (might come from settings)
            if ( !finfo.exists() )
                continue;

            // We assume the situation where we have several lyrics for a single music is more prevalent
            QMap< QString, QString > musicFiles, lyricFiles;
            bool modtime_checked = true;

            Q_FOREACH( QFileInfo fi, QDir( current ).entryInfoList( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst ) )
            {
                // Re-add directories into the list
                if ( fi.isDir() )
                {
                    paths.push_back( fi.absoluteFilePath() );
                    continue;
                }

                // If we're here this means the directory also has files. Those we will only check for if the parent directory
                // has been modified after the last update
                //FIXME! thisdoesn't seem right
                if ( !modtime_checked )
                {
                    if ( lastupdate > 0 && QFileInfo( current ).lastModified().toMSecsSinceEpoch() < lastupdate )
                        break;

                    modtime_checked = true;
                }

                // No more directories in this dir, but we have files
                if ( KaraokePlayable::isSupportedCompleteFile( fi.fileName()) )
                {
                    // We only add zip files if enabled for this collection
                    if ( fi.fileName().endsWith( ".zip", Qt::CaseInsensitive) && !it->scanZips )
                        continue;

                    // Schedule for processing right away
                    SongDatabaseEntry entry;
                    entry.filePath = fi.absoluteFilePath();
                    entry.colidx = it->id;
                    entry.language = it->defaultLanguage;

                    addProcessing( entry );
                }
                else if ( KaraokePlayable::isSupportedMusicFile( fi.fileName()) )
                {
                    // Music is mapped basename -> file
                    musicFiles[ fi.baseName() ] = fi.fileName();
                }
                else if ( KaraokePlayable::isSupportedLyricFile( fi.fileName()) )
                {
                    // But lyric is mapped file -> basename
                    lyricFiles[ fi.fileName() ] = fi.baseName();
                }
                else
                    Logger::debug( "SongDatabaseScanner: unknown file %s, skipping", qPrintable( fi.absoluteFilePath() ) );
            }

            // Try to see if we have all matched music-lyric files - and move them to completeFiles
            while ( !lyricFiles.isEmpty() )
            {
                QString lyric = lyricFiles.firstKey();
                QString lyricbase = lyricFiles.take( lyric );

                if ( musicFiles.contains( lyricbase ) )
                {
                    // And we have a complete song
                    SongDatabaseEntry entry;
                    entry.filePath = current + Util::separator() + lyric;
                    entry.musicPath = musicFiles[ lyricbase ];
                    entry.colidx = it->id;
                    entry.language = it->defaultLanguage;

                    addProcessing( entry );
                }
                else
                    Logger::debug( "SongDatabaseScanner: WARNING no music found for lyric file %s", qPrintable( current + Util::separator() + lyric) );
            }
        }
    }

    // All done - put an entry with an empty path and wake all threads
    addProcessing( SongDatabaseEntry() );
    m_processingQueueCond.wakeAll();

    Logger::debug( "SongDatabaseScanner: scanCollectionsThread finished" );
}

void SongDatabaseScanner::cleanupThread()
{
    Logger::debug( "SongDatabaseScanner: cleanup thread started" );

    if ( !pDatabase->cleanupCollections() )
        Logger::error( "SongDatabaseScanner: database cleanup failed with error" );

    // If m_threadsRunning was 1 when this thread finished, this is the last one before the submitting thread
    if ( m_threadsRunning.fetchAndAddAcquire( -1 ) == 1 )
    {
        // Signal that no more data would be submitted - shut down
        Logger::debug( "SongDatabaseScanner: all procesing threads are finished, and cleanup is finished too, signaling the submitter to shut down" );
        m_finishScanning = 1;

        // In case the submitter is waiting in condition
        m_submittingQueueCond.wakeAll();
    }
    else
        Logger::debug( "SongDatabaseScanner: cleanup thread finished" );
}

void SongDatabaseScanner::processingThread()
{
    Logger::debug( "SongDatabaseScanner: procesing thread started" );

    while ( m_finishScanning == 0 )
    {
        // Wait until there are more entries in processing queue
        m_processingQueueMutex.lock();

        // Someone else might have taken our item
        if ( m_processingQueue.isEmpty() )
        {
            m_processingQueueCond.wait( &m_processingQueueMutex );

            // Since we're woken up here, our mutex is now locked
            if ( m_processingQueue.isEmpty() )
            {
                m_processingQueueMutex.unlock();
                continue;
            }
        }

        // An entry with empty path means no more entries (and we do not take it from queue to let other threads see it)
        if ( m_finishScanning || m_processingQueue.first().filePath.isEmpty() )
        {
            m_processingQueueMutex.unlock();
            break;
        }

        // Take our item and let others do work
        SongDatabaseEntry entry = m_processingQueue.takeFirst();
        m_processingQueueMutex.unlock();

        m_stat_karaokeFilesProcessed++;

        // Query the database to see what, if anything we already have for this path
        Database_SongInfo info;

        if ( pDatabase->songByPath( entry.filePath, info ) )
        {
            // We have the song, does it have all the information?
            if ( !info.artist.isEmpty() && !info.title.isEmpty() && !info.type.isEmpty() && info.language != 0 )
            {
                // Is it up-to-date?
                if ( QFileInfo(entry.filePath).lastModified() <= QDateTime::fromMSecsSinceEpoch( info.added * 1000LL ) )
                {
                    Logger::debug( "SongDatabaseScanner: file %s has all the info and is up-to-date, skipped", qPrintable(entry.filePath) );
                    continue;
                }

                Logger::debug( "SongDatabaseScanner: file %s has all the info but is not up-to-date", qPrintable(entry.filePath) );
            }
        }

        // Detecting the lyrics type
        int p = entry.filePath.lastIndexOf( '.' );

        if ( p == -1 )
            continue;

        // Just use the uppercase extension
        entry.type = entry.filePath.mid( p + 1 ).toUpper();

        // Read the karaoke file unless it is a video file
        if ( !KaraokePlayable::isVideoFile( entry.filePath ) )
        {
            QScopedPointer<KaraokePlayable> karaoke( KaraokePlayable::create( entry.filePath ) );

            if ( !karaoke || !karaoke->parse() )
            {
                Logger::debug( "SongDatabaseScanner: WARNING ignoring file %s as it cannot be parsed", qPrintable(entry.filePath) );
                continue;
            }

            // For non-CDG files we can do the artist/title and language detection from source
            if ( !karaoke->lyricObject().endsWith( ".cdg", Qt::CaseInsensitive ) && m_collection[entry.colidx].detectLanguage && m_langDetector )
            {
                // Get the pointer to the device the lyrics are stored in (will be deleted after the pointer out-of-scoped)
                QScopedPointer<QIODevice> lyricDevice( karaoke->openObject( karaoke->lyricObject() ) );

                if ( lyricDevice == 0 )
                {
                    Logger::debug( "SongDatabaseScanner: WARNING cannot open lyric file %s in karaoke file %s", qPrintable( karaoke->lyricObject() ), qPrintable(entry.filePath) );
                    continue;
                }

                QScopedPointer<PlayerLyricsText> lyrics( new PlayerLyricsText( "", "" ) );

                if ( !lyrics->load( lyricDevice.data(), karaoke->lyricObject() ) )
                {
                    if ( karaoke->lyricObject() != entry.filePath )
                        Logger::debug( "SongDatabaseScanner: karaoke file %s contains invalid lyrics %s", qPrintable(entry.filePath), qPrintable( karaoke->lyricObject() ) );
                    else
                        Logger::debug( "SongDatabaseScanner: lyrics file %s cannot be loaded", qPrintable(entry.filePath) );

                    continue;
                }

                // Detect the language
                QString lyricsText = lyrics->exportAsText();
                entry.language = m_langDetector->detectLanguage( lyricsText.toUtf8() );

                // Fill up the artist/title if we detected them
                if ( lyrics->properties().contains( LyricsLoader::PROP_ARTIST ) )
                    entry.artist = lyrics->properties()[ LyricsLoader::PROP_ARTIST ];

                if ( lyrics->properties().contains( LyricsLoader::PROP_TITLE ) )
                    entry.title = lyrics->properties()[ LyricsLoader::PROP_TITLE ];

                if ( lyrics->properties().contains( LyricsLoader::PROP_LYRIC_SOURCE ) )
                    entry.type += "/" + lyrics->properties()[ LyricsLoader::PROP_LYRIC_SOURCE ];
            }
        }

        // Get the artist/title from path according to settings
        if ( !guessArtistandTitle( entry.filePath, m_collection[entry.colidx].artistTitleSeparator, entry.artist, entry.title ) )
        {
            Logger::debug( "SongDatabaseScanner: failed to detect artist/title for file %s, skipped", qPrintable(entry.filePath) );
            continue;
        }

        Logger::debug( "SongDatabaseScanner: added/updated song %s by %s, type %s, language %s, file %s",
                       qPrintable(entry.title),
                       qPrintable(entry.artist),
                       qPrintable(entry.type),
                       entry.language.isEmpty() ? "---" : qPrintable(entry.language),
                       qPrintable(entry.filePath) );

        // entry.colidx has different meaning in the database - fix it before adding
        entry.colidx = m_collection[ entry.colidx ].id;

        // and prepare for submission
        addSubmitting( entry );
    }

    // If m_threadsRunning was 1 when this thread finished, this is the last one before the submitting thread
    if ( m_threadsRunning.fetchAndAddAcquire( -1 ) == 1 )
    {
        // Signal that no more data would be submitted - shut down
        Logger::debug( "SongDatabaseScanner: last procesing thread finished, signaling the submitter to shut down" );
        m_finishScanning = 1;

        // In case the submitter is waiting in condition
        m_submittingQueueCond.wakeAll();
    }
    else
        Logger::debug( "SongDatabaseScanner: procesing thread finished" );
}

void SongDatabaseScanner::submittingThread()
{
    const int ENTRIES_TO_UPDATE = 200;

    Logger::debug( "SongDatabaseScanner: submitting thread started" );

    while ( m_finishScanning == 0 )
    {
        // Wait until there are more entries in processing queue
        m_submittingQueueMutex.lock();

        // Someone else might have taken our item
        if ( m_submittingQueue.size() >= ENTRIES_TO_UPDATE  )
        {
            // We make a copy of current queue (still locked), clear and unlock it - this ensures
            // all processing threads do not stop while we're updating the database.
            // Due to Qt's copy-on-write this doesn't result in such huge waste of memory
            QList<SongDatabaseEntry> copy = m_submittingQueue;
            m_submittingQueue.clear();
            m_submittingQueueMutex.unlock();

            // Now update at our own pace
            pDatabase->updateDatabase( copy );

            // and straight away into the loop (no falling through into wait, mutex is not locked)
            continue;
        }

        m_submittingQueueCond.wait( &m_submittingQueueMutex );
        m_submittingQueueMutex.unlock();
    }

    // Submit the rest, if any
    if ( !m_submittingQueue.isEmpty() )
        pDatabase->updateDatabase( m_submittingQueue );

    if ( !m_abortScanning )
    {
        pDatabase->updateLastScan();
        pDatabase->getDatabaseCurrentState();
        emit pEventor->scanCollectionFinished();

        Logger::debug( "SongDatabaseScanner: submitter thread finished, scan completed" );
    }
    else
        Logger::debug( "SongDatabaseScanner: submitter thread finished, scan aborted" );
}

void SongDatabaseScanner::parseCollectionIndex( const CollectionEntry& col, const QByteArray &indexdata)
{
    // Index file is a simple vertical dash-separated text file in UTF8, containing per each line:
    // <artist> <title> <filepathfromroot> <musicpathifneeded> <type> [language]
    // filepathfromroot must contain path of lyrics (for music+lyric file)
    // or the complete file (if video or zip). In former case musicpathifneeded
    // should contain the music file, otherwise it should be empty.
    QStringList entries = QString::fromUtf8( indexdata ).split( "\n" );

    Q_FOREACH( const QString& entry, entries )
    {
        // Skip empty lines
        if ( entry.trimmed().isEmpty() )
            continue;

        // Parse it
        QStringList values = entry.split( '|' );

        if ( values.size() < 5 )
        {
            Logger::error( "SongDatabaseScanner: Invalid line in the collection index: %s",
                           qPrintable( entry) );
            continue;
        }

        SongDatabaseEntry dbe;
        dbe.colidx = col.id;
        dbe.artist = values[0];
        dbe.title = values[1];
        dbe.filePath = col.rootPath + "/" + values[2];

        if ( !values[3].isEmpty() )
            dbe.musicPath = col.rootPath + "/" + values[3];

        dbe.type = values[4];

        if ( values.size() > 5 )
            dbe.language = values[5];

        addSubmitting( dbe );
    }

    Logger::error( "SongDatabaseScanner: added %d entries via index file", entries.size() );
}

void SongDatabaseScanner::addProcessing(const SongDatabaseScanner::SongDatabaseEntry &entry)
{
    m_stat_karaokeFilesFound++;
    m_processingQueueMutex.lock();
    m_processingQueue.push_back( entry );
    m_processingQueueMutex.unlock();
    m_processingQueueCond.wakeOne();
}

void SongDatabaseScanner::addSubmitting(const SongDatabaseScanner::SongDatabaseEntry &entry)
{
    m_stat_karaokeFilesSubmitted++;
    m_submittingQueueMutex.lock();
    m_submittingQueue.push_back( entry );
    m_submittingQueueMutex.unlock();
    m_submittingQueueCond.wakeOne();
}

bool SongDatabaseScanner::guessArtistandTitle( const QString& filePath, const QString& separator, QString& artist, QString& title )
{
    if ( separator != "/" )
    {
        int p = filePath.lastIndexOf( Util::separator() );

        if ( p != -1 )
            artist = filePath.mid( p + 1 ); // has path component
        else
            artist = filePath; // no path component

        p = artist.indexOf( separator );

        if ( p == -1 )
            return false;

        title = artist.mid( p + separator.length() );
        artist = artist.left( p );
    }
    else
    {
        // Find out which separator we use
        QChar separator = '/';

        int p = filePath.lastIndexOf( separator );

        if ( p == -1 )
        {
            separator = '\\';

            if ( (p = filePath.lastIndexOf( separator )) == -1 )
                return false;
        }

        title = filePath.mid( p + 1 );
        artist = filePath.left( p );

        // Trim down the artist if needed - may not be there (i.e. "Adriano Celentano/Le tempo.mp3")
        if ( (p = artist.lastIndexOf( separator )) != -1 )
            artist = artist.mid( p + 1 );

        // Trim the extension from title
        if ( (p = title.lastIndexOf( '.' )) != -1 )
            title = title.left( p );
    }

    return true;
}

