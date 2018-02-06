#include <QJsonValue>

#include "collectionentry.h"

CollectionEntry::CollectionEntry()
{
    id = 0;
    priority = 0;
    type = CollectionProvider::TYPE_FILESYSTEM;
    detectLanguage = false;
    scanZips = false;
    lastScanned = -1;
    ignoreSSLerrors = false;
}

QJsonObject CollectionEntry::toJson() const
{
    QJsonObject out;

    out[ "id" ] = id;
    out[ "priority" ] = priority;
    out[ "type" ] = type == CollectionProvider::TYPE_FILESYSTEM ? "fs" : "http";
    out[ "root" ] = rootPath;
    out[ "name" ] = name;

    if ( !authuser.isEmpty() && !authpass.isEmpty() )
    {
        out[ "authuser" ] = authuser;
        out[ "authpass" ] = authpass;
    }


    out[ "ignoreSSLerrors" ] = ignoreSSLerrors;
    out[ "detectLanguage" ] = detectLanguage;
    out[ "scanZips" ] = scanZips;
    out[ "artistTitleSeparator" ] = artistTitleSeparator;

    if ( !defaultLanguage.isEmpty() )
        out[ "defaultLanguage" ] = defaultLanguage;

    if ( lastScanned != -1 )
        out[ "lastScanned" ] = QString::number( (long long) lastScanned );

    return out;
}

void CollectionEntry::fromJson(const QJsonObject &data)
{
    id = data.value( "id" ).toInt( 0 );
    name = data.value( "name" ).toString();
    priority = data.value( "priority" ).toInt( 0 );
    type  = data.value( "type" ).toString( "fs" ) == "fs" ? CollectionProvider::TYPE_FILESYSTEM : CollectionProvider::TYPE_HTTP;
    rootPath = data.value( "root" ).toString();
    authuser = data.value( "authuser" ).toString();
    authpass = data.value( "authpass" ).toString();
    detectLanguage = data.value( "detectLanguage" ).toBool( false );
    scanZips = data.value( "scanZips" ).toBool( false );
    artistTitleSeparator = data.value( "artistTitleSeparator" ).toString();
    defaultLanguage = data.value( "defaultLanguage" ).toString();
    lastScanned = data.value( "lastScanned" ).toString("-1").toLongLong();
    ignoreSSLerrors = data.value( "ignoreSSLerrors" ).toBool( false );
}
