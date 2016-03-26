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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QString>

#include "interface_languagedetector.h"
#include "interface_mediaplayer.h"

// Loads and keeps state of loaded plugins
class PluginManager
{
    public:
        PluginManager( const QString& pluginPath = "" );

        // This plugin is loaded and unloaded on demand
        Interface_LanguageDetector * loadLanguageDetector();
        void releaseLanguageDetector();

        // This plugin is only loaded, no unload
        Interface_MediaPlayerPlugin * loadPitchChanger();

    private:
        // This function is used to load plugins with any interface, and auto-cast
        // return pointer to the interface
        template <class T> T * loadPlugin( const char * name );        

        void    releasePlugin( const QString& name );

        QString     m_pluginPath;
};

extern PluginManager * pPluginManager;

#endif // PLUGINMANAGER_H
