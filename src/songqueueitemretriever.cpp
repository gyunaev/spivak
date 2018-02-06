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

#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <QStandardPaths>

#include "util.h"
#include "songqueueitemretriever.h"

SongQueueItemRetriever::SongQueueItemRetriever()
    : QObject(0)
{
    provider = 0;
    collectionindex = 0;
    collectiontype = CollectionProvider::TYPE_INVALID;
}

SongQueueItemRetriever::~SongQueueItemRetriever()
{
    Q_FOREACH ( QIODevice * f, outputs )
    {
        delete f;
    }

    if ( provider )
        provider->deleteLater();
}

QString SongQueueItemRetriever::cachedFileName(const QString &source) const
{
    // We use the first source base name for the cache
    QString basename = QCryptographicHash::hash( sources.front().toUtf8(), QCryptographicHash::Md5 ).toHex();

    // Get the file extension
    QString ext = Util::fileExtension( source );

    return QStandardPaths::writableLocation( QStandardPaths::CacheLocation )
        + QDir::separator()
        + basename + "." + ext;
}
