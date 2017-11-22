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

#include "playerlyrics.h"
#include "settings.h"
#include "karaokepainter.h"
#include "currentstate.h"

PlayerLyrics::PlayerLyrics()
{
    m_timeDelay = pSettings->playerMusicLyricDelay;
}

PlayerLyrics::~PlayerLyrics()
{
}

bool PlayerLyrics::draw(KaraokePainter &p)
{
    qint64 time = qMax( 0, pCurrentState->playerPosition + m_timeDelay );

    if ( !render( p, time ) )
        return false;

    m_lastRenderedTime = pCurrentState->playerPosition;
    return true;
}

void PlayerLyrics::setDelay(int delayms)
{
    m_timeDelay = delayms;
    pCurrentState->playerLyricsDelay = delayms;
}

int PlayerLyrics::delay() const
{
    return m_timeDelay;
}

LyricsLoader::Properties PlayerLyrics::properties() const
{
    return m_properties;
}
