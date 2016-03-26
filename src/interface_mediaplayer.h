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

#ifndef INTERFACE_MEDIAPLAYER_H
#define INTERFACE_MEDIAPLAYER_H

#include <QString>
#include <QtPlugin>


// Those plugins implement GStreamer plugins internally
class Interface_MediaPlayerPlugin
{
    public:
        // Initializes the plugin; since it is linked statically internally
        // See "Embedding static elements in your application" in GStreamer manual
        virtual bool    init() = 0;

        // Returns the element name which this plugin implements (which should be created via factory)
        virtual const char* elementName() = 0;

        // Returns the media player capability implemented by this plugin, if any
        virtual int     capability() = 0;

        // Returns the parameter name which is used to change the capability.
        // It should accept changes via g_object_set()
        virtual const char* parameterName() = 0;
};

#define IID_InterfaceMediaPlayerPlugin  "com.ulduzsoft.Skivak.Plugin.MediaPlayer-v1"

Q_DECLARE_INTERFACE( Interface_MediaPlayerPlugin, IID_InterfaceMediaPlayerPlugin )


#endif // INTERFACE_MEDIAPLAYER_H
