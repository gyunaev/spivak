#include "collectionprovider.h"
#include "collectionproviderfs.h"
#include "collectionproviderhttp.h"

CollectionProvider::CollectionProvider(QObject *parent)
    : QObject( parent )
{
}

CollectionProvider::~CollectionProvider()
{
}


// A simple factory returning a provider by type
CollectionProvider *CollectionProvider::createProvider( Type type, QObject * parent )
{
    switch ( type )
    {
        case TYPE_FILESYSTEM:
            return new CollectionProviderFS(parent);

        case TYPE_HTTP:
            return new CollectionProviderHTTP(parent);

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
