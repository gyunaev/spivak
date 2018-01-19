#include "collectionprovider.h"
#include "collectionproviderfs.h"
#include "collectionproviderhttp.h"


CollectionProvider::~CollectionProvider()
{
}

CollectionProvider::CollectionProvider()
{
}


// A simple factory returning a provider by type
CollectionProvider *CollectionProvider::createProvider(CollectionEntry::Type type)
{
    switch ( type )
    {
        case CollectionEntry::TYPE_FILESYSTEM:
            return new CollectionProviderFS();

        case CollectionEntry::TYPE_HTTP:
            return new CollectionProviderHTTP();

        default:
            return 0;
    }
}
