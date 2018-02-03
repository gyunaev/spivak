/**************************************************************************
 *  Spivak Karaoke PLayer - a free, cross-platform desktop karaoke player *
 *  Copyright (C) 2015-2018 George Yunaev, support@ulduzsoft.com          *
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

#ifndef SONGQUEUEITEMRETRIEVER_H
#define SONGQUEUEITEMRETRIEVER_H

#include <QList>
#include <QFile>
#include <QString>

#include "collectionprovider.h"

//
// This class is instantiated each time a retriever for the SongQueueItem
// is needed. It is not part of SongQueueItem because those items are frequently
// copied (i.e. when moved in the queue), while this object must remain.
class SongQueueItemRetriever : public QObject
{
    Q_OBJECT

    public:
        SongQueueItemRetriever();
        ~SongQueueItemRetriever();

        // Collection provider instance
        CollectionProvider  *   provider;

        // File where we write the output object
        QList<QIODevice*>       outputs;

        // Song sources which are to be retrieved,
        // and file names into which are they to be saved
        QStringList             sources;
        QStringList             outfiles;

        // During retrieve this keeps the current percentage (total)
        int                     percentage;

    private:
        Q_DISABLE_COPY(SongQueueItemRetriever)
};

#endif // SONGQUEUEITEMRETRIEVER_H
