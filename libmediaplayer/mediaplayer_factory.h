#ifndef MEDIAPLAYE_FACTORY_H
#define MEDIAPLAYE_FACTORY_H

#include <Qt>

class MediaPlayer;

extern "C" Q_DECL_EXPORT MediaPlayer * create_media_player();

#endif // MEDIAPLAYE_FACTORY_H
