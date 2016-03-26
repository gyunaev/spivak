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

#ifndef FEEDBACKDIALOG_H
#define FEEDBACKDIALOG_H

#include <QNetworkAccessManager>
#include <QDialog>
#include <QTimer>

namespace Ui {
class FeedbackDialog;
}

class FeedbackDialog : public QDialog
{
    Q_OBJECT

    public:
        // Turns on feedback sending mode or crash reporting mode depending on 2nd param
        explicit FeedbackDialog(QWidget *parent = 0 , QString crashPath = "");
        ~FeedbackDialog();

    private slots:
        void    btnBrowse();
        void    restartPlayer();
        void    accept();
        void    sendFinished( QNetworkReply *reply );
        void    sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
        void    updateSendingFeedback();
        void    showPrivacyPolicy();

    private:
        void appendPart(QHttpMultiPart *form, const QString &name, const QString &value);

        Ui::FeedbackDialog *ui;
        QString     m_attachment;
        QNetworkAccessManager m_networkManager;
        bool        m_crashMode;

        // Kind-of animation while sending the feedback
        QString     m_sendingDots;
        QTimer      m_sendingTimer;
};

#endif // FEEDBACKDIALOG_H
