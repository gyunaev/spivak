#ifndef COLLECTIONPROVIDERFS_H
#define COLLECTIONPROVIDERFS_H

#include "collectionprovider.h"

// This provider implements basic file system access
class CollectionProviderFS : public CollectionProvider
{
    public:
        CollectionProviderFS( int id, QObject * parent );

        // Returns true if this provider is based on file system, and the file operations
        // are supported directly.
        virtual bool    isLocalProvider() const;

        // Returns the provider type
        virtual Type type() const;

    protected:
        // Implemented in subclasses. Should download one or more files syncrhonously,
        // and return when everything is downloaded. With non-zero ID should also
        // call progress and finished callbacks (no callbacks with zero ID)
        virtual void retrieveMultiple( int id, const QList<QString>& urls, QList<QIODevice *> files );
};

#endif // COLLECTIONPROVIDERFS_H
