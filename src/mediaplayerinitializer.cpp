#include <QFile>
#include <QBuffer>
#include <QTimer>

#include "mediaplayerinitializer.h"
#include "libmediaplayer/interface_mediaplayer.h"
#include "pluginmanager.h"

MediaPlayerInitializer::MediaPlayerInitializer()
    : QThread( 0 ), mPlayer(0)
{
    moveToThread( this );
}

MediaPlayerInitializer::~MediaPlayerInitializer()
{
    delete mPlayer;
    qDebug("initializer deleted");
}

void MediaPlayerInitializer::audioLoaded()
{
    // If we are here, the audio is loaded, play it - this is important
    // because GStreamer might fail at this stage, after loading successfully.
    mPlayer->play();
}

void MediaPlayerInitializer::audioFinished()
{
    // If we are here, everything works fine and we're done
    emit audioInitializationFinished( "" );
}

void MediaPlayerInitializer::audioError(QString text)
{
    // Error
    emit audioInitializationFinished( text );
}

void MediaPlayerInitializer::run()
{
    // Create and connect the player
    mPlayer = pPluginManager->createMediaPlayer();

    if ( mPlayer )
    {
        connect( mPlayer->qObject(), SIGNAL( error( QString )), this, SLOT(audioError(QString)) );
        connect( mPlayer->qObject(), SIGNAL( loaded() ), this, SLOT(audioLoaded()) );
        connect( mPlayer->qObject(), SIGNAL( finished() ), this, SLOT(audioFinished()) );

        // Load the media file here - this would initialize the player and thus take a while.
        // This is why we are doing it in a separate thread so we don't block the UI
        // Load the file from resources
        QFile f ( ":/sounds/test.mp3" );

        if ( f.open( QIODevice::ReadOnly ) )
        {
            // Load it into a buffer so we can use loadMedia with QIODevice
            QBuffer * buf = new QBuffer( this );
            buf->setData( f.readAll() );
            buf->open( QIODevice::ReadOnly );

            // Load the data into player (and initialize everything)
            mPlayer->loadMedia( buf, MediaPlayer::LoadAudioStream );
        }
        else
        {
            QTimer::singleShot( 0, this, [this]()->void{ audioError( tr("Failed to open test file") ); } );
        }
    }
    else
    {
        // Player cannot be created, usually GStreamer libraries aren't loaded
        QTimer::singleShot( 0, this, [this]()->void{ audioError( tr("Audio plugin cannot be loaded") ); } );
    }

    // Run the thread loop so we can deliver signals and slots
    exec();
}
