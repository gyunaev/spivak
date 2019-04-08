#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QBuffer>
#include <QTimer>

#include "mediaplayerinitializer.h"
#include "libmediaplayer/interface_mediaplayer.h"
#include "pluginmanager.h"
#include "settings.h"

#if defined (Q_OS_WIN)
static const char * GSTREAMER_LIBRARY_NAME = "libgstreamer-1.0-0.dll";
static const char * GSTREAMER_PLUGIN_NAME = "libgstaudioconvert.dll";
#elif defined (Q_OS_MAC)
#CHECKME
static const char * GSTREAMER_LIBRARY_NAME = "libgstreamer-1.0.dylib";
static const char * GSTREAMER_PLUGIN_NAME = "libgstaudioconvert.dylib";
#else
static const char * GSTREAMER_LIBRARY_NAME = "libgstreamer-1.0.so";
static const char * GSTREAMER_PLUGIN_NAME = "libgstaudioconvert.so";
#endif

MediaPlayerInitializer::MediaPlayerInitializer()
    : QThread( 0 ), mPlayer(0)
{
    // Finds path to GStreamer library and presets the settings if needed
    findGStreamerPath();

    moveToThread( this );
}

MediaPlayerInitializer::~MediaPlayerInitializer()
{
    delete mPlayer;
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

void MediaPlayerInitializer::findGStreamerPath()
{
    // Try to load GStreamer library as-is
    QLibrary lib( GSTREAMER_LIBRARY_NAME );

    if ( lib.load() )
    {
        // We are good, its in the system path. No need to set mGStreamerPath
        qDebug("Found GStreamer libraries in PATH/LD_LIBRARY_PATH");
        lib.unload();
        return;
    }

    // Try to find GStreamer path. First check the one we have in settings
    if ( !pSettings->pathGStreamerBinaries.isEmpty() && checkGStreamerPath( pSettings->pathGStreamerBinaries ) )
        return;

    // Now try the app install directory
    if ( checkGStreamerPath( QApplication::applicationDirPath() + QDir::separator() + "gstreamer" ) )
        return;

    // Now try the default installation directory C:\GStreamer\1.0\<arch>\bin
    if ( checkGStreamerPath( QString("C:\\GStreamer\\1.0\\%1\\bin\\")
                                      .arg( QSysInfo::buildCpuArchitecture() == "i386" ? "x86" : "x86_64" ) ) )
        return;

    // Nothing found. Ask the user.
    while ( true )
    {
        QString path = QFileDialog::getExistingDirectory( 0,
                                                          tr("Please choose GStreamer directory"),
                                                          "" );

        if ( !checkGStreamerPath( path ) )
        {
            if ( QMessageBox::question( 0,
                                   tr("Invalid GStreamer path"),
                                   tr("The selected GStreamer path is invalid.\n"
                                      "The shared library %1 file was not found at this path\n\n"
                                      "Would you like to choose a path again?") ) == QMessageBox::Yes )
            {
                // Ask again
                continue;
            }

            // Do not ask, use default and hope its loaded
            break;
        }
    }
}

bool MediaPlayerInitializer::checkGStreamerPath( const QString &path )
{
    QString libpath = path + QDir::separator() + GSTREAMER_LIBRARY_NAME;

    if ( !QFile::exists( libpath ) )
        return false;

    // We found the path. Now try to find the plugin path
    QString gstPluginPath;

    QStringList pluginpaths;
    pluginpaths << "gstreamer" << "../lib/gstreamer-1.0";

    // Try all possible plugin paths
    for ( QString pluginpath : pluginpaths )
    {
        QString checkpath = path + QDir::separator() + pluginpath + QDir::separator() + GSTREAMER_PLUGIN_NAME;

        if ( QFile::exists( checkpath ) )
        {
            qDebug("Detected GST plugin path %s", qPrintable( checkpath ) );
            gstPluginPath = path + QDir::separator() + pluginpath;
            break;
        }
    }

    if ( gstPluginPath.isEmpty() )
    {
        qDebug("GST plugin path is not detected in GST path %s, ignoring", qPrintable( path ) );
        return false;
    }

    // Add the path to the system-specific path
#ifdef WIN32
    path.replace( "/", "\\" );
    QString var = qgetenv( "PATH" ) + ";" + path;
    qputenv( "PATH", var );
#else
    qputenv( "LD_LIBRARY_PATH", qgetenv( "LD_LIBRARY_PATH" ) + ":" + path.toUtf8() );
#endif

    gstPluginPath.replace( "/", QDir::separator() );
    qputenv( "GST_PLUGIN_PATH", gstPluginPath.toUtf8() );

    qDebug("GStreamer loader: load path is set to %s, plugin path %s",
           qPrintable( path ),
           qPrintable( gstPluginPath ) );

    pSettings->pathGStreamerBinaries = path;
    return true;
}
