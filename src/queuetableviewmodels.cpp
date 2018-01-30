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

#include <QUrl>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMessageBox>

#include "util.h"
#include "songqueue.h"
#include "database.h"
#include "queuetableviewmodels.h"
#include "musiccollectionmanager.h"

TableModelQueue::TableModelQueue(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int TableModelQueue::rowCount(const QModelIndex &) const
{
    return m_queue.size();
}

int TableModelQueue::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant TableModelQueue::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role != Qt::DisplayRole)
        return QVariant();

    if ( orientation == Qt::Horizontal )
    {
        if ( section == 0 )
            return "Singer";
        else
            return "Song";
    }
    else
        return QString("%1").arg(section+1);
}

QVariant TableModelQueue::data(const QModelIndex &index, int role) const
{
    if ( index.row() >= 0 && index.row() < m_queue.size() && index.column() < 2 )
    {
        if ( role == Qt::DisplayRole )
        {
            if ( index.column() == 0 )
                return m_queue[index.row()].singer;
            else
                return QObject::tr( "\"%1\" by %2") .arg(m_queue[index.row()].title) .arg(m_queue[index.row()].artist);
        }
        else if ( role == Qt::UserRole )
        {
            // This result is used for searching
            return QString( "%1  %2 %3") .arg(m_queue[index.row()].title) .arg(m_queue[index.row()].artist) .arg(m_queue[index.row()].singer);
        }
        else if ( role == Qt::FontRole )
        {
            if ( index.row() == (int) m_currentItem )
            {
                QFont boldFont;
                boldFont.setBold(true);
                return boldFont;
            }
        }
    }

    return QVariant();
}

Qt::ItemFlags TableModelQueue::flags(const QModelIndex &) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool TableModelQueue::canDropMimeData(const QMimeData *data, Qt::DropAction, int, int, const QModelIndex &parent) const
{
    // Songs can be dropped anywhere
    if ( data->formats().contains( "application/x-karaoke-song" ) )
        return true;

    // But items - only between items (not on current items)
    if ( data->formats().contains( "application/x-karaoke-queue-item" ) )
    {
        return parent.isValid() ? false : true;
    }

    return false;
}

bool TableModelQueue::dropMimeData(const QMimeData *data, Qt::DropAction, int row, int, const QModelIndex &parent)
{
    // Drop can be one of the following:
    // - Into the empty space (parent is invalid, row=-1);
    // - Between existing items (parent is invalid, row=number);
    // - Over an existing item (parent.row is valid, row=-1);

    if ( data->formats().contains( "application/x-karaoke-queue-item" ) )
    {
        // We do not allow dropping items on items, so parent must be invalid
        Q_ASSERT( !parent.isValid() );

        int origrow = data->data( "application/x-karaoke-queue-item" ).toInt();

        // The target row is either valid (we must move there), or -1 (we must move to the end)
        if ( row == -1 )
            row = m_queue.size() - 1;

        if ( origrow == row )
            return false;

        // Move the item after the row xrow
        pSongQueue->moveSong( origrow, row );

        // Queue changed signal will trigger reimport
        return true;
    }
    else if ( data->formats().contains( "application/x-karaoke-song" ) )
    {
        // What's the item id?
        Database_SongInfo info;

        int songid = data->data( "application/x-karaoke-song" ).toInt();

        if ( !pDatabase->songById( data->data( "application/x-karaoke-song" ).toInt(), info ) )
            return false;

        if ( !parent.isValid() )
        {
            // Inserting the song
            if ( row == -1 )
                row = m_queue.size();

            pSongQueue->insertSong( row,  "", songid );
            return true;
        }

        if ( parent.row() == -1 )
            return false;

        // We're overwriting the current queued song; ask first
        if ( QMessageBox::question( 0,
                                    QObject::tr( "Replace the song?"),
                                    QObject::tr( "Replace the song %1 for singer %2")
                                        .arg( m_queue[parent.row()].title )
                                        .arg( m_queue[parent.row()].singer ) ) != QMessageBox::Yes )
            return false;

        pSongQueue->replaceSong( parent.row(), songid );
        return true;
    }
    else
        return false;
}

