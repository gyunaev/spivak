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

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>

#include "pluginmanager.h"
#include "interface_languagedetector.h"
#include "songdatabasescanner.h"
#include "actionhandler.h"
#include "currentstate.h"
#include "settingsdialog.h"
#include "settings.h"
#include "eventor.h"
#include "logger.h"
#include "database.h"
#include "mainwindow.h"
#include "multimediatestwidget.h"
#include "karaokepainter.h"
#include "ui_settingsdialog.h"


static const char * coltemplatelist[] = { "/", " - ", " = ", 0 };


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::SettingsDialog)
{
    setAttribute( Qt::WA_DeleteOnClose );
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex( 0 );

    connect( ui->boxBackgroundType, SIGNAL(currentIndexChanged(int)), this, SLOT(backgroundChanged(int)) );
    connect( ui->boxCollectionDetectLang, SIGNAL(toggled(bool)), this, SLOT(collectionLanguageCheckboxChanged()) );
    connect( ui->boxCollectionSeparator, SIGNAL(activated(int)), this, SLOT(collectionSeparatorChanged()) );

    connect( ui->btnBrowseCollection, SIGNAL(clicked()), this, SLOT(browseCollection()) );
    connect( ui->leArtistTitleTest, SIGNAL(textChanged(QString)), this, SLOT(collectionTestArtistTitle( QString )) );
    connect( ui->btnBrowseImages, SIGNAL(clicked()), this, SLOT(browseBackgroundImages()) );
    connect( ui->btnBrowseVideos, SIGNAL(clicked()), this, SLOT(browseBackgroundVideos()) );
    connect( ui->btnBrowseLIRCmapping, SIGNAL(clicked()), this, SLOT(browseLIRCmapping()) );
    connect( ui->boxLIRCenable, SIGNAL(toggled(bool)), this, SLOT(lircChanged()) );
    connect( ui->btnBrowseMusic, SIGNAL(clicked()), this, SLOT(browseMusicPath()) );

    connect( ui->btnUpdateCollection, &QPushButton::clicked, this, &SettingsDialog::startUpdateDatabase );
    connect( ui->btnEraseCollection, &QPushButton::clicked, this, &SettingsDialog::eraseDatabase );

    connect( ui->boxCollectionTypeFS, &QRadioButton::clicked, this, &SettingsDialog::collectionTypeChanged );
    connect( ui->boxCollectionTypeHTTP, &QRadioButton::clicked, this, &SettingsDialog::collectionTypeChanged );

    // Prepare the languages if the interface is available
    m_langdetector = pPluginManager->loadLanguageDetector();

    ui->boxCollectionLanguage->addItem( "No language" );

    // Language detection plugin is optional, and might not be present
    if ( m_langdetector )
    {
        ui->boxCollectionLanguage->addItems( m_langdetector->languages() );
    }
    else
    {
        ui->boxCollectionDetectLang->setChecked( false );
        ui->boxCollectionDetectLang->setEnabled( false );
        ui->boxCollectionLanguage->setEnabled( false );
    }

    //
    // Init the parameters
    //

    //
    // Collection tab
    //

    // Common separator values are hardcoded
    for ( int i = 0; coltemplatelist[i]; i++ )
        ui->boxCollectionSeparator->addItem( collectionSeparatorString( coltemplatelist[i] ), coltemplatelist[i] );

    ui->boxCollectionSeparator->addItem( tr("Add a new separator..." ) );

    if ( !pSettings->collections.isEmpty() )
    {
        const CollectionEntry& col = pSettings->collections.first();

        if ( col.type == CollectionProvider::TYPE_FILESYSTEM )
        {
            ui->boxCollectionTypeFS->setChecked( true );
            ui->boxCollectionDetectLang->setChecked( col.detectLanguage );
        }
        else
        {
            ui->boxCollectionTypeHTTP->setChecked( true );
            ui->leHTTPuser->setText( col.authuser );
            ui->leHTTPpass->setText( col.authpass );
            ui->boxIgnoreSSLerrors->setChecked( col.ignoreSSLerrors );
        }

        ui->leCollectionName->setText( col.name );
        ui->leCollectionPath->setText( col.rootPath );
        ui->boxColelctionZIP->setChecked( col.scanZips );

        if ( !col.defaultLanguage.isEmpty() && m_langdetector )
            ui->boxCollectionLanguage->setCurrentText( col.defaultLanguage );

        // Is current separator there?
        if ( ui->boxCollectionSeparator->findData( col.artistTitleSeparator ) == -1 )
            ui->boxCollectionSeparator->insertItem( ui->boxCollectionSeparator->count() - 1, collectionSeparatorString(col.artistTitleSeparator), col.artistTitleSeparator );

        // And show it as current
        ui->boxCollectionSeparator->setCurrentIndex( ui->boxCollectionSeparator->findData( col.artistTitleSeparator  ) );
    }

    // Modify the text strings
    collectionTypeChanged();

    // Show the database information
    refreshDatabaseInformation();

    // and connect collection scan slots
    connect( pEventor, &Eventor::scanCollectionProgress, this, &SettingsDialog::scanCollectionProgress, Qt::QueuedConnection );
    connect( pEventor, &Eventor::scanCollectionFinished, this, &SettingsDialog::refreshDatabaseInformation, Qt::QueuedConnection );

    //
    // Background tab
    switch ( pSettings->playerBackgroundType )
    {
    case Settings::BACKGROUND_TYPE_NONE:
    case Settings::BACKGROUND_TYPE_COLOR:
        ui->boxBackgroundType->setCurrentIndex( 0 );
        ui->btnBackgroundColor->setColor( pSettings->playerBackgroundColor );
        break;

    case Settings::BACKGROUND_TYPE_IMAGE:
        ui->boxBackgroundType->setCurrentIndex( 1 );

        if ( !pSettings->playerBackgroundObjects.isEmpty() )
            ui->lePathImages->setText( pSettings->playerBackgroundObjects[0] );

        if ( pSettings->playerBackgroundTransitionDelay > 0 )
        {
            ui->spinBackgroundDelay->setValue( pSettings->playerBackgroundTransitionDelay );
            ui->boxBackgroundTransitions->setChecked( true );
        }
        break;

    case Settings::BACKGROUND_TYPE_VIDEO:
        ui->boxBackgroundType->setCurrentIndex( 2 );

        if ( !pSettings->playerBackgroundObjects.isEmpty() )
            ui->lePathVideos->setText( pSettings->playerBackgroundObjects[0] );

        break;
    }

    ui->boxCDGtransparent->setChecked( pSettings->playerCDGbackgroundTransparent );

    if ( pSettings->playerIgnoreBackgroundFromFormats )
        ui->rbBackgroundIgnoreCustom->setChecked( true );
    else
        ui->rbBackgroundUseCustom->setChecked( true );

    //
    // Player tab
    ui->fontPlayerLyrics->setFont( pSettings->playerLyricsFont );
    ui->spinMaxLyricFontLines->setValue( pSettings->playerLyricsFontFitLines );
    ui->btnPlayerLyricsFuture->setColor( pSettings->playerLyricsTextAfterColor );
    ui->btnPlayerLyricsPast->setColor( pSettings->playerLyricsTextBeforeColor );
    ui->btnPlayerLyricsInfo->setColor( pSettings->notificationCenterColor );
    ui->btnPlayerLyricsSpotlight->setColor( pSettings->playerLyricsTextSpotColor );
    ui->btnPlayerLyricsNotification->setColor( pSettings->notificationTopColor );
    ui->lyricBackgroundTransparency->setValue( pSettings->playerLyricBackgroundTintPercentage );

    ui->spinLyricDelay->setValue( pSettings->playerMusicLyricDelay );

    //
    // Music tab
    if ( !pSettings->musicCollections.isEmpty() )
    {
        ui->boxMusicEnable->setChecked(true);

        ui->leMusicPath->setText( pSettings->musicCollections[0] );

        if ( pSettings->musicCollectionCrossfadeTime > 0 )
        {
            ui->boxMusicCrossfade->setChecked( true );
            ui->spinMusicCrossfadeDelay->setValue( pSettings->musicCollectionCrossfadeTime );
        }
        else
            ui->boxMusicCrossfade->setChecked( false );

        if ( pSettings->musicCollectionSortedOrder )
            ui->rbMusicSortOrder->setChecked( true );
        else
            ui->rbMusicRandomOrder->setChecked( true );
    }
    else
        ui->boxMusicEnable->setChecked(false);

    //
    // Misc tab
    if ( pSettings->queueAddNewSingersNext )
        ui->rbQueueNewStart->setChecked( true );
    else
        ui->rbQueueNewEnd->setChecked( true );

    ui->boxSaveQueueOnExit->setChecked( pSettings->queueSaveOnExit );

    // Web server
    ui->boxWebEnable->setChecked( pSettings->httpEnabled );
    ui->boxWebAllowAddSong->setChecked( pSettings->httpEnableAddQueue );

    // LIRC
#ifdef Q_OS_WIN
    // Lirc on Windows has predefined domain
    ui->leLIRCdevice->setText( "localhost:8765" );
#else
    ui->leLIRCdevice->setText( pSettings->lircDevicePath );
#endif

    ui->boxLIRCenable->setChecked( pSettings->lircEnabled );
    ui->leLIRCmapping->setText( pSettings->lircMappingFile );
    lircChanged();

    // Full screen
    ui->boxForceFullScreen->setChecked( pSettings->startInFullscreen );

    // MIDI
    if ( pSettings->playerUseBuiltinMidiSynth )
        ui->rbMidiBuiltin->setChecked( true );
    else
        ui->rbMidiNative->setChecked( true );

    // Add multimedia test widget
    MultimediaTestWidget * mt = new MultimediaTestWidget(this);
    ui->tabWidget->addTab( mt, "Sound test");

    // Update various handlers
    backgroundChanged(0);
    collectionLanguageCheckboxChanged();

    if ( pMainWindow->karaokeDatabaseIsScanning() )
    {
        ui->btnUpdateCollection->setText( tr("Stop") );
        ui->btnEraseCollection->setEnabled( false );
    }

    // Animation parameters
    m_lyricAnimationValue = 0;
    m_lyricAnimationTimer.setInterval( 150 );
    connect( &m_lyricAnimationTimer, SIGNAL(timeout()), this, SLOT(updateLyricsPreview()) );
    m_lyricAnimationTimer.start();
}

