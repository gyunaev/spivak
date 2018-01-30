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

#include "songqueueitem.h"
#include "util.h"

SongQueueItem::SongQueueItem()
{
    id = 0;
    songid = 0;
    percentage = 0;
}

SongQueueItem::~SongQueueItem()
{
}

QString SongQueueItem::stateText() const
{
    switch ( state )
    {
        case STATE_NOT_READY:
            return QObject::tr("waiting to be downloaded");

        case STATE_GETTING_READY:
            return QObject::tr("downloading %1%%").arg( percentage );

        case STATE_READY:
            return QObject::tr("ready");

        case STATE_PLAYING:
            return QObject::tr("playing");

        default:
            return "unknown";
    }
}

QString SongQueueItem::cachedFileName(const QString &source) const
{
    // We use the first source base name for the cache
    QString basename = QCryptographicHash::hash( sources.front().toUtf8(), QCryptographicHash::Md5 ).toHex();

    // Get the file extension
    QString ext = Util::fileExtension( source );

    return QStandardPaths::writableLocation( QStandardPaths::CacheLocation )
        + QDir::separator()
        + basename + "." + ext;
}
