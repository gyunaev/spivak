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

#ifndef PLUGIN_LANGUAGEDETECTOR_H
#define PLUGIN_LANGUAGEDETECTOR_H

#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QtPlugin>

class Interface_LanguageDetector
{
    public:
        // Returns proper full language name according to ISO
        virtual QString  detectLanguage( const QByteArray& data ) = 0;

        // Returns all supported language names
        virtual QStringList  languages() = 0;
};

#define IID_InterfaceLanguageDetector   "com.ulduzsoft.Skivak.Plugin.LanguageDetector"

Q_DECLARE_INTERFACE( Interface_LanguageDetector, IID_InterfaceLanguageDetector )

#endif // PLUGIN_LANGUAGEDETECTOR_H
