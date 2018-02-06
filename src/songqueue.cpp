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
#include <QFile>
#include <QDataStream>
#include <QStringList>
#include <QFileInfo>
#include <QStandardPaths>

#include "logger.h"
#include "settings.h"
#include "songqueue.h"
#include "eventor.h"
#include "karaokesong.h"
#include "actionhandler.h"
#include "database.h"
#include "songqueueitemretriever.h"
#include "collectionprovider.h"

SongQueue * pSongQueue;

class SongQueueEntry
{
    public:
        SongQueueItem   item;

        // If the song needs to be retrieved, this field is non-null. Otherwise null.
        SongQueueItemRetriever * retriever;

        SongQueueEntry( const SongQueueItem& i ) : item(i)
        {
            retriever = 0;
        }

        ~SongQueueEntry()
        {
            delete retriever;
        }
};


SongQueue::SongQueue(QObject *parent)
    : QObject( parent )
{
    m_currentSong = 0;
    m_nextQueueId = 1;
    m_beingRetrieved = 0;
}

SongQueue::~SongQueue()
{
    if ( !pSettings->queueFilename.isEmpty() && !pSettings->queueSaveOnExit )
        QFile::remove( pSettings->queueFilename );
}

void SongQueue::init()
{
    if ( !pSettings->queueFilename.isEmpty() )
        load();
}

void SongQueue::addSong( SongQueueItem song )
{
    addSong( song, 0 );
}

void SongQueue::addSong( SongQueueItem song, SongQueueItemRetriever * retriever )
{
    // First verify the sanity
    if ( song.file.isEmpty() && !retriever )
    {
        Logger::error( "Attempting to add a non-local song with empty source or provider" );
        return;
    }

    // Add the singer into the queue if he's not there, and find the current singer (who's singing the current song)
    int singeridx = addSinger( song.singer );

    // Add to the end now
    song.id = m_nextQueueId++;

    // Assume it to be ready
    song.state = SongQueueItem::STATE_READY;

    // Find out if it a local song or requires retrieval through provider
    if ( retriever )
    {
        // See if all song files are present
        Q_FOREACH( const QString& src, retriever->sources )
        {
            QString srcfile = retriever->cachedFileName( src );

            if ( song.file.isEmpty() )
                song.file = srcfile;

            if ( !QFile::exists( srcfile ) )
            {
                // File does not exist - so we need to retrieve it
                Logger::debug( "SongQueue: cached file %s for song %d does not exist locally", qPrintable(srcfile), song.id );

                song.state = SongQueueItem::STATE_NOT_READY;
                break;
            }
        }
    }

    // Add to queue
    SongQueueEntry * entry = new SongQueueEntry( song );

    // If the song status is ready, we do not need retriever anymore
    if ( song.state == SongQueueItem::STATE_READY && retriever )
        retriever->deleteLater();
    else
        entry->retriever = retriever;

    m_queue.append( entry );

    // Now we have to rearrange the songs from current to the end according to the singer order
    singeridx++;

    if ( singeridx >= m_singers.size() )
        singeridx = 0;

    int songidx = m_currentSong + 1;
    Logger::debug( "Queue: Adding file %d %s by %s into queue id %d: %s", song.songid, qPrintable(song.file), qPrintable(song.singer), song.id, qPrintable( song.stateText() ) );
/*
    for ( int i = 0; i < m_singers.size(); i++ )
        qDebug() << "Singer " << i << ": " << m_singers[i];

    for ( int i = 0; i < m_queue.size(); i++ )
        qDebug() << "Queue " << i << ": " << m_queue[i].file << " " << m_queue[i].singer;
*/

    // We iterate through the list of songs in queue and ensure the order of songs in queue matches
    // the order of singers. We only do this if there is more than one singer in queue.
    if ( m_singers.size() > 1 )
    {
        while ( songidx < m_queue.size() )
        {
            // This song should be sung by singeridx
            qDebug("%d: should be next singer %s", songidx, qPrintable( m_singers[singeridx] ));

            // Is the next song already sang by the next supposed singer?
            if ( m_queue[songidx]->item.singer != m_singers[singeridx] )
            {
                // Find the next song with the next supposed singer
                int i = songidx + 1;

                for ( ; i < m_queue.size(); i++ )
                    if ( m_queue[i]->item.singer == m_singers[singeridx] )
                        break;

                // If there is a song with this singer, swap them and move to next song
                if ( i < m_queue.size() )
                {
                    // Yes; swap with the current one, and move on
                    m_queue.swap( songidx, i );
                    songidx++;
                }
                else
                {
                    // If there are no songs in queue with this singer (i.e. Joe is supposed to sing next,
                    // but there is no song queued for Joe), just switch to next singer down below.
                }
            }
            else
            {
                // Yes, this item is already in place, move to next
                songidx++;
            }

            // Switch to next singer
            singeridx++;

            if ( singeridx >= m_singers.size() )
                singeridx = 0;
        }
    }

/*
    // Dump the queue
    qDebug() << "Queue after adding song" << file << "by" << singer;

    for ( int i = 0; i < m_queue.size(); i++ )
        qDebug() << "Queue " << i << ": " << m_queue[i].file << " " << m_queue[i].singer;
*/
    queueUpdated();
}

