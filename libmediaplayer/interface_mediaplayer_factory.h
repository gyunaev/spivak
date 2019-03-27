#ifndef INTERFACE_MEDIAPLAYER_FACTORY_H
#define INTERFACE_MEDIAPLAYER_FACTORY_H

class MediaPlayer;

// Implement a factory to create a MediaPlayer object
class Interface_MediaPlayerFactory
{
    public:
        // Returns the media player object created with new; destroy with delete.
        virtual MediaPlayer * create() = 0;
};

#define IID_InterfaceMediaPlayerFactory "com.ulduzsoft.spivak.MediaPlayer-v1"

Q_DECLARE_INTERFACE( Interface_MediaPlayerFactory, IID_InterfaceMediaPlayerFactory )


#endif // INTERFACE_MEDIAPLAYER_FACTORY_H
