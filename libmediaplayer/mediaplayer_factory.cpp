#include "mediaplayer_factory.h"
#include "mediaplayer_gstreamer.h"

MediaPlayer *create_media_player()
{
    return new MediaPlayer_GStreamer( 0 );
}