void SongQueue::addSong( const QString &singer, int id )
{
    Database_SongInfo info;

    if ( !pDatabase->songById( id, info ) )
        return;

    SongQueueItem song;
    SongQueueItemRetriever * retriever = 0;

    song.artist = info.artist;
    song.file = info.filePath;
    song.songid = id;
    song.singer = singer;
    song.title = info.title;

    // Check the collection type
    if ( pSettings->collections.contains(info.collectionid) )
    {
        if ( pSettings->collections[info.collectionid].type != CollectionProvider::TYPE_FILESYSTEM )
        {
            // Create the provider
            retriever = new SongQueueItemRetriever();
            retriever->collectionindex = info.collectionid;
            retriever->sources = info.filePath.split('|');

            // Clear the file (so it is reinitialized to cached) and create the sources
            song.file.clear();
        }
    }

    addSong( song, retriever );
}

void SongQueue::addSong(const QString &singer, const QString &file)
{
    SongQueueItem song;

    song.file = file;
    song.songid = 0;
    song.singer = singer;

    addSong( song );
}

void SongQueue::insertSong(unsigned int position, const QString &singer, int songid)
{
    Database_SongInfo info;

    if ( !pDatabase->songById( songid, info ) )
        return;

    SongQueueItem song;

    song.id = m_nextQueueId++;
    song.artist = info.artist;
    song.file = info.filePath;
    song.songid = songid;
    song.singer = singer;
    song.title = info.title;

    // Song queue is exported from m_currentSong, so we need to adjust the items
    m_queue.insert( position, new SongQueueEntry( song ) );

    queueUpdated();
}

void SongQueue::replaceSong(unsigned int position, int songid)
{
    Database_SongInfo info;

    if ( !pDatabase->songById( songid, info ) )
        return;

    m_queue[ position ]->item.artist = info.artist;
    m_queue[ position ]->item.title = info.title;
    m_queue[ position ]->item.file = info.filePath;
    m_queue[ position ]->item.songid = info.id;

    queueUpdated();
}

void SongQueue::replaceSong(unsigned int position, const SongQueueItem &song)
{
    m_queue[ position ]->item.artist = song.artist;
    m_queue[ position ]->item.title = song.title;
    m_queue[ position ]->item.file = song.file;
    m_queue[ position ]->item.songid = song.songid;

    // This might replace the singer, in which case we must add another singer
    if ( m_queue[ position ]->item.singer != song.singer )
        addSinger( song.singer );

    m_queue[ position ]->item.singer = song.singer;
    queueUpdated();
}

void SongQueue::removeSong(unsigned int position)
{
    // Song queue is exported from m_currentSong, so we need to adjust the items
    delete m_queue[position];
    m_queue.removeAt( position );

    queueUpdated();
}

bool SongQueue::removeSongById(int id)
{
    for ( int i = 0; i < m_queue.size(); i++ )
    {
        if ( m_queue[i]->item.id == id )
        {
            delete m_queue[i];
            m_queue.removeAt( i );
            queueUpdated();

            return i == (int) m_currentSong;
        }
    }

    return false;
}

void SongQueue::moveSong(unsigned int from, unsigned int to)
{
    // Song queue is exported from m_currentSong, so we need to adjust the items
    m_queue.move( from, to );

    queueUpdated();
}

