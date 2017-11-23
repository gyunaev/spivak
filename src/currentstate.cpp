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
#include <QFileInfo>
#include <QSettings>
#include <QApplication>

#include "util.h"
#include "logger.h"
#include "actionhandler.h"
#include "settings.h"
#include "currentstate.h"

CurrentState * pCurrentState;

CurrentState::CurrentState(QObject *parent)
    : QObject(parent)
{
    msecPerFrame = 1000 / pSettings->playerRenderFPS;

    QSettings settings;
    playerVolume = settings.value( "temp/playerVolume", 50 ).toInt();
    musicVolume = settings.value( "temp/musicVolume", 50 ).toInt();
    windowSizeKarQueue = settings.value( "temp/windowSizeKarQueue", QSize(700, 550 )).toSize();
    windowSizeMusicQueue = settings.value( "temp/windowSizeMusicQueue", QSize(350, 500 )).toSize();
    windowSizeMain = settings.value( "temp/windowSizeMain", QSize(800, 600)).toSize();

    QStringList splitterSizes = settings.value( "temp/splitterSizeKarQueue", QStringList()).toStringList();

    Q_FOREACH ( QString s, splitterSizes )
    {
        splitterSizeKarQueue.push_back( s.toInt( ) );
    }

    m_playerBackgroundLastObject = 0;
    playerBackgroundLastVideoPosition = 0;
    m_databaseSongs = 0;
    m_databaseArtists = 0;

    // Validate settings
    applySettingsBackground();
}

void CurrentState::loadBackgroundObjects()
{
    m_playerBackgroundObjects.clear();

    QStringList extensions;

    if ( pSettings->playerBackgroundType == Settings::BACKGROUND_TYPE_IMAGE )
        extensions << "jpg" << "png" << "gif" << "bmp" << "webp" << "jpeg";
    else
        extensions << "avi" << "mkv" << "mp4" << "3gp" << "mov" << "webm";

    // Entries in playerBackgroundObjects could be files or directories
    Q_FOREACH( const QString& path, pSettings->playerBackgroundObjects )
        Util::enumerateDirectory( path, extensions, m_playerBackgroundObjects );

    Logger::debug( "Background objects: %d loaded", m_playerBackgroundObjects.size() );

    // Load temporary parameters and validate them
    QSettings settings;

    // Do we still have this object?
    QString lastobject = settings.value( "temp/playerBackgroundLastObject", "" ).toString();
    int lastindex;

    if ( lastobject.isEmpty() || (lastindex = m_playerBackgroundObjects.indexOf( lastobject )) == -1 )
    {
        // Nope; reset the values
        m_playerBackgroundLastObject = 0;
        playerBackgroundLastVideoPosition = 0;
    }
    else
    {
        // Yes; read the rest values
        m_playerBackgroundLastObject = lastindex;
        playerBackgroundLastVideoPosition = settings.value( "temp/playerBackgroundLastVideoPosition", 0 ).toInt();
    }
}

void CurrentState::saveTempData()
{
    // Save temporary parameters
    QSettings settings;

    if ( !m_playerBackgroundObjects.isEmpty() )
    {
        settings.setValue( "temp/playerBackgroundLastObject", currentBackgroundObject() );
        settings.setValue( "temp/playerBackgroundLastVideoPosition", playerBackgroundLastVideoPosition );
    }

    settings.setValue( "temp/playerVolume", playerVolume );
    settings.setValue( "temp/musicVolume", musicVolume );
    settings.setValue( "temp/windowSizeKarQueue", windowSizeKarQueue );
    settings.setValue( "temp/windowSizeMusicQueue", windowSizeMusicQueue );
    settings.setValue( "temp/windowSizeMain", windowSizeMain );

    QStringList sizes;

    Q_FOREACH ( int i, splitterSizeKarQueue )
    {
        sizes.push_back( QString::number( i ) );
    }

    settings.setValue( "temp/splitterSizeKarQueue", sizes );
}

QString CurrentState::currentBackgroundObject()
{
    if ( m_playerBackgroundObjects.isEmpty() )
        return "";

    return m_playerBackgroundObjects[ m_playerBackgroundLastObject ];
}

void CurrentState::nextBackgroundObject()
{
    pCurrentState->playerBackgroundLastVideoPosition = 0;
    m_playerBackgroundLastObject++;

    if ( (int) m_playerBackgroundLastObject >= m_playerBackgroundObjects.size() )
        m_playerBackgroundLastObject = 0;
}

void CurrentState::applySettingsBackground()
{
    if ( pSettings->playerBackgroundType == Settings::BACKGROUND_TYPE_IMAGE || pSettings->playerBackgroundType == Settings::BACKGROUND_TYPE_VIDEO )
    {
        loadBackgroundObjects();

        if ( m_playerBackgroundObjects.isEmpty() )
        {
            pActionHandler->error( tr("No %1 was found; disabling background") .arg( pSettings->playerBackgroundType == Settings::BACKGROUND_TYPE_VIDEO ? "videos" : "images") );
            pSettings->playerBackgroundType = Settings::BACKGROUND_TYPE_NONE;
        }
    }
}
