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

#include <QProcess>
#include <QFileDialog>
#include <QNetworkReply>
#include <QButtonGroup>
#include <QMessageBox>
#include <QHttpPart>
#include <QWhatsThis>

#include "feedbackdialog.h"
#include "ui_feedbackdialog.h"

FeedbackDialog::FeedbackDialog(QWidget *parent, QString crashPath )
    : QDialog(parent), ui(new Ui::FeedbackDialog)
{
    ui->setupUi(this);
    m_attachment = crashPath;

    // If we have attachment, we're submitting a crash
    m_crashMode = crashPath.isEmpty() ? false : true;

    connect( ui->btnAttach, &QPushButton::clicked, this, &FeedbackDialog::btnBrowse );

    if ( m_attachment.isEmpty() )
    {
        setWindowTitle( tr("Sending the feedback"));

        ui->lblInfo->setText( tr("<qt>Please enter the application feedback below. It will not be published anywhere and will go directly to the developers."
                                 "<p>You can also attach a file using the button below if needed. Include your e-mail address if you want us to reach out to you."
                                 "<p>If you care, here is our <a href='#'>Privacy policy</a></qt>") );
    }
    else
    {
        setWindowTitle( tr("Sending the crash information"));

        ui->lblInfo->setText( tr("<qt><b>OMG!!! It can't be!!! Unfortunately the application has crashed :(</b><p>We apologize for inconvenience, and ask you to send the crash information to us which would help us fix it."
                                 "<p>If you can, please explain below which actions led to the crash, and if you also include your e-mail address,<br>this would let us reach out for you to let you know it is fixed. If you care, here is our <a href='#'>Privacy policy</a>") );

        ui->lblAttachedFile->setText( tr("Attached: %1") .arg(m_attachment) );
        ui->btnAttach->setEnabled( false );

        // Add a button to restart the player
        connect( ui->buttonBox->addButton( tr("Restart player"), QDialogButtonBox::ResetRole ), &QPushButton::clicked, this, &FeedbackDialog::restartPlayer );
    }

    connect( ui->lblInfo, SIGNAL(linkActivated(QString)), this, SLOT(showPrivacyPolicy()) );
    ui->buttonBox->button( QDialogButtonBox::Ok )->setText( tr("Send the feedback") );
    connect( ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &FeedbackDialog::reject );

    connect( &m_sendingTimer, SIGNAL(timeout()), this, SLOT(updateSendingFeedback()) );
}

FeedbackDialog::~FeedbackDialog()
{
    delete ui;
}

void FeedbackDialog::btnBrowse()
{
    m_attachment = QFileDialog::getOpenFileName( 0, tr("Attach a file") );

    if ( m_attachment.isEmpty() )
        ui->lblAttachedFile->setText( tr("No file attached") );
    else
        ui->lblAttachedFile->setText( tr("Attached: %1") .arg( QFileInfo(m_attachment).fileName() ) );
}

void FeedbackDialog::restartPlayer()
{
    QProcess::startDetached( QCoreApplication::applicationFilePath() );
}

void FeedbackDialog::appendPart(QHttpMultiPart *form, const QString &name, const QString &value)
{
    QHttpPart part;

    part.setHeader( QNetworkRequest::ContentTypeHeader, QVariant("text/plain") );
    part.setHeader( QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"%1\"") .arg( name ) );
    part.setBody( value.toUtf8() );

    form->append( part );
}


void FeedbackDialog::accept()
{
    QHttpMultiPart *multiPart = new QHttpMultiPart( QHttpMultiPart::FormDataType );

    // Form fields
    appendPart( multiPart, "app", "karaokeplayer" );
    appendPart( multiPart, "type", ui->btnAttach->isEnabled() ? "feedback" : "crash" );

    // In case of crash submission the user might be lazy enough to write anything
    if ( !ui->leFeedback->toPlainText().isEmpty() )
        appendPart( multiPart, "desc", ui->leFeedback->toPlainText() );
    else
        appendPart( multiPart, "desc", "No description provided" );

    // Attach the file Append the file attachment if present
    if ( !m_attachment.isEmpty() )
    {
        // Get the attachment file name
        QString filename = QFileInfo(m_attachment).fileName();

        // And open it
        QFile * attach = new QFile( m_attachment );

        if ( !attach->open( QIODevice::ReadOnly ) )
        {
            QMessageBox::critical( 0,
                                   tr("Failed to open attachment"),
                                   tr("Cannot open the attached file %1: %2") .arg(m_attachment) .arg(attach->errorString()) );
            delete multiPart;
            return;
        }

        QHttpPart filePart;
        filePart.setHeader( QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream") );
        filePart.setHeader( QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"file\"; filename=\"%1\"").arg( filename ) );
        filePart.setBodyDevice( attach  );

        // we cannot delete the file now, so delete it with the multiPart
        attach->setParent( multiPart );

        multiPart->append( filePart );
    }

    connect( &m_networkManager, &QNetworkAccessManager::finished, this, &FeedbackDialog::sendFinished );
    connect( &m_networkManager, &QNetworkAccessManager::sslErrors, this, &FeedbackDialog::sslErrors );

    // Submit the POST request
    QUrl url = QSslSocket::supportsSsl() ? QUrl("https://www.ulduzsoft.com/submit.php") : QUrl("http://www.ulduzsoft.com/submit.php");
    QNetworkRequest request;
    request.setUrl( url );

    QNetworkReply *reply = m_networkManager.post( request, multiPart );
    multiPart->setParent( reply ); // delete the multiPart with the reply

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

    m_sendingDots = "...";
    updateSendingFeedback();
    m_sendingTimer.start( 500 );
}

void FeedbackDialog::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    // This is necessary as we're using a self-signed cert
    if ( !errors.isEmpty() )
    {
        if ( errors.first().error() == QSslError::SelfSignedCertificate )
        {
            reply->ignoreSslErrors();
            return;
        }
    }
}

void FeedbackDialog::updateSendingFeedback()
{
    ui->lblAttachedFile->setText( tr("Sending the feedback%1") .arg(m_sendingDots) );

    m_sendingDots.append( '.' );

    if ( m_sendingDots.length() > 20 )
        m_sendingDots = "...";
}

void FeedbackDialog::showPrivacyPolicy()
{
    QWhatsThis::showText ( mapToGlobal( ui->leFeedback->pos() ), ui->leFeedback->whatsThis(), this );
}

void FeedbackDialog::sendFinished( QNetworkReply *reply )
{
    m_sendingTimer.stop();

    if ( reply->error() != QNetworkReply::NoError )
    {
        QMessageBox::critical( 0,
                               tr("Failed to send the feedback"),
                               tr("Failed to send the feedback: %1") .arg( reply->errorString() ) );
        return;
    }

    QByteArray response = reply->readAll();

    if ( !response.startsWith( "200" ) )
    {
        QMessageBox::critical( 0,
                               tr("Failed to send the feedback"),
                               tr("Server error processing the feedback: %1") .arg( QString::fromUtf8( response  ) ) );
        return;
    }

    if ( m_crashMode )
    {
        ui->lblAttachedFile->setText( tr("Feedback successfully sent") );

        // And clean up after ourselves
        QFile::remove( m_attachment );
    }
    else
        QDialog::accept();
}
