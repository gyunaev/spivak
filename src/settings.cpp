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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QStandardPaths>

#include "util.h"
#include "settings.h"
#include "logger.h"

Settings * pSettings;


Settings::Settings()
{
    m_appDataPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
    m_settingsFile = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) + QDir::separator() + "config.json";

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

bool Settings::load()
{
    // Load the settings file
    QFile file(m_settingsFile);

    if ( file.open(QIODevice::ReadOnly ) )
    {
        QByteArray data = file.readAll();

        if ( !data.isEmpty() )
        {
            QJsonDocument document = QJsonDocument::fromJson(data);

            if ( document.isObject() )
            {
                // Parse the settings from JSON
                fromJson( document.object() );
                return true;
            }
        }
    }

    // Use default settings
    fromJson( QJsonObject() );
    return false;
}

bool Settings::save()
{
    // QSaveFile is an I/O device for writing text and binary files, without
    // losing existing data if the writing operation fails.
    QSaveFile file(m_settingsFile);

    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        return false;

    QJsonDocument document( toJson() );

    // QSaveFile will remember the write error happen, so no need to check
    file.write( document.toJson() );
    file.commit();
    return true;
}

QJsonObject Settings::toJson()
{
    QJsonObject out;

    out[ "player/BackgroundType" ] = (int) playerBackgroundType;
    out[ "player/BackgroundObjects" ] = fromStringList( playerBackgroundObjects );
    out[ "player/BackgroundTransitionDelay" ] = (int) playerBackgroundTransitionDelay;
    out[ "player/RenderFPS" ] = playerRenderFPS;

    out[ "player/BackgroundColor" ] = playerBackgroundColor.name();
    out[ "player/LyricsFont"] = playerLyricsFont.family();
    out[ "player/LyricsTextBeforeColor"] = playerLyricsTextBeforeColor.name();
    out[ "player/LyricsTextAfterColor"] = playerLyricsTextAfterColor.name();
    out[ "player/LyricsTextSpotColor"] = playerLyricsTextSpotColor.name();
    out[ "player/LyricsTextBackgroundTintPercentage"] = playerLyricBackgroundTintPercentage;
    out[ "player/LyricsFontMaximumSize"] = playerLyricsFontMaxSize;
    out[ "player/LyricsFontFitLines"] = playerLyricsFontFitLines;

    out[ "player/CDGbackgroundTransparent"] = playerCDGbackgroundTransparent;
    out[ "player/IgnoreBackgroundFromFormats"] = playerIgnoreBackgroundFromFormats;
    out[ "player/MusicLyricDelay"] = playerMusicLyricDelay;
    out[ "player/VolumeStep"] = playerVolumeStep;
    out[ "player/UseBuiltinMidiSynth"] = playerUseBuiltinMidiSynth;

    out[ "queue/AddNewSingersNext"] = queueAddNewSingersNext;
    out[ "queue/SaveOnExit"] = queueSaveOnExit;
    out[ "queue/MaxConcurrentPrepare"] = (int) queueMaxConcurrentPrepare;

    out[ "database/PathReplacementPrefixFrom"] = songPathReplacementFrom;
    out[ "database/PathReplacementPrefixTo"] = songPathReplacementTo;

    // LIRC
    out[ "lirc/Enable"] = lircEnabled;
    out[ "lirc/DevicePath"] = lircDevicePath;
    out[ "lirc/MappingFile"] = lircMappingFile;

    // http
    out[ "http/Enabled"] = httpEnabled;
    out[ "http/ListeningPort"] = (int) httpListenPort;
    out[ "http/EnableAddQueue"] = httpEnableAddQueue;

    // Those are only set if not empty
    if ( !httpAccessCode.isEmpty() )
        out[ "http/SecureAccessCode"] = httpAccessCode;

    if ( !httpDocumentRoot.isEmpty() )
        out[ "http/DocumentRoot"] = httpDocumentRoot;

    out[ "http/ForceUseHostname"] = httpForceUseHost;

    out[ "misc/DialogAutoCloseTimer" ] = dialogAutoCloseTimer;

    // Music collection
    out[ "musicCollection/Paths"] = fromStringList( musicCollections );
    out[ "musicCollection/SortedOrder"] = musicCollectionSortedOrder;
    out[ "musicCollection/CrossfadeTime"] = musicCollectionCrossfadeTime;

    out[ "mainmenu/StartInFullscreen"] = startInFullscreen;
    out[ "mainmenu/FirstTimeWizardShown"] = firstTimeWizardShown;

    // Notification
    out[ "notification/TopColor"] = notificationTopColor.name();
    out[ "notification/CenterColor"] = notificationCenterColor.name();

    // Store the cache dir only if the location is changed (i.e. not standard)
    if ( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) != cacheDir )
        out[ "player/cacheDir"] = cacheDir;

    // Collections
    QJsonArray colarray;

    Q_FOREACH( const CollectionEntry& e, collections )
    {
        colarray.push_back( e.toJson() );
    }

    out[ "collection" ] = colarray;

    return out;
}

