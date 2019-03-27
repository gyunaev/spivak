#include "mediaplayer_factory.h"
#include "mediaplayer_gstreamer.h"


MediaPlayerFactory::MediaPlayerFactory()
{
}

MediaPlayer *MediaPlayerFactory::create()
{
    return new MediaPlayer_GStreamer();
}