SettingsDialog::~SettingsDialog()
{
    // If we loaded the language detector, release it
    if ( m_langdetector )
        pPluginManager->releaseLanguageDetector();

    delete ui;
}

void SettingsDialog::backgroundChanged(int )
{
    switch ( ui->boxBackgroundType->currentIndex() )
    {
    case 0:
        ui->groupBackgroundImage->hide();
        ui->groupBackgroundVideo->hide();
        ui->groupBackgroundColor->show();
        break;

    case 1:
        ui->groupBackgroundColor->hide();
        ui->groupBackgroundVideo->hide();
        ui->groupBackgroundImage->show();
        break;

    case 2:
        ui->groupBackgroundColor->hide();
        ui->groupBackgroundImage->hide();
        ui->groupBackgroundVideo->show();
        break;
    }
}

void SettingsDialog::collectionLanguageCheckboxChanged()
{
    if ( ui->boxCollectionDetectLang->isChecked() )
        ui->lblCollectionLanguage->setText( tr("Set language if not detected to:") );
    else
        ui->lblCollectionLanguage->setText( tr("Make the language for all songs:") );
}

void SettingsDialog::browseCollection()
{
    QString e = QFileDialog::getExistingDirectory( 0, "Choose the karaoke root directory" );

    if ( e.isEmpty() )
        return;

    if ( e != ui->leCollectionPath->text() && !ui->leCollectionPath->text().isEmpty() )
        pDatabase->resetLastDatabaseUpdate();

    ui->leCollectionPath->setText( e );
}

