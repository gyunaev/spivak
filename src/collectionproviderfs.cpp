#include "collectionproviderfs.h"

CollectionProviderFS::CollectionProviderFS()
    : CollectionProvider()
{

}

bool CollectionProviderFS::isLocalProvider() const
{
    return true;
}

QString CollectionProviderFS::retrieveFile(const QString &filename, QIODevice *storage)
{

}