QMimeData *TableModelQueue::mimeData(const QModelIndexList &indexes) const
{
    QMimeData * data = new QMimeData();

    // Encode the item data - just the row #
    data->setData( "application/x-karaoke-queue-item", QByteArray::number( indexes[0].row() ) );
    return data;
}

QStringList TableModelQueue::mimeTypes() const
{
    return QStringList() << "application/x-karaoke-queue-item" << "application/x-karaoke-song";
}

void TableModelQueue::updateQueue()
{
    // Get the song list
    beginResetModel();
    m_queue = pSongQueue->exportQueue();
    m_currentItem = pSongQueue->currentItem();
    endResetModel();
}

QStringList TableModelQueue::singers()
{
    QStringList out;

    for ( int i = 0; i < m_queue.size(); i++ )
        if ( !m_queue[i].singer.isEmpty() && !out.contains(m_queue[i].singer ) )
            out.push_back( m_queue[i].singer );

    return out;
}

const SongQueueItem &TableModelQueue::itemAt(const QModelIndex &index)
{
    return m_queue[ index.row() ];
}


TableModelSearch::TableModelSearch(QObject *parent) : QAbstractTableModel(parent)
{
}

int TableModelSearch::rowCount(const QModelIndex &) const
{
    return m_results.size();
}

int TableModelSearch::columnCount(const QModelIndex &) const
{
    return 5;
}

QVariant TableModelSearch::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role != Qt::DisplayRole)
        return QVariant();

    if ( orientation == Qt::Horizontal )
    {
        switch ( section )
        {
        case 0:
            return QObject::tr("Artist");

        case 1:
            return QObject::tr("Title");

        case 2:
            return QObject::tr("Type");

        case 3:
            return QObject::tr("Rating");

        default:
            return QObject::tr("Langugage");
        }
    }

    return QVariant();
}

QVariant TableModelSearch::data(const QModelIndex &index, int role) const
{
    if ( index.row() >= 0 && index.row() < m_results.size() )
    {
        if ( role == Qt::DisplayRole )
        {
            switch ( index.column() )
            {
            case 0:
                return m_results[index.row()].artist;

            case 1:
                if ( m_results[index.row()].title.isEmpty() )
                    return m_results[index.row()].filePath;

                return m_results[index.row()].title;

            case 2:
                return m_results[index.row()].type;

            case 3:
                return QString::number( m_results[index.row()].rating );

            default:
                return m_results[index.row()].language;
            }
        }
        else if ( role == Qt::ToolTipRole )
        {
            return QObject::tr( "\"%1\" by %2") .arg( m_results[index.row()].title ) .arg( m_results[index.row()].artist );
        }

        // This is used in add/edit dialog
        if ( role == Qt::UserRole )
            return m_results[index.row()].id;
    }

    return QVariant();
}

Qt::ItemFlags TableModelSearch::flags(const QModelIndex &) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
}

struct ComparerSearchModel
{
    ComparerSearchModel( int column, bool ascending ) { m_column = column; m_ascending = ascending; }
    int m_column;
    bool m_ascending;

    inline bool operator() (const Database_SongInfo& q1, const Database_SongInfo& q2)
    {
        const Database_SongInfo& p1 = m_ascending ? q1 : q2;
        const Database_SongInfo& p2 = m_ascending ? q2 : q1;

        switch ( m_column )
        {
        case 0:
            return p1.artist < p2.artist;

        case 1:
            return p1.title < p2.title;

        case 2:
            return p1.rating < p2.rating;

        case 3:
            return p1.language < p2.language;

        default:
            abort();
        }
    }
};


void TableModelSearch::sort(int column, Qt::SortOrder order)
{
    beginResetModel();
    std::stable_sort( m_results.begin(), m_results.end(), ComparerSearchModel(column, order != Qt::AscendingOrder) );
    endResetModel();
}

QMimeData *TableModelSearch::mimeData(const QModelIndexList &indexes) const
{
    QMimeData * data = new QMimeData();

    data->setData( "application/x-karaoke-song", QByteArray::number( m_results[indexes[0].row()].id ) );
    return data;
}

