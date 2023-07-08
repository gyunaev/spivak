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

#ifndef BACKGROUNDIMAGE_H
#define BACKGROUNDIMAGE_H

#include <QTime>
#include <QImage>
#include <QAtomicInt>

#include "karaokepainter.h"
#include "background.h"

// Background for image or slideshow
class BackgroundImage : public Background
{
    public:
        BackgroundImage();

        // Background could be initialized either from the settings (by callign initFromSettings() or from
        // a specific file/QIODevice - for example for KFN files (the file name needs to be passed to know which type
        // of file is being loaded). If the background cannot be initialized, it must return an error.
        virtual bool    initFromSettings();
        virtual bool    initFromFile( QIODevice * file, const QString& filename );

        virtual void    pause( bool pausing );

        // Draws the background on the image; the prior content of the image is undefined. If false is returned,
        // it is considered an error, and the whole playing process is aborted - use wisely
        virtual qint64  draw( KaraokePainter& p );

    private:
        void    loadNewImage();
        bool    performTransition( KaraokePainter& p );

        void    animateMove( KaraokePainter& p );
        void    animateZoom( KaraokePainter& p );

        // Last time the image was updated
        QTime   m_nextUpdate;

        // If transition is in progress, it is from current to new; otherwise new is always invalid
        QImage  m_currentImage;
        QImage  m_newImage;

        // For transition between current and new image - percentage (0-100) of how much of new image is shown
        int     m_percentage;

        // Animation speed: 0 - no animation
        int     m_animationSpeed;

        // For animateMove
        QPoint  m_movementOrigin;   // current top-left coordinate of the origin rectangle
        QPoint  m_movementVelocity;

        // For animateZoom - percentage between 100% desktop size an
        int     m_zoomFactor;

        // If custom background is loaded, do not switch images
        bool    m_customBackground;

        // If the playback is paused, stop the animation
        QAtomicInt  m_playbackPaused;
};

#endif // BACKGROUNDIMAGE_H
