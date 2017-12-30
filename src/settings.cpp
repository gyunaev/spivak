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

#include <QDir>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#include "util.h"
#include "settings.h"
#include "logger.h"

Settings * pSettings;


Settings::Settings()
{
    m_appDataPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );

    if ( m_appDataPath.isEmpty() )
        m_appDataPath = ".";

    if ( !m_appDataPath.endsWith( Util::separator() ) )
        m_appDataPath += Util::separator();

    songdbFilename = m_appDataPath + "karaoke.db";
    queueFilename = m_appDataPath + "queue.dat";

    // Create the application data dir if it doesn't exist
    if ( !QFile::exists( m_appDataPath ) )
    {
        if ( !QDir().mkpath( m_appDataPath ) )
            qCritical("Cannot create application data directory %s", qPrintable(m_appDataPath) );
    }

    // Create the cache data dir if it doesn't exist
    if ( !QFile::exists( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ) )
    {
        if ( !QDir().mkpath( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ) )
            qCritical("Cannot create application cache directory %s", qPrintable(QStandardPaths::writableLocation( QStandardPaths::AppDataLocation )) );
    }

    load();

    // songPathPrefix should follow directory separator
    if ( !songPathReplacementFrom.isEmpty() && !songPathReplacementFrom.endsWith( Util::separator() ) )
        songPathReplacementFrom.append( Util::separator() );
}

QString Settings::replacePath(const QString &origpath)
{
    if ( songPathReplacementFrom.isEmpty() || !origpath.startsWith( songPathReplacementFrom ) )
        return origpath;

    return songPathReplacementTo + origpath.mid( ( songPathReplacementFrom.length() ) );
}

void Settings::load()
{
    QSettings settings;

    playerBackgroundType = (BackgroundType) settings.value( "player/BackgroundType", (int)BACKGROUND_TYPE_IMAGE ).toInt();
    playerBackgroundObjects = settings.value( "player/BackgroundObjects", QStringList() << ":/background" ).toStringList();
    playerBackgroundTransitionDelay = settings.value( "player/BackgroundTransitionDelay", 30 ).toInt();
    playerRenderFPS = settings.value( "player/RenderFPS", 25 ).toInt();
    playerBackgroundColor = QColor( settings.value( "player/BackgroundColor", "black" ).toString() );
    playerLyricsFont = QFont( settings.value( "player/LyricsFont", "arial" ).toString() );
    playerLyricsTextBeforeColor = QColor( settings.value( "player/LyricsTextBeforeColor", "blue" ).toString() );
    playerLyricsTextAfterColor = QColor( settings.value( "player/LyricsTextAfterColor", "red" ).toString() );
    playerLyricsTextSpotColor = QColor( settings.value( "player/LyricsTextSpotColor", "yellow" ).toString() );
    playerCDGbackgroundTransparent = settings.value( "player/CDGbackgroundTransparent", false ).toBool();
    playerMusicLyricDelay = settings.value( "player/MusicLyricDelay", 0 ).toInt();
    playerIgnoreBackgroundFromFormats = settings.value( "player/IgnoreBackgroundFromFormats", false ).toBool();
    playerLyricsFontFitLines = settings.value( "player/LyricsFontFitLines", 4 ).toInt();
    playerLyricsFontMaxSize = settings.value( "player/LyricsFontMaximumSize", 512 ).toInt();
    playerVolumeStep = settings.value( "player/VolumeStep", 10 ).toInt();
    playerUseBuiltinMidiSynth = settings.value( "player/UseBuiltinMidiSynth", true ).toBool();
    playerLyricBackgroundTintPercentage = settings.value( "player/LyricsTextBackgroundTintPercentage", 75 ).toInt();

    queueAddNewSingersNext = settings.value( "queue/AddNewSingersNext", false ).toBool();
    queueSaveOnExit = settings.value( "queue/SaveOnExit", false ).toBool();

    songPathReplacementFrom = settings.value( "database/PathReplacementPrefixFrom", "" ).toString();
    songPathReplacementTo = settings.value( "database/PathReplacementPrefixTo", "" ).toString();

    lircDevicePath = settings.value( "lirc/DevicePath", "" ).toString();
    lircMappingFile = settings.value( "lirc/MappingFile", "" ).toString();
    lircEnabled = settings.value( "lirc/Enable", false ).toBool();

    // http
    httpEnabled = settings.value( "http/Enabled", false ).toBool();
    httpListenPort = settings.value( "http/ListeningPort", 8000 ).toInt();
    httpDocumentRoot = settings.value( "http/DocumentRoot", "" ).toString();
    httpEnableAddQueue = settings.value( "http/EnableAddQueue", false ).toBool();
    httpAccessCode = settings.value( "http/SecureAccessCode", "" ).toString();

    startInFullscreen = settings.value( "mainmenu/StartInFullscreen", false ).toBool();
    firstTimeWizardShown = settings.value( "mainmenu/FirstTimeWizardShown", false ).toBool();

    cacheDir = settings.value( "player/cacheDir", QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).toString();

    // Notification
    notificationTopColor = QColor( settings.value( "notification/TopColor", "white" ).toString() );
    notificationCenterColor = QColor( settings.value( "notification/CenterColor", "white" ).toString() );

    // Music collection
    musicCollections = settings.value( "musicCollection/Paths", QStringList() ).toStringList();
    musicCollectionSortedOrder = settings.value( "musicCollection/SortedOrder", true ).toBool();
    musicCollectionCrossfadeTime = settings.value( "musicCollection/CrossfadeTime", 5 ).toInt();
}