void SettingsDialog::collectionTestArtistTitle(QString test)
{
    m_testFileName = test;
    updateCollectionTestOutput();
}

void SettingsDialog::browseBackgroundImages()
{
    QString e = QFileDialog::getExistingDirectory( 0, "Choose the background image root path" );

    if ( e.isEmpty() )
        return;

    ui->lePathImages->setText( e );
}

void SettingsDialog::browseBackgroundVideos()
{
    QString e = QFileDialog::getExistingDirectory( 0, "Choose the background videos root path" );

    if ( e.isEmpty() )
        return;

    ui->lePathVideos->setText( e );
}

void SettingsDialog::browseLIRCmapping()
{
    QString e = QFileDialog::getOpenFileName( 0, "Choose the LIRC mapping file" );

    if ( e.isEmpty() )
        return;

    ui->leLIRCmapping->setText( e );
}

void SettingsDialog::browseMusicPath()
{
    QString e = QFileDialog::getExistingDirectory( 0, "Choose the background videos root path" );

    if ( e.isEmpty() )
        return;

    ui->leMusicPath->setText( e );
}

void SettingsDialog::startUpdateDatabase()
{
    if ( !validateAndStoreCollection() )
        return;

    if ( pMainWindow->karaokeDatabaseIsScanning() )
    {
        if ( QMessageBox::question( 0, tr("Scan is in progress"),
                tr("The collection scan is already in progress. Press Yes if you want to abort it, and start a new scan.\n\nPress No if you want to wait until it finishes") ) != QMessageBox::Yes )
            return;

        pMainWindow->karaokeDatabaseAbortScan();
        refreshDatabaseInformation();
        return;
    }

    if ( QMessageBox::question( 0, tr("Update the collection?"),
            tr("Do you want to update the karaoke collection?\n\nThis will take some time. "
               "Existing songs will not be removed, nor would be the songs which have been deleted from disk.") ) != QMessageBox::Yes )
        return;

    if ( !pMainWindow->karaokeDatabaseStartScan() )
        return;

    ui->btnUpdateCollection->setText( tr("Stop") );
    ui->btnEraseCollection->setEnabled( false );   
}