void Settings::fromJson(const QJsonObject &data)
{
    playerBackgroundType = (BackgroundType) data.value( "player/BackgroundType" ).toInt( (int)BACKGROUND_TYPE_IMAGE );
    playerBackgroundObjects = toStringList( data.value( "player/BackgroundObjects" ), ":/background" );
    playerBackgroundTransitionDelay = data.value( "player/BackgroundTransitionDelay" ).toInt( 30 );
    playerRenderFPS = data.value( "player/RenderFPS" ).toInt( 25 );
    playerBackgroundColor = QColor( data.value( "player/BackgroundColor" ).toString( "black" ) );
    playerLyricsFont = QFont( data.value( "player/LyricsFont" ).toString( "arial" ) );
    playerLyricsTextBeforeColor = QColor( data.value( "player/LyricsTextBeforeColor" ).toString( "blue" ) );
    playerLyricsTextAfterColor = QColor( data.value( "player/LyricsTextAfterColor" ).toString("red") );
    playerLyricsTextSpotColor = QColor( data.value( "player/LyricsTextSpotColor" ).toString("yellow") );
    playerCDGbackgroundTransparent = data.value( "player/CDGbackgroundTransparent" ).toBool(false);
    playerMusicLyricDelay = data.value( "player/MusicLyricDelay" ).toInt( 0 );
    playerIgnoreBackgroundFromFormats = data.value( "player/IgnoreBackgroundFromFormats" ).toBool(false);
    playerLyricsFontFitLines = data.value( "player/LyricsFontFitLines").toInt( 4 );
    playerLyricsFontMaxSize = data.value( "player/LyricsFontMaximumSize" ).toInt( 512);
    playerVolumeStep = data.value( "player/VolumeStep").toInt(10);
    playerUseBuiltinMidiSynth = data.value( "player/UseBuiltinMidiSynth" ).toBool( true );
    playerLyricBackgroundTintPercentage = data.value( "player/LyricsTextBackgroundTintPercentage" ).toInt( 75 );

    queueAddNewSingersNext = data.value( "queue/AddNewSingersNext" ).toBool(false);
    queueSaveOnExit = data.value( "queue/SaveOnExit" ).toBool(false);
    queueMaxConcurrentPrepare = data.value( "queue/MaxConcurrentPrepare" ).toInt( 3 );

    songPathReplacementFrom = data.value( "database/PathReplacementPrefixFrom" ).toString();
    songPathReplacementTo = data.value( "database/PathReplacementPrefixTo" ).toString();

    lircDevicePath = data.value( "lirc/DevicePath" ).toString();
    lircMappingFile = data.value( "lirc/MappingFile" ).toString();
    lircEnabled = data.value( "lirc/Enable" ).toBool( false );

    // http
    httpEnabled = data.value( "http/Enabled" ).toBool( false );
    httpListenPort = data.value( "http/ListeningPort" ).toInt( 8000 );
    httpDocumentRoot = data.value( "http/DocumentRoot" ).toString();
    httpEnableAddQueue = data.value( "http/EnableAddQueue" ).toBool( false );
    httpAccessCode = data.value( "http/SecureAccessCode" ).toString();
    httpForceUseHost = data.value( "http/ForceUseHostname" ).toString();

    startInFullscreen = data.value( "mainmenu/StartInFullscreen" ).toBool( false );
    firstTimeWizardShown = data.value( "mainmenu/FirstTimeWizardShown" ).toBool( false );

    dialogAutoCloseTimer = data.value( "misc/DialogAutoCloseTimer" ).toInt( 10 );

    cacheDir = data.value( "player/cacheDir" ).toString( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) );

    // Notification
    notificationTopColor = QColor( data.value( "notification/TopColor" ).toString( "white" ) );
    notificationCenterColor = QColor( data.value( "notification/CenterColor" ).toString( "white" ) );

    // Music collection
    musicCollections = toStringList( data.value( "musicCollection/Paths" ) );
    musicCollectionSortedOrder = data.value( "musicCollection/SortedOrder" ).toBool( true );
    musicCollectionCrossfadeTime = data.value( "musicCollection/CrossfadeTime" ).toInt( 5 );

    // Collections
    collections.clear();
    QJsonArray colarray = data.value("collection").toArray();

    Q_FOREACH( const QJsonValue& v, colarray )
    {
        CollectionEntry entry;
        entry.fromJson( v.toObject() );
        collections[ entry.id ] = entry;
    }
}

QJsonValue Settings::fromStringList(const QStringList &list)
{
    if ( list.isEmpty() )
        return QString();

    return list.join( "|" );
}

QStringList Settings::toStringList( const QJsonValue& value, const QString& defaultval )
{
    QString listvalue = value.toString( defaultval );

    if ( listvalue.isEmpty() )
        return QStringList();

    return listvalue.split( "|" );
}
