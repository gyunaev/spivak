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

#ifndef QUEUETABLEVIEWMODELS_H
#define QUEUETABLEVIEWMODELS_H

#include <QAbstractTableModel>
#include <QMimeDatabase>

#include "database.h"
#include "songqueue.h"

class TableModelQueue : public QAbstractTableModel
{
    public:
        TableModelQueue(QObject * parent = 0);

        // Those must be overriden in a new model
        int rowCount(const QModelIndex & = QModelIndex()) const;
        int columnCount(const QModelIndex & = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &) const;

        // And those we oveerride to support drag'n'drop
        bool canDropMimeData(const QMimeData * data, Qt::DropAction, int, int, const QModelIndex & parent) const;
        bool dropMimeData(const QMimeData * data, Qt::DropAction, int row, int column, const QModelIndex & parent);
        QMimeData *	mimeData(const QModelIndexList & indexes) const;
        QStringList	mimeTypes() const;

        // This is called anytime the queue is updated, either by our action or by outside action
        void updateQueue();

        // Retrieves the list of singers
        QStringList singers();

        // Gets a copy of an item
        const SongQueueItem& itemAt( const QModelIndex & index );

    private:
        QList<SongQueueItem>    m_queue;
        unsigned int            m_currentItem;
};


class TableModelSearch : public QAbstractTableModel
{
    public:
        TableModelSearch(QObject * parent = 0);

        // Those must be overriden in a new model
        int rowCount(const QModelIndex & = QModelIndex()) const;
        int columnCount(const QModelIndex & = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &) const;

        // This model supports sorting
        void sort(int column, Qt::SortOrder order);

        // And those we oveerride to support drag (no drop here)
        QMimeData *	mimeData(const QModelIndexList & indexes) const;
        QStringList	mimeTypes() const;

        // Song result for the index
        const Database_SongInfo&  infoAt(const QModelIndex & index) const;

        // Called when a new search is performed, invalidates the model
        void performSearch( const QString& what );

    private:
        QList< Database_SongInfo > m_results;
};


class TableModelMusic : public QAbstractTableModel
{
    public:
        TableModelMusic(QObject * parent = 0);

        // Those must be overriden in a new model
        int rowCount(const QModelIndex & = QModelIndex()) const;
        int columnCount(const QModelIndex & = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &) const;

        // And those we oveerride to support drag'n'drop
        bool canDropMimeData(const QMimeData * data, Qt::DropAction, int, int, const QModelIndex & parent) const;
        bool dropMimeData(const QMimeData * data, Qt::DropAction, int row, int column, const QModelIndex & parent);
        QMimeData *	mimeData(const QModelIndexList & indexes) const;
        QStringList	mimeTypes() const;

        // This is called anytime the queue is updated, either by our action or by outside action
        void updateCollection();

    private:
        QStringList     m_collection;
};

#endif // QUEUETABLEVIEWMODELS_H
