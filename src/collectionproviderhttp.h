#ifndef COLLECTIONPROVIDERHTTP_H
#define COLLECTIONPROVIDERHTTP_H

#include <QMap>
#include <QThread>
#include <QIODevice>
#include <QAtomicInt>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "collectionprovider.h"

class CollectionProviderHTTP : public CollectionProvider
{
    //Q_OBJECT

    public:
        CollectionProviderHTTP(int id, QObject * parent);

        // Returns true if this provider is based on file system, and the file operations
        // are supported directly.
        virtual bool    isLocalProvider() const;

        // Returns the provider type
        virtual Type type() const;

    protected:
        // Implemented in subclasses. Should download one or more files syncrhonously,
        // and return when everything is downloaded. With non-zero ID should also
        // call progress and finished callbacks (no callbacks with zero ID)
        virtual void retrieveMultiple( int id, const QList<QString>& urls, QList<QIODevice *> files );

    private slots:
        void    httpFinished();
        void    httpReadyRead();
        void    httpProgress( qint64 bytesReceived, qint64 bytesTotal );
        void    httpAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
        void    httpsslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

    private:
        void    cleanup();

        class RequestData
        {
            public:
                QNetworkReply * reply;
                QIODevice * file;
                QString     url;
                qint64      bytesReceived;
                qint64      bytesTotal;
                bool        finished;
        };

        int m_id;
        QNetworkAccessManager m_qnam;
        QMap< QNetworkReply *, RequestData >  m_state;
        bool    m_credentialsSent;
};

#endif // COLLECTIONPROVIDERHTTP_H