void SettingsDialog::eraseDatabase()
{
    if ( QMessageBox::question( 0, tr("Erase the collection?"),
            tr("Do you want to DELETE ALL EXISTING SONGS and rescan the Karaoke collection?\n\nALL EXISTING SONGS AND ASSOCIATED INFORMATION WILL BE REMOVED") ) != QMessageBox::Yes )
        return;

    pDatabase->clearDatabase();
    refreshDatabaseInformation();
}

void SettingsDialog::scanCollectionProgress( QString progress )
{
    ui->lblCollectionInfo->setText( progress );
}

void SettingsDialog::refreshDatabaseInformation()
{
    if ( pCurrentState->m_databaseSongs == 0 )
    {
        ui->lblCollectionInfo->setText( tr("Database is empty") );
    }
    else
    {
        ui->lblCollectionInfo->setText( tr("Database: %1 songs, %2 artists, last updated %3")
                                        .arg( pCurrentState->m_databaseSongs )
                                        .arg( pCurrentState->m_databaseArtists )
                                        .arg( pCurrentState->m_databaseUpdatedDateTime ) );
    }

    // Enable the Erase button and change the update back
    ui->btnEraseCollection->setEnabled( true );
    ui->btnUpdateCollection->setText( tr("Update") );
}

void SettingsDialog::lircChanged()
{
    bool enable = ui->boxLIRCenable->isChecked();
    bool deviceenable = enable;

#ifdef Q_OS_WIN
    deviceenable = false;
#endif

    ui->leLIRCdevice->setEnabled( deviceenable );
    ui->leLIRCmapping->setEnabled( enable );
    ui->btnBrowseLIRCmapping->setEnabled( enable );
}

