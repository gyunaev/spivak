#ifndef COLLECTIONPROVIDER_H
#define COLLECTIONPROVIDER_H

#include <QThread>
#include <QObject>
#include <QIODevice>

// The role of the provider class is to provide the data files for the player.
// Player can only play files which are stored locally, but some collections
// can store the files remotely (HTTP, DAV, Dropbox etc). This class provides
// the interface for retrieving those files when needed.
class CollectionProvider : public QObject
{
    Q_OBJECT

    public:
        enum Type
        {
            TYPE_INVALID,
            TYPE_FILESYSTEM = 1,
            TYPE_HTTP,
        };

        CollectionProvider(int id, QObject * parent = 0 );
        virtual ~CollectionProvider();

        // Creates a provider to perform actions for specific collection type
        static CollectionProvider * createProviderForType( Type type, QObject * parent = 0 );
        static CollectionProvider * createProviderForID( int id, QObject * parent = 0 );

        // Returns true if this provider is based on file system, and the file operations
        // are supported directly.
        virtual bool    isLocalProvider() const = 0;

        // Returns the provider type
        virtual Type type() const = 0;

        // Makes the file available locally (for example by downloading it)
        // and stores it in the provided QIODevice.
        // Emits the finished() signal once completed or if error happens.
        // While the file is being retrieved, periodically emits progress signal.
        // Function is executed asynchronously.
        void    download( int id, const QString& url, QIODevice * local );

        // Same as above but for multiple files - finished() is only emitted when
        // all files are downloaded.
        void    downloadAll( int id, const QList<QString>& urls, QList<QIODevice *> locals );

    signals:
        void    finished( int id, QString errormsg );
        void    progress( int id, int percentage );

    protected:
        // Makes the files available locally (for example by downloading it) from
        // those URLs, and store them in the provided QIODevices.
        // Emits the finished() signal once completed or if error happens.
        // While the file is being retrieved, periodically emits progress signal.
        // Function is executed asynchronously.
        virtual void retrieveMultiple( int id, const QList<QString>& urls, QList<QIODevice *> files ) = 0;

        // This thread is used to convert async calls to sync calls for retrieval
        QThread workerThread;

        // Collection ID which the collection was created with. -1 if it was created without ID.
        int     collectionID;
};

#endif // COLLECTIONPROVIDER_H
