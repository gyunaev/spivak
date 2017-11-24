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

#ifndef PLAYERNOTIFICATION_H
#define PLAYERNOTIFICATION_H

#include <QList>
#include <QMutex>
#include <QTime>
#include <QString>

#include "karaokepainter.h"

class Background;

// Renders a notification on top of the screen
class Notifications : public QObject
{
    Q_OBJECT

    public:
        Notifications( QObject * parent );
        ~Notifications();

        // Draws the notification on the painter. Returns time for the next update
        // when the image will change.
        qint64  drawTop(KaraokePainter& p);

        // Draw the regular notification
        qint64  drawRegular( KaraokePainter& p );

    public slots:
        void    queueUpdated();

        // Received from notification handler; to pause the animation on background
        void    songStarted();
        void    songStopped();

        // Action settings changed (incl. Internet), so maybe the message might change too
        void    settingsChanged();

        void    setOnScreenMessage( const QString& message );
        void    clearOnScreenMessage();

        void    showMessage( const QString& message, int show = 5000 );
        void    showMessage( int percentage, const QString& message, int show = 5000 );

    private slots:
        void    webServerUrlChanged( QString newurl );

    private:
        void    reset();

        // This doesn't lock mutex, so should be only called internally
        void    updateWelcomeMessage();

    private:
        QFont           m_font;
        int             m_lastScreenHeight;

        QString         m_firstItem;
        QString         m_notificationLine;
        QString         m_textQueueSize;

        int             m_scrollOffset;
        int             m_scrollStep;

        // Custom messages
        QMutex          m_mutex;
        QFont           m_customFont;
        QString         m_customMessage;
        double          m_customMessageAnimationValue;
        double          m_customMessageAnimationAdder;
        bool            m_karaokePlaying;

        // Web server URL
        QString         m_webserverURL;

        QString         m_smallMessage;
        int             m_smallPercentage;
        QTime           m_smallMessageExpires;
};

extern Notifications * pNotification;

#endif // PLAYERNOTIFICATION_H
