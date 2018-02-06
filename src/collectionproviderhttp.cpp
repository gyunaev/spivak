#include <QAuthenticator>

#include "collectionproviderhttp.h"
#include "settings.h"
#include "logger.h"


CollectionProviderHTTP::CollectionProviderHTTP(int id, QObject *parent)
    : CollectionProvider(id, parent), m_qnam(parent)
{
    m_credentialsSent = false;
}

bool CollectionProviderHTTP::isLocalProvider() const
{
    return false;
}

CollectionProvider::Type CollectionProviderHTTP::type() const
{
    return TYPE_HTTP;
}

void CollectionProviderHTTP::retrieveMultiple(int id, const QList<QString> &urls, QList<QIODevice *> files)
{
    // This would definitely be invalid usage
    if ( files.size() != urls.size() )
        abort();

    m_id = id;

    for ( int i = 0; i < files.size(); i++ )
    {
        RequestData reqdata;

        reqdata.file = files[i];
        reqdata.url = urls[i];
        reqdata.bytesReceived = 0;
        reqdata.bytesTotal = 0;
        reqdata.finished = false;

        // This starts the request and uses the main event loop
        reqdata.reply = m_qnam.get( QNetworkRequest( QUrl(urls[i]) ) );

        m_state[ reqdata.reply ] = reqdata;

        connect( reqdata.reply, &QNetworkReply::finished, this, &CollectionProviderHTTP::httpFinished);
        connect( reqdata.reply, &QIODevice::readyRead, this, &CollectionProviderHTTP::httpReadyRead);
        connect( reqdata.reply, &QNetworkReply::downloadProgress, this, &CollectionProviderHTTP::httpProgress );
        connect( &m_qnam, &QNetworkAccessManager::authenticationRequired, this, &CollectionProviderHTTP::httpAuthenticationRequired );
        connect( &m_qnam, &QNetworkAccessManager::sslErrors, this, &CollectionProviderHTTP::httpsslErrors );

        Logger::debug( "Initiating download for %s into %p (nr %p)", qPrintable( reqdata.url), reqdata.file, reqdata.reply);
    }
}

void CollectionProviderHTTP::cleanup()
{
    Q_FOREACH( const RequestData& d, m_state )
    {
        d.reply->deleteLater();
    }

    m_state.clear();
}

void CollectionProviderHTTP::httpFinished()
{
    // If we're cleared up already this means we are done - those are stale events
    if ( m_state.isEmpty() )
        return;

    QNetworkReply * reply = (QNetworkReply *) sender();

    Logger::debug( "HTTP finished for %p", reply );

    if ( reply->error() )
    {
        Logger::error( "HTTP download for %s finished with error %s",
                       qPrintable( m_state[ reply ].url ),
                       qPrintable( reply->errorString() ) );

        m_state[ reply ].finished = true;

        // Finish the thread
        emit finished( m_id, reply->errorString() );
        return;
    }

    // Non-error - set the finished flag
    m_state[ reply ].finished = true;

    m_state[ reply ].file->close();
    // Is everything finished?
    Q_FOREACH( const RequestData& d, m_state )
    {
        if ( !d.finished )
            return;
    }

    // All finished, we are done in this thread
    emit finished( m_id, QString() );
}

void CollectionProviderHTTP::httpReadyRead()
{
    // If we're cleared up already this means we are done - those are stale events
    if ( m_state.isEmpty() )
        return;

    QNetworkReply * reply = (QNetworkReply *) sender();

    // Read and write everything
    m_state[ reply ].file->write( reply->readAll() );
}

void CollectionProviderHTTP::httpProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    // If we're cleared up already this means we are done - those are stale events
    if ( m_state.isEmpty() )
        return;

    QNetworkReply * reply = (QNetworkReply *) sender();

    m_state[ reply ].bytesReceived = bytesReceived;
    m_state[ reply ].bytesTotal = bytesTotal;

    qint64 alltotal = 0, allreceived = 0;

    // Record the received progress
    Q_FOREACH( const RequestData& d, m_state )
    {
        // If we have never received total, it is zero and we cannot calculate,
        // thus we ignore it
        if ( d.bytesTotal == 0 )
            return;

        // If total is -1 this means we will not know total. In this case we
        // just flip between 25% and 75% each time we get this call
        if ( d.bytesTotal == -1 )
        {
            Logger::debug( "Progress cant be calculated" );
            emit progress( m_id, -1 );
            return;
        }

        alltotal += d.bytesTotal;
        allreceived += d.bytesReceived;
    }

    // If we're here everything is right, so calculate the percentage
    Logger::debug( "Calculated download average %d", allreceived * 100 / alltotal );
    emit progress( m_id, allreceived * 100 / alltotal );
}

void CollectionProviderHTTP::httpAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    if ( collectionID >= 0 && !m_credentialsSent )
    {
        authenticator->setUser( pSettings->collections[collectionID].authuser );
        authenticator->setPassword( pSettings->collections[collectionID].authpass );
        m_credentialsSent = true;
    }
}

void CollectionProviderHTTP::httpsslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    // Only go through this if the ignore checkbox is set
    if ( m_id < 0 || !pSettings->collections[collectionID].ignoreSSLerrors )
        return;

    Q_FOREACH ( const QSslError& err, errors )
    {
        switch ( err.error() )
        {
            case QSslError::SelfSignedCertificate:
            case QSslError::SelfSignedCertificateInChain:
            case QSslError::HostNameMismatch:
                break;

            // For all other errors we return so ignore isn't caller
            default:
                qDebug("not ignoring %d", err.error() );
                return;
        }
    }

    reply->ignoreSslErrors();
}
