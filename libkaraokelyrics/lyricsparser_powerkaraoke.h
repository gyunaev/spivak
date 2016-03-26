#ifndef LYRICSPARSER_POWERKARAOKE_H
#define LYRICSPARSER_POWERKARAOKE_H

#include "lyricsparser.h"

class LyricsParser_PowerKaraoke : public LyricsParser
{
    public:
        LyricsParser_PowerKaraoke();

        // Parses the lyrics, filling up the output container. Throws an error
        // if there are any issues during parsing, otherwise fills up output.
        void parse(QIODevice *file, LyricsLoader::Container& output, LyricsLoader::Properties& properties );
};

#endif // LYRICSPARSER_POWERKARAOKE_H
