#include <QFile>

#include "collectionproviderfs.h"

CollectionProviderFS::CollectionProviderFS(int id, QObject *parent)
    : CollectionProvider(id, parent)
{
}

bool CollectionProviderFS::isLocalProvider() const
{
    return true;
}

CollectionProvider::Type CollectionProviderFS::type() const
{
    return TYPE_FILESYSTEM;
}

void CollectionProviderFS::retrieveMultiple(int id, const QList<QString> &urls, QList<QIODevice *> files)
{
    // This would definitely be invalid usage
    if ( files.size() != urls.size() )
        abort();

    for ( int i = 0; i < urls.size(); i++ )
    {
        QFile f( urls[i] );

        if ( !f.open( QIODevice::ReadOnly) )
        {
            emit finished( id, f.errorString() );
            return;
        }

        // Not the best way for RAM usage, but we're dealing with small files here
        if ( files[i]->write( f.readAll() ) == -1 )
        {
            emit finished( id, files[i]->errorString() );
            return;
        }

        files[i]->close();
    }

    emit finished( id, QString() );
}
