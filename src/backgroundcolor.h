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

#ifndef BACKGROUNDCOLOR_H
#define BACKGROUNDCOLOR_H

#include <QColor>

#include "background.h"

class BackgroundColor : public Background
{
    public:
        BackgroundColor();

        // Background could be initialized either from the settings (by callign initFromSettings() or from
        // a specific file/QIODevice - for example for KFN files (the file name needs to be passed to know which type
        // of file is being loaded). If the background cannot be initialized, it must return an error.
        virtual bool    initFromSettings();
        virtual bool    initFromFile( QIODevice * file, const QString& filename );

        // Draws the background on the image; the prior content of the image is undefined. If false is returned,
        // it is considered an error, and the whole playing process is aborted - use wisely
        qint64   draw( KaraokePainter& p );

    private:
        QColor  m_color;
};

#endif // PLAYERBACKGROUNDCOLOR_H