int SongQueue::addSinger(const QString &singer)
{
    int singeridx = 0;

    if ( !m_queue.isEmpty() && (int) m_currentSong < m_queue.size() )
        singeridx = qMax( m_singers.indexOf( m_queue[m_currentSong]->item.singer ), 0 );

    if ( m_singers.indexOf( singer ) == -1 )
    {
        if ( pSettings->queueAddNewSingersNext )
            m_singers.insert( singeridx + 1, singer );
        else
            m_singers.append( singer );
    }

    return singeridx;
}

SongQueueItem SongQueue::current() const
{
    return m_queue[ m_currentSong ]->item;
}

bool SongQueue::prev()
{
    if ( m_currentSong == 0  )
        return false;

    m_currentSong--;
    queueUpdated();
    return true;
}

bool SongQueue::next()
{
    if ( (int) m_currentSong >= m_queue.size() )
        return false;

    m_currentSong++;
    queueUpdated();

    return (int) m_currentSong < m_queue.size();
}

QList<SongQueueItem> SongQueue::exportQueue() const
{
    QList<SongQueueItem> output;

    for ( int i = 0; i < m_queue.size(); i++ )
        output.push_back( m_queue[i]->item );

    return output;
}

unsigned int SongQueue::currentItem() const
{
    return m_currentSong;
}

bool SongQueue::isEmpty() const
{
    return (int) m_currentSong == m_queue.size() || m_queue.isEmpty();
}

void SongQueue::clear()
{
    m_queue.clear();
    m_singers.clear();
    m_currentSong = 0;

    queueUpdated();
}

void SongQueue::statusChanged(int id, bool playstarted)
{
    // Find the file in queue - we go through the entire loop in case a file is added more than once
    for ( int i = 0; i < m_queue.size(); i++ )
    {
        if ( m_queue[i]->item.id == id )
        {
            m_queue[i]->item.state = playstarted ? SongQueueItem::STATE_PLAYING : SongQueueItem::STATE_READY;
            break;
        }
    }

    queueUpdated();
}

void SongQueue::queueUpdated()
{
    // Save the queue if needed
    if ( !pSettings->queueFilename.isEmpty() )
        save();

    // Check if there are any songs which need to be retrieved
    if ( m_beingRetrieved < pSettings->queueMaxConcurrentPrepare )
    {
        for ( int i = m_currentSong; i < m_queue.size(); i++ )
        {
            if ( m_queue[i]->item.state == SongQueueItem::STATE_NOT_READY )
            {
                // Maximum is not reached - start retrieving
                startRetrieve( i );

                // If we are already at maximum, no more downloading
                if ( m_beingRetrieved >= pSettings->queueMaxConcurrentPrepare )
                    break;
            }
        }
    }
    else
        Logger::debug( "SongQueue: song needs to be retrieved but max retrievers is reached, ignoring" );

    emit pEventor->queueChanged();
}

void SongQueue::save()
{
    QFile fout( pSettings->queueFilename );

    if ( !fout.open( QIODevice::WriteOnly ) )
    {
        pActionHandler->error( "Cannot store queue: " + fout.errorString() );
        return;
    }

    QDataStream dts( &fout );
    dts << QString("KARAOKEQUEUE");
    dts << (int) 2;
    dts << m_currentSong;
    dts << m_nextQueueId;
    dts << m_singers;
    dts << m_queue.size();

    for ( int i = 0; i < m_queue.size(); i++ )
    {
        // Ignore the non-local songs
        if ( m_queue[i]->retriever )
            continue;

        dts << m_queue[i]->item.id;
        dts << m_queue[i]->item.songid;
        dts << m_queue[i]->item.file;
        dts << m_queue[i]->item.artist;
        dts << m_queue[i]->item.title;
        dts << m_queue[i]->item.singer;
    }
}

