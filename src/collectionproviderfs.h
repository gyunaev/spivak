#ifndef COLLECTIONPROVIDERFS_H
#define COLLECTIONPROVIDERFS_H

#include "collectionprovider.h"

class CollectionProviderFS : public CollectionProvider
{
    public:
        CollectionProviderFS();

        // Returns true if this provider is based on file system, and the file operations
        // are supported directly.
        virtual bool    isLocalProvider() const;

        // Synchronously retrieve the content of a specific file in the provider context.
        // This function may take significant time (i.e. HTTP download) and should thus
        // only be called in a separate thread.
        // It returns an empty string if the file read successfully, otherwise error message.
        virtual QString retrieveFile( const QString& filename, QIODevice * storage );
};

#endif // COLLECTIONPROVIDERFS_H
