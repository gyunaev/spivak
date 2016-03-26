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

#ifndef MULTIMEDIATESTWIDGET_H
#define MULTIMEDIATESTWIDGET_H

#include <QWidget>
#include <QTemporaryFile>

#include "mediaplayer.h"

namespace Ui {
class MultimediaTestWidget;
}

class MultimediaTestWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit MultimediaTestWidget(QWidget *parent);
        ~MultimediaTestWidget();

    private slots:
        // Player slots
        void	error( QString error );
        void    loaded();

        void    startTest();

        // Tests whether we can play WAV
        void    testNext();

    private:
        void    show();
        void    append( const QString& text, bool newparagraph = true );
        bool    playerLoadResourceFile( const QString& filename );

        Ui::MultimediaTestWidget *ui;

        // Here we use just QMediaPlayer to get deeper access to debugging facilities
        MediaPlayer     m_player;

        // Files to test play, and current testing
        QStringList     m_testExtensions;
        unsigned int    m_currentTesting;

        // Extension splayable
        QStringList     m_playableFormats;

        // Suppress further errors
        bool            m_suppressErrors;
};

#endif // MULTIMEDIATESTWIDGET_H