void SettingsDialog::updateLyricsPreview()
{
    QString sampleLine = tr("This is a test lyrics line");

    QImage img( ui->lblTextRenderingExample->size(), QImage::Format_RGB32 );
    img.fill( Qt::gray );
    KaraokePainter p( &img );

    // Draw the crosspattern rectangle as background
    p.fillRect( 0,
                0,
                ui->lblTextRenderingExample->size().width(),
                ui->lblTextRenderingExample->size().height(),
                QBrush( Qt::white, Qt::CrossPattern ) );

    // And tint it according to percentage
    QColor tint( 0, 0, 0, int( (double) ui->lyricBackgroundTransparency->value() * 2.55) );
    p.fillRect( 0,
                0,
                ui->lblTextRenderingExample->size().width(),
                ui->lblTextRenderingExample->size().height(),
                QBrush( tint ) );

    QFont font = ui->fontPlayerLyrics->font();
    font.setPointSize( p.tallestFontSize( font, img.height() ) );
    p.setFont( font );

    // Make copies of the values as we're replacing them temporarily
    QColor before = pSettings->playerLyricsTextBeforeColor;
    QColor after = pSettings->playerLyricsTextAfterColor;
    QColor spot = pSettings->playerLyricsTextSpotColor;

    pSettings->playerLyricsTextAfterColor = ui->btnPlayerLyricsFuture->color();
    pSettings->playerLyricsTextBeforeColor = ui->btnPlayerLyricsPast->color();
    pSettings->playerLyricsTextSpotColor = ui->btnPlayerLyricsSpotlight->color();

    p.drawOutlineTextGradient( (img.width() - p.fontMetrics().width( sampleLine )) / 2,
                               p.fontMetrics().ascent(),
                               m_lyricAnimationValue,
                               sampleLine );
    p.end();

    ui->lblTextRenderingExample->setPixmap( QPixmap::fromImage( img ) );

    pSettings->playerLyricsTextBeforeColor = before;
    pSettings->playerLyricsTextAfterColor = after;
    pSettings->playerLyricsTextSpotColor = spot;

    m_lyricAnimationValue += 0.05;

    if ( m_lyricAnimationValue >= 1.0 )
        m_lyricAnimationValue = 0.0;
}

bool SettingsDialog::validateAndStoreCollection()
{
    if ( ui->leCollectionPath->text().isEmpty() )
    {
        // Collection removed
        //TODO: erase songs
        if ( !pSettings->collections.isEmpty() )
            pSettings->collections.remove( pSettings->collections.first().id );

        return true;    // valid but empty
    }

    // If the collection list is empty, add the first one
    if ( pSettings->collections.isEmpty() )
    {
        // Use ID 1
        CollectionEntry col;
        col.id = 1;
        pSettings->collections[ col.id ] = col;
    }

    // Modify the first collection
    CollectionEntry& col = pSettings->collections.first();

    col.name = ui->leCollectionName->text();
    col.rootPath = ui->leCollectionPath->text();
    col.type = ui->boxCollectionTypeFS->isChecked()
            ? CollectionProvider::TYPE_FILESYSTEM : CollectionProvider::TYPE_HTTP;

    if ( col.type == CollectionProvider::TYPE_FILESYSTEM )
    {
        if ( !QFileInfo(col.rootPath).exists() )
        {
            QMessageBox::critical( 0, "The collection path does not exist", "Cannot use this collection - the path does not exist" );

            // Switch to our tab and focus on value
            ui->tabWidget->setCurrentIndex( 0 );
            ui->leCollectionPath->setFocus();
            return false;
        }

        col.detectLanguage = m_langdetector && ui->boxCollectionDetectLang->isChecked();
    }
    else
    {
        col.detectLanguage = false;
        col.authuser = ui->leHTTPuser->text();
        col.authpass = ui->leHTTPpass->text();
        col.ignoreSSLerrors = ui->boxIgnoreSSLerrors->isChecked();
    }

    col.scanZips = ui->boxColelctionZIP->isChecked();

    if ( ui->boxCollectionLanguage->currentIndex() > 0 && m_langdetector )
        col.defaultLanguage = ui->boxCollectionLanguage->currentText();
    else
        col.defaultLanguage.clear();

    col.artistTitleSeparator = ui->boxCollectionSeparator->currentData().toString();
    return true;
}

QString SettingsDialog::collectionSeparatorString(const QString &value)
{
    if ( value == "/" )
        return tr("Directory slash, such as A-HA%1Take on me.mp3").arg( QDir::separator() );
    else
        return tr("\"%1\", such as A-HA%1Take on me.mp3").arg( value );
}

