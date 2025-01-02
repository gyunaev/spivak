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

#include <QString>

// Interface for libcld.so - it is 4Mb library, no need to keep it loaded all the time,
// as it is only used during scanning (and even there it is not really necessary)
// Separating this allows to load libcld.so only when neeeded on platforms with limited resources
class LanguageDetector
{
    public:
        // Returns the object if language detector is available, otherwise returns null
        static LanguageDetector * create();

        // Takes care about the language ID conversion too
        QString  detectLanguage( const QByteArray& data );

        // This returns all languages
        QStringList  languages();

    private:
        LanguageDetector();
};

#endif // LANGUAGEDETECTOR_H