QStringList TableModelSearch::mimeTypes() const
{
    return QStringList() << "application/x-karaoke-song";
}

const Database_SongInfo &TableModelSearch::infoAt(const QModelIndex &index) const
{
    return m_results[ index.row() ];
}

void TableModelSearch::performSearch(const QString &what)
{
    beginResetModel();

    if ( !pDatabase->search( what, m_results ) )
        m_results.clear();

    endResetModel();
}

TableModelMusic::TableModelMusic(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int TableModelMusic::rowCount(const QModelIndex &) const
{
    return m_collection.size();
}

int TableModelMusic::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant TableModelMusic::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role != Qt::DisplayRole)
        return QVariant();

    if ( orientation == Qt::Horizontal )
        return "Music file";
    else
        return QString("%1").arg(section);
}

QVariant TableModelMusic::data(const QModelIndex &index, int role) const
{
    if ( index.row() >= 0 && index.row() < m_collection.size() )
    {
        if ( role == Qt::DisplayRole )
        {
            // Trim down the file name
            if ( m_collection[ index.row()].length() > 60 )
                return QString("...%1").arg( m_collection[ index.row()].right( 60 ) );

            return m_collection[ index.row()];
        }
        else if ( role == Qt::ToolTipRole )
            return m_collection[ index.row()];
    }

    return QVariant();
}

Qt::ItemFlags TableModelMusic::flags(const QModelIndex &) const
{
    // We do not allow dropping ON items. However without Qt::ItemIsDropEnabled we cannot drop even between items.
    // Instead we disable it in canDrop.
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool TableModelMusic::canDropMimeData(const QMimeData * m, Qt::DropAction, int, int, const QModelIndex &parent) const
{
    // To drop, the drag type must be URL or application/x-music-queue-item, and must be dropped NOT on an item
    return (m->hasUrls() || m->hasFormat( "application/x-music-queue-item" )) && !parent.isValid();
}

bool TableModelMusic::dropMimeData(const QMimeData *data, Qt::DropAction, int row, int, const QModelIndex &parent)
{
    // Just another check - we do not allow to drop on existing items, only between them
    if ( parent.isValid() )
        return false;

    // Adjust row for the empty space
    if ( row == -1 )
        row = m_collection.size() - 1;

    // What is being dropped?
    if ( data->hasFormat( "application/x-music-queue-item" ) )
    {
        int orig = data->data( "application/x-music-queue-item" ).toInt();
        pMusicCollectionMgr->moveMusicItem( orig, row );
        return true;
    }
    else if ( data->hasUrls() )
    {
        // Just add the files, it will trigger rescan
        bool found = false;

        Q_FOREACH ( QUrl url, data->urls() )
        {
            QString filename = url.toLocalFile();

            // We only allow dropping music files, so its extension must be in the supportedExtensions
            if ( !pMusicCollectionMgr->supportedExtensions().contains( Util::fileExtension( filename )) )
                continue;

            // Insert the file at this row
            pMusicCollectionMgr->insertMusic( row, filename );
            found = true;
        }

        return found;
    }
    else
        return false;
}

QMimeData *TableModelMusic::mimeData(const QModelIndexList &indexes) const
{
    QMimeData * data = new QMimeData();

    // We represent the items in the collection as application/x-music-queue-item
    data->setData( "application/x-music-queue-item", QByteArray::number( indexes[0].row() ) );
    return data;
}

QStringList TableModelMusic::mimeTypes() const
{
    /*
    QStringList mimes;

    Q_FOREACH( const QString& ext, pMusicCollectionMgr->supportedExtensions() )
    {
        QString testfile = "file." + ext;
        mimes.push_back( m_mimeDatabase.mimeTypeForFile( testfile, QMimeDatabase::MatchExtension ).name() );
    }

    return mimes;
    */
    return QStringList() << "text/uri-list" << "application/x-music-queue-item";
}

void TableModelMusic::updateCollection()
{
    beginResetModel();
    pMusicCollectionMgr->exportCollection( m_collection );
    endResetModel();
}
