#include "collectionproviderhttp.h"

CollectionProviderHTTP::CollectionProviderHTTP()
    : CollectionProvider()
{

}

bool CollectionProviderHTTP::isLocalProvider() const
{
    return false;
}

QString CollectionProviderHTTP::retrieveFile(const QString &filename, QIODevice *storage)
{

}
