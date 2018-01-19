#ifndef COLLECTIONPROVIDERHTTP_H
#define COLLECTIONPROVIDERHTTP_H

#include "collectionprovider.h"

class CollectionProviderHTTP : public CollectionProvider
{
    public:
        CollectionProviderHTTP();

        // Returns true if this provider is based on file system, and the file operations
        // are supported directly.
        virtual bool    isLocalProvider() const;

        // Synchronously retrieve the content of a specific file in the provider context.
        // This function may take significant time (i.e. HTTP download) and should thus
        // only be called in a separate thread.
        // It returns an empty string if the file read successfully, otherwise error message.
        virtual QString retrieveFile( const QString& filename, QIODevice * storage );

};

#endif // COLLECTIONPROVIDERHTTP_H
