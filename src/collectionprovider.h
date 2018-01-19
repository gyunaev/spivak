#ifndef COLLECTIONPROVIDER_H
#define COLLECTIONPROVIDER_H

#include <QObject>
#include <QIODevice>
#include "collectionentry.h"

// The role of the provider class is to provide the data files for the player.
// Player can only play files which are stored locally, but some collections
// can store the files remotely (HTTP, DAV, Dropbox etc). This class provides
// the interface for retrieving those files when needed.
class CollectionProvider : public QObject
{
    Q_OBJECT
    public:
        virtual ~CollectionProvider();

        // Creates a provider to perform actions for specific collection type
        static CollectionProvider * createProvider( CollectionEntry::Type type );

        // Returns true if this provider is based on file system, and the file operations
        // are supported directly.
        virtual bool    isLocalProvider() const = 0;

        // Synchronously retrieve the content of a specific file in the provider context.
        // This function may take significant time (i.e. HTTP download) and should thus
        // only be called in a separate thread.
        // It returns an empty string if the file read successfully, otherwise error message.
        virtual QString retrieveFile( const QString& filename, QIODevice * storage ) = 0;

        // Makes the file available locally (for example by downloading it)
        // and stores it in the provided QIODevice.
        // Emits the finished() signal once completed or if error happens.
        // While the file is being retrieved, periodically emits progress signal.
        // Function is executed asynchronously.
        void    startRetrieveOne( int id, const QString& url, QIODevice * local );

        // Same as above but for multiple files - finished() is only emitted when
        // all files are downloaded.
        void    startRetrieveAll( int id, const QList<QUrl>& urls, QList<QIODevice *> locals );

    signals:
        void    finished( int id, QString errormsg );
        void    progress( int id, int percentage );

    protected:
        CollectionProvider();
};

#endif // COLLECTIONPROVIDER_H