void SettingsDialog::collectionSeparatorChanged()
{
    // If this is the last element, it allows us to enter a value
    if ( ui->boxCollectionSeparator->currentIndex() == ui->boxCollectionSeparator->count() - 1 )
    {
        QString newsep = QInputDialog::getText( 0, tr("Enter a separator"), tr("Separator characters:") );

        if ( newsep.isEmpty() )
        {
            ui->boxCollectionSeparator->setCurrentIndex( 0 );
            return;
        }

        // If we do not have this item already, add it to the box
        if ( ui->boxCollectionSeparator->findData( newsep ) == -1 )
            ui->boxCollectionSeparator->insertItem( ui->boxCollectionSeparator->count() - 1, collectionSeparatorString(newsep), newsep );

        ui->boxCollectionSeparator->setCurrentIndex( ui->boxCollectionSeparator->findData( newsep  ) );
    }

    updateCollectionTestOutput();
}

void SettingsDialog::collectionTypeChanged()
{
    if ( ui->boxCollectionTypeFS->isChecked() )
    {
        // Hide the authentication
        ui->lblHTTPauth->hide();
        ui->leHTTPuser->hide();
        ui->leHTTPpass->hide();
        ui->boxIgnoreSSLerrors->hide();

        // Show the language detection
        ui->boxCollectionDetectLang->show();
        ui->lblFSdetectLang->show();
        ui->lblRootDir->setText( tr("Root directory of the local &Karaoke collection:"));
    }
    else
    {
        // Show the authentication
        ui->lblHTTPauth->show();
        ui->leHTTPuser->show();
        ui->leHTTPpass->show();
        ui->boxIgnoreSSLerrors->show();

        // Hide the language detection
        ui->boxCollectionDetectLang->hide();
        ui->lblFSdetectLang->hide();
        ui->lblRootDir->setText( tr("Full URL of the &Karaoke collection:"));

    }
}

void SettingsDialog::updateCollectionTestOutput()
{
    if ( m_testFileName.isEmpty() )
    {
        ui->lblTestArtistTitle->setText( "Nothing entered" );
        return;
    }

    QString artist, title;

    if ( !SongDatabaseScanner::guessArtistandTitle( m_testFileName, ui->boxCollectionSeparator->currentData().toString(), artist, title ) || title.isEmpty() )
        ui->lblTestArtistTitle->setText( tr("Cannot detect artist/title in this file using the separator above") );
    else
        ui->lblTestArtistTitle->setText( tr("Artist: <b>%1</b>, Title: <b>%2</b>")
                                         .arg(artist.toHtmlEscaped()) .arg(title.toHtmlEscaped()) );
}


