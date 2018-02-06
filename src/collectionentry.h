#ifndef COLLECTIONENTRY_H
#define COLLECTIONENTRY_H

#include <QJsonObject>
#include "collectionprovider.h"

class CollectionEntry
{
    public:
        CollectionEntry();

        // Collection ID. Fixed. Data in sqlite (songs.collectionid value)
        int         id;

        // Collection priority index (how early should it be used). 0 - earliest
        int         priority;

        // Collection type
        CollectionProvider::Type type;

        // Collection human-readable name
        QString     name;

        // Collection root (either path to the directory or root URL)
        QString     rootPath;

        // Authentication information (if needed)
        QString     authuser;
        QString     authpass;

        // For HTTP collection, whether to ignore SSL errors
        bool        ignoreSSLerrors;

        // Detect lyrics language (if false, language is not detected)
        bool        detectLanguage;

        // Default language (if detection fails or is impossible such as for CD+G or video files)
        // If empty, no language is set
        QString     defaultLanguage;

        // If true, ZIP archives are considered to be Karaoke files and would be scanned.
        // Otherwise they would be ignored.
        bool        scanZips;

        // Collection artist and title separator (to detect artist/title from filenames)
        QString     artistTitleSeparator;

        // Last time full scan was performed on this collection. -1 - never
        time_t      lastScanned;

    public:
        // Convert this collection to/from JSON
        QJsonObject toJson() const;
        void        fromJson( const QJsonObject & json );
};

#endif // COLLECTIONENTRY_H
