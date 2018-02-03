#include "settings.h"
#include "collectionprovider.h"
#include "collectionproviderfs.h"
#include "collectionproviderhttp.h"

CollectionProvider::CollectionProvider(int id, QObject *parent)
    : QObject( parent )
{
    collectionID = id;
}

CollectionProvider::~CollectionProvider()
{
}

CollectionProvider *CollectionProvider::createProviderForType(CollectionProvider::Type type, QObject *parent)
{
    switch ( type )
    {
        case TYPE_FILESYSTEM:
            return new CollectionProviderFS( -1, parent );

        case TYPE_HTTP:
            return new CollectionProviderHTTP( -1, parent );

        default:
            return 0;
    }
}

CollectionProvider *CollectionProvider::createProviderForID(int id, QObject *parent)
{
    if ( !pSettings->collections.contains(id) )
        return 0;

    switch ( pSettings->collections[id].type )
    {
        case TYPE_FILESYSTEM:
            return new CollectionProviderFS( id, parent );

        case TYPE_HTTP:
            return new CollectionProviderHTTP( id, parent );

        default:
            return 0;
    }

}

void CollectionProvider::download(int id, const QString &url, QIODevice *local)
{
    QList<QString> urls;
    QList<QIODevice*> files;

    urls << url;
    files << local;

    retrieveMultiple( id, urls, files );
}

void CollectionProvider::downloadAll(int id, const QList<QString> &urls, QList<QIODevice *> locals)
{
    retrieveMultiple( id, urls, locals );
}
