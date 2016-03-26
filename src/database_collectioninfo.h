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

#ifndef DATABASE_COLLECTIONINFO_H
#define DATABASE_COLLECTIONINFO_H

#include <QString>

class Database_CollectionInfo
{
    public:
        Database_CollectionInfo();

        // Collection ID
        int         id;

        // Root path to the directory
        QString     rootPath;

        // Force language (if non -1, the language detection is not performed, and all songs will assume
        // the language specified here. 0 is valid and means no language.
        bool        detectLanguage;

        // Default language (if detection fails or is impossible such as for CD+G or video files)
        QString     defaultLanguage;

        // If true, ZIP archives would be scanned. Otherwise they would be ignored.
        bool        scanZips;

        // Collection artist and title separator (to detect artist/title from filenames)
        QString     artistTitleSeparator;

        // Last time full scan was performed on this collection
        time_t      lastScanned;
};

#endif // DATABASE_COLLECTIONINFO_H