void Settings::save()
{
    QSettings settings;

    settings.setValue( "player/BackgroundType", playerBackgroundType );
    settings.setValue( "player/BackgroundObjects", playerBackgroundObjects );
    settings.setValue( "player/BackgroundTransitionDelay", playerBackgroundTransitionDelay );
    settings.setValue( "player/RenderFPS", playerRenderFPS );

    settings.setValue( "player/BackgroundColor", playerBackgroundColor.name() );
    settings.setValue( "player/LyricsFont", playerLyricsFont.family() );
    settings.setValue( "player/LyricsTextBeforeColor", playerLyricsTextBeforeColor.name() );
    settings.setValue( "player/LyricsTextAfterColor", playerLyricsTextAfterColor.name() );
    settings.setValue( "player/LyricsTextSpotColor", playerLyricsTextSpotColor.name() );
    settings.setValue( "player/LyricsTextBackgroundTintPercentage", playerLyricBackgroundTintPercentage );
    settings.setValue( "player/LyricsFontMaximumSize", playerLyricsFontMaxSize );
    settings.setValue( "player/LyricsFontFitLines", playerLyricsFontFitLines );

    settings.setValue( "player/CDGbackgroundTransparent", playerCDGbackgroundTransparent );
    settings.setValue( "player/IgnoreBackgroundFromFormats", playerIgnoreBackgroundFromFormats );
    settings.setValue( "player/MusicLyricDelay", playerMusicLyricDelay );
    settings.setValue( "player/VolumeStep", playerVolumeStep );
    settings.setValue( "player/UseBuiltinMidiSynth", playerUseBuiltinMidiSynth );

    settings.setValue( "queue/AddNewSingersNext", queueAddNewSingersNext );
    settings.setValue( "queue/SaveOnExit", queueSaveOnExit );

    settings.setValue( "database/PathReplacementPrefixFrom", songPathReplacementFrom );
    settings.setValue( "database/PathReplacementPrefixTo", songPathReplacementTo );

    // LIRC
    settings.setValue( "lirc/Enable", lircEnabled );
    settings.setValue( "lirc/DevicePath", lircDevicePath );
    settings.setValue( "lirc/MappingFile", lircMappingFile );

    // http
    settings.setValue( "http/Enabled", httpEnabled );
    settings.setValue( "http/ListeningPort", httpListenPort );
    settings.setValue( "http/DocumentRoot", httpDocumentRoot );
    settings.setValue( "http/EnableAddQueue", httpEnableAddQueue );
    settings.setValue( "http/SecureAccessCode", httpAccessCode );

    // Music collection
    settings.setValue( "musicCollection/Paths", musicCollections );
    settings.setValue( "musicCollection/SortedOrder", musicCollectionSortedOrder );
    settings.setValue( "musicCollection/CrossfadeTime", musicCollectionCrossfadeTime );

    settings.setValue( "mainmenu/StartInFullscreen", startInFullscreen );
    settings.setValue( "mainmenu/FirstTimeWizardShown", firstTimeWizardShown );

    // Notification
    settings.setValue( "notification/TopColor", notificationTopColor.name() );
    settings.setValue( "notification/CenterColor", notificationCenterColor.name() );

    // Store the cache dir only if the location is changed (i.e. not standard)
    if ( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) != cacheDir )
        settings.setValue( "player/cacheDir", cacheDir );
}
