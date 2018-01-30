#include "songqueueitemretriever.h"

SongQueueItemRetriever::SongQueueItemRetriever()
    : QObject(0)
{
    provider = 0;
}

SongQueueItemRetriever::~SongQueueItemRetriever()
{
    Q_FOREACH ( QIODevice * f, outputs )
    {
        delete f;
    }

    if ( provider )
        provider->deleteLater();
}
