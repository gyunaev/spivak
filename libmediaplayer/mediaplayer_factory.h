#ifndef MEDIAPLAYE_FACTORY_H
#define MEDIAPLAYE_FACTORY_H

#include <QObject>

#include "interface_mediaplayer_factory.h"

class MediaPlayerFactory :  public QObject, public Interface_MediaPlayerFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "com.ulduzsoft.spivak.MediaPlayer" )
    Q_INTERFACES( Interface_MediaPlayerFactory )

    public:
        MediaPlayerFactory();

        virtual MediaPlayer * create();
};

#endif // MEDIAPLAYE_FACTORY_H
