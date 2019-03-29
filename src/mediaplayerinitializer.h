#ifndef MEDIAPLAYERINITIALIZER_H
#define MEDIAPLAYERINITIALIZER_H

#include <QThread>

class MediaPlayer;
class QBuffer;

//
// The role of this class is to initialize the audio system at startup in a separate thread
// (this may be expensive operation requiring loading of dozen shared libraries), as well as
// testing whether the MediaPlayer_GStreamer playback is working , by trying to load and play a
// silent test file.
//
class MediaPlayerInitializer : public QThread
{
    Q_OBJECT

    public:
        MediaPlayerInitializer();
        ~MediaPlayerInitializer();

    signals:
        // Emitted when audio init and test is finished. Successful if errorMsg is empty.
        void    audioInitializationFinished( QString errorMsg );

    private slots:
        // The media file has loaded
        void    audioLoaded();

        // The media file has finished naturally
        void    audioFinished();

        // The media file cannot be loaded, or finished with an error
        void    audioError( QString text );

    protected:
        // the actual thread body
        void    run() override;

    private:
        MediaPlayer * mPlayer;
};

#endif // MEDIAPLAYERINITIALIZER_H
