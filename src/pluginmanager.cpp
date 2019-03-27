/**************************************************************************
 *  Spivak Karaoke PLayer - a free, cross-platform desktop karaoke player *
 *  Copyright (C) 2015-2016 George Yunaev, support@ulduzsoft.com          *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *																	      *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <QApplication>
#include <QPluginLoader>
#include <QFile>
#include <QDir>

#include "logger.h"
#include "pluginmanager.h"

PluginManager * pPluginManager;

static const char * langplugin = "plugin_langdetect";
static const char * pitchplugin = "plugin_pitchchanger";

PluginManager::PluginManager( const QString& pluginPath )
{
    m_pluginPath = pluginPath;
    mCreateMediaPlayerFunction = 0;

    // We allow to override our plugin path
    if ( qEnvironmentVariableIsSet("SPIVAK_PLUGIN_PATH") )
        m_pluginPath = QString::fromUtf8( qgetenv("SPIVAK_PLUGIN_PATH") );

    // If invalid or empty - use the local path
    if ( m_pluginPath.isEmpty() || !QFile::exists( m_pluginPath ) )
        m_pluginPath = QApplication::applicationDirPath() + "/plugins";

#ifdef Q_OS_LINUX
    // Linux: and if this is invalid, use the system one
    if ( m_pluginPath.isEmpty() || !QFile::exists( m_pluginPath ) )
#ifdef Q_PROCESSOR_X86_64
        m_pluginPath = "/usr/lib64/spivak/plugins";
#else
        m_pluginPath = "/usr/lib/spivak/plugins";
#endif
#endif
}

Interface_LanguageDetector *PluginManager::loadLanguageDetector()
{
    return loadPlugin<Interface_LanguageDetector>( langplugin );
}

void PluginManager::releaseLanguageDetector()
{
    pPluginManager->releasePlugin( langplugin );
}

Interface_MediaPlayerPlugin *PluginManager::loadPitchChanger()
{
    return loadPlugin<Interface_MediaPlayerPlugin>( pitchplugin );
}

MediaPlayer *PluginManager::createMediaPlayer()
{
    if ( !mMediaPlayerLibrary.isLoaded() )
    {
        mMediaPlayerLibrary.setFileName( "mediaplayer" );

        if ( !mMediaPlayerLibrary.load() )
        {
            Logger::error( "Failed to load media player library: %s", qPrintable( mMediaPlayerLibrary.errorString() ) );
            return 0;
        }

        mCreateMediaPlayerFunction = (MediaPlayer * (*)()) mMediaPlayerLibrary.resolve( "createMediaPlayer" );

        if ( !mCreateMediaPlayerFunction )
        {
            Logger::error( "Failed to resolve createMediaPlayer function in the media player library %s", qPrintable( mMediaPlayerLibrary.fileName() ) );
            mMediaPlayerLibrary.unload();
            return 0;
        }
    }

    return mCreateMediaPlayerFunction();
}

void PluginManager::releasePlugin(const QString &name)
{
    // QPluginLoader picks up the current library state, and should not
    // load it again if it is loaded already
    QPluginLoader loader( pPluginManager->m_pluginPath + name );

    if ( loader.isLoaded() )
        loader.unload();
}

template <class T> T *PluginManager::loadPlugin(const char *name)
{
    if ( !pPluginManager )
        pPluginManager = new PluginManager();

#if defined (Q_OS_WIN)
    const char * pluginextension = ".dll";
#elif defined (Q_OS_MAC)
    const char * pluginextension = ".bundle";
#else
    const char * pluginextension = ".so";
#endif

    QString path = pPluginManager->m_pluginPath + QDir::separator() + name + pluginextension;
    Logger::debug( "PluginManager: attempting to load plugin %s from %s", name, qPrintable( path ));
    QPluginLoader loader( path );

    // This will load the plugin if it is not loaded yet
    QObject * instance = loader.instance();

    if ( !instance )
    {
        Logger::error( "PluginManager: failed to load plugin %s from %s: %s",
                       name, qPrintable( path ), qPrintable(loader.errorString()) );
        return 0;
    }

    T * iface = qobject_cast< T * > (instance);

    // If this is null, the plugin is loaded, but does not export proper interface
    if ( !iface )
    {
        loader.unload();
        return 0;
    }

    Logger::debug( "PluginManager: plugin %s is successfully loaded from %s", name, qPrintable( path ));
    return iface;
}