void SettingsDialog::accept()
{
    // Check the parameters first
    if ( !validateAndStoreCollection() )
        return;

    bool background_changed = false;

    // Background
    if ( ui->boxBackgroundType->currentIndex() == 0 )
    {
        if ( pSettings->playerBackgroundType != Settings::BACKGROUND_TYPE_COLOR )
            background_changed = true;

        pSettings->playerBackgroundType = Settings::BACKGROUND_TYPE_COLOR;
        pSettings->playerBackgroundColor = ui->btnBackgroundColor->color();
    }
    else if ( ui->boxBackgroundType->currentIndex() == 1 )
    {
        pSettings->playerBackgroundType = Settings::BACKGROUND_TYPE_IMAGE;

        if ( ui->lePathImages->text().isEmpty() )
        {
            // Warn the user and send the focus
            QMessageBox::critical( 0, tr("Background image path is empty"), tr("You must specify the background image path") );
            ui->tabWidget->setCurrentIndex( 1 );
            ui->lePathImages->setFocus();
            return;
        }

        // Remember the old one, we'll compare then against it
        QStringList oldpath = pSettings->playerBackgroundObjects;

        pSettings->playerBackgroundObjects.clear();
        pSettings->playerBackgroundObjects.push_back( ui->lePathImages->text() );

        if ( ui->boxBackgroundTransitions->isChecked() )
            pSettings->playerBackgroundTransitionDelay = ui->spinBackgroundDelay->value();
        else
            pSettings->playerBackgroundTransitionDelay = 0;

        if ( pSettings->playerBackgroundType != Settings::BACKGROUND_TYPE_IMAGE
        || oldpath != pSettings->playerBackgroundObjects )
            background_changed = true;
    }
    else if ( ui->boxBackgroundType->currentIndex() == 2 )
    {
        pSettings->playerBackgroundType = Settings::BACKGROUND_TYPE_VIDEO;

        if ( ui->lePathVideos->text().isEmpty() )
        {
            // Warn the user and send the focus
            QMessageBox::critical( 0, tr("Background video path is empty"), tr("You must specify the background video path") );
            ui->tabWidget->setCurrentIndex( 1 );
            ui->lePathVideos->setFocus();
            return;
        }

        // Remember the old one, we'll compare then against it
        QStringList oldpath = pSettings->playerBackgroundObjects;

        pSettings->playerBackgroundObjects.clear();
        pSettings->playerBackgroundObjects.push_back( ui->lePathVideos->text() );

        if ( pSettings->playerBackgroundType != Settings::BACKGROUND_TYPE_VIDEO
        || oldpath != pSettings->playerBackgroundObjects )
            background_changed = true;
    }

    pSettings->playerCDGbackgroundTransparent = ui->boxCDGtransparent->isChecked();
    pSettings->playerIgnoreBackgroundFromFormats = ui->rbBackgroundIgnoreCustom->isChecked();

    // Player tab
    pSettings->playerLyricsFont = ui->fontPlayerLyrics->font();
    pSettings->playerLyricsFontFitLines = ui->spinMaxLyricFontLines->value();
    pSettings->playerLyricsTextAfterColor = ui->btnPlayerLyricsFuture->color();
    pSettings->playerLyricsTextBeforeColor = ui->btnPlayerLyricsPast->color();
    pSettings->playerLyricsTextSpotColor = ui->btnPlayerLyricsSpotlight->color();
    pSettings->playerLyricBackgroundTintPercentage = qMax( 0, qMin( ui->lyricBackgroundTransparency->value(), 100 ));
    pSettings->notificationCenterColor = ui->btnPlayerLyricsInfo->color();
    pSettings->notificationTopColor = ui->btnPlayerLyricsNotification->color();
    pSettings->playerMusicLyricDelay = ui->spinLyricDelay->value();

    //
    // Misc tab
    pSettings->queueAddNewSingersNext = ui->rbQueueNewStart->isChecked();
    pSettings->queueSaveOnExit = ui->boxSaveQueueOnExit->isChecked();

    // Web server
    pSettings->httpEnabled = ui->boxWebEnable->isChecked();
    pSettings->httpEnableAddQueue = ui->boxWebAllowAddSong->isChecked();

    // LIRC
    pSettings->lircDevicePath = ui->leLIRCdevice->text();
    pSettings->lircEnabled = ui->boxLIRCenable->isChecked();
    pSettings->lircMappingFile = ui->leLIRCmapping->text();

    // Full screen
    pSettings->startInFullscreen = ui->boxForceFullScreen->isChecked();

    pSettings->playerUseBuiltinMidiSynth = ui->rbMidiBuiltin->isChecked();

    // Music
    QStringList origMusicPaths = pSettings->musicCollections;

    if ( ui->boxMusicEnable->isChecked() )
    {
        if ( ui->leMusicPath->text().isEmpty() )
        {
            // Warn the user and send the focus
            QMessageBox::critical( 0, tr("Music collection path is empty"), tr("You must specify the music collection path") );
            ui->tabWidget->setCurrentIndex( 3 );
            ui->leMusicPath->setFocus();
            return;
        }

        pSettings->musicCollections.clear();
        pSettings->musicCollections.push_back( ui->leMusicPath->text() );

        if ( ui->boxMusicCrossfade->isChecked() )
            pSettings->musicCollectionCrossfadeTime = ui->spinMusicCrossfadeDelay->value();
        else
            pSettings->musicCollectionCrossfadeTime = 0;

        pSettings->musicCollectionSortedOrder = ui->rbMusicSortOrder->isChecked();
    }
    else
        pSettings->musicCollections.clear();

    // Save 'em first
    pSettings->save();

    // Emit the signals
    if ( background_changed )
        emit pEventor->settingsChangedBackground();

    if ( origMusicPaths != pSettings->musicCollections )
        emit pEventor->settingsChangedMusic();

    emit pEventor->settingsChanged();

    QDialog::accept();
}
