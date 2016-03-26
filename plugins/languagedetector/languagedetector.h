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

#ifndef LANGUAGEDETECTOR_H
#define LANGUAGEDETECTOR_H

#include <QObject>
#include <QString>
#include "../../src/interface_languagedetector.h"

// Interface for libcld.so - it is 4Mb library, no need to keep it loaded all the time,
// as it is only used during scanning (and even there it is not really necessary)
class LanguageDetector : public QObject, public Interface_LanguageDetector
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "com.ulduzsoft.Skivak.Plugin.LanguageDetector" )
    Q_INTERFACES( Interface_LanguageDetector )

    public:
        LanguageDetector();

        // Takes care about the language ID conversion too
        QString  detectLanguage( const QByteArray& data ) Q_DECL_OVERRIDE;

        // This returns all languages
        QStringList  languages() Q_DECL_OVERRIDE;
};

#endif // LANGUAGEDETECTOR_H