void SongQueue::load()
{
    QFile fout( pSettings->queueFilename );

    if ( !fout.open( QIODevice::ReadOnly ) )
        return;

    QDataStream dts( &fout );
    QString header;
    int version;

    dts >> header;

    if ( header != "KARAOKEQUEUE" )
        return;

    dts >> version;

    if ( version != 2 )
        return;

    dts >> m_currentSong;
    dts >> m_nextQueueId;
    dts >> m_singers;
    dts >> version; // now queue size

    for ( int i = 0; i < version; i++ )
    {
        SongQueueItem s;

        dts >> s.id;
        dts >> s.songid;
        dts >> s.file;
        dts >> s.artist;
        dts >> s.title;
        dts >> s.singer;

        s.state = SongQueueItem::STATE_READY;

        m_queue.append( new SongQueueEntry( s ) );
    }

    if ( !m_queue.isEmpty() )
        emit pEventor->queueChanged();
}

bool SongQueue::startRetrieve( int idx )
{
    SongQueueEntry * entry = m_queue[idx];

    Logger::debug( "SongQueue: start retrieving song id %d from %s", idx, qPrintable(entry->retriever->sources.join(", ")) );

    // Create the provider
    if ( entry->retriever->collectionindex >= 0 )
        entry->retriever->provider = CollectionProvider::createProviderForID( entry->retriever->collectionindex );
    else
        entry->retriever->provider = CollectionProvider::createProviderForType( entry->retriever->collectiontype );

    if ( entry->retriever->provider == 0 )
    {
        Logger::error( "SongQueue: cannot start retrieving song from %s: no provider", qPrintable( entry->retriever->sources.join(", ")) );
        removeSong( idx );
        return false;
    }

    // Connect the signals
    connect( entry->retriever->provider, SIGNAL(finished(int,QString)), this, SLOT(providerFinished(int,QString)) );
    connect( entry->retriever->provider, SIGNAL(progress(int,int)), this, SLOT(providerProgress(int,int)) );

    QStringList urls;

    // Add downloaders for missing sources
    Q_FOREACH( QString src, entry->retriever->sources )
    {
        QString srcfile = entry->retriever->cachedFileName( src );

        if ( !QFile::exists( srcfile ) )
        {
            QFile * newfile = new QFile( srcfile );

            entry->retriever->outputs.push_back( newfile );
            entry->retriever->outfiles.push_back( srcfile );

            if ( !newfile->open( QIODevice::WriteOnly ) )
            {
                Logger::error( "SongQueue: cannot start retrieving from %s: cannot write into cached file %s", qPrintable( src ), qPrintable( srcfile ) );
                removeSong( idx );
                return false;
            }

            Logger::debug( "SongQueue: setting up to retrieve %s into %s", qPrintable( src ), qPrintable( srcfile ) );

            urls.push_back( src.replace("#", "%23") );
        }
        else
            Logger::debug( "SongQueue: not retrieving %s as %s already present", qPrintable( src ), qPrintable( srcfile ) );
    }

    // Start the download
    entry->retriever->provider->downloadAll( entry->item.id, urls, entry->retriever->outputs );

    // And store it in the map
    m_beingRetrieved++;

    entry->item.state = SongQueueItem::STATE_GETTING_READY;
    return true;
}

void SongQueue::providerFinished(int id, const QString &error)
{
    // Non-empty error means we failed
    Logger::debug( "SongQueue: Queue id %d finished downloading: %s",
                   id, error.isEmpty() ? "success" : qPrintable( error ) );

    // Find the file in queue
    for ( int i = 0; i < m_queue.size(); i++ )
    {
        if ( m_queue[i]->item.id == id )
        {
            if ( !error.isEmpty() )
            {
                // Downloaded with an error. Remove all the cached files
                Q_FOREACH( const QString& src, m_queue[i]->retriever->outfiles )
                {
                    QFile::remove( src );
                }

                // Since it failed, we delete the entry from the queue
                m_queue.removeAt( i );
            }
            else
            {
                // We do not need the retriever anymore
                delete m_queue[i]->retriever;
                m_queue[i]->retriever = 0;

                // We're ready to play
                m_queue[i]->item.state = SongQueueItem::STATE_READY;
            }

            break;
        }
    }

    m_beingRetrieved--;
    queueUpdated();
}

void SongQueue::providerProgress(int id, int progress)
{
    Logger::debug( "SongQueue: Progress received for queue id %d: %d%%", id, progress );

    for ( int i = 0; i < m_queue.size(); i++ )
    {
        if ( m_queue[i]->item.id == id )
        {
            m_queue[i]->item.readiness = progress;
            break;
        }
    }
}
