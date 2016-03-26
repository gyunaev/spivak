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
    connect( ui->btnTestArtistTitle, SIGNAL(clicked()), this, SLOT(browseCollectionTestFileName()) );
    connect( ui->btnBrowseImages, SIGNAL(clicked()), this, SLOT(browseBackgroundImages()) );
    connect( ui->btnBrowseVideos, SIGNAL(clicked()), this, SLOT(browseBackgroundVideos()) );
    connect( ui->btnBrowseLIRCmapping, SIGNAL(clicked()), this, SLOT(browseLIRCmapping()) );
    connect( ui->boxLIRCenable, SIGNAL(toggled(bool)), this, SLOT(lircChanged()) );
    connect( ui->btnBrowseMusic, SIGNAL(clicked()), this, SLOT(browseMusicPath()) );

    connect( ui->btnUpdateCollection, &QPushButton::clicked, this, &SettingsDialog::startUpdateDatabase );
    connect( ui->btnEraseCollection, &QPushButton::clicked, this, &SettingsDialog::eraseDatabase );

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
        ui->boxCollectionDetectLang->setEnabled( false );
        ui->boxCollectionDetectLang->setChecked( false );
    }


    //
    // Init the parameters
    //

    //
    // Collection tab
    QList<Database_CollectionInfo> cols = pDatabase->getCollections();

    // Common separator values are hardcoded
    for ( int i = 0; coltemplatelist[i]; i++ )
        ui->boxCollectionSeparator->addItem( collectionSeparatorString( coltemplatelist[i] ), coltemplatelist[i] );

    ui->boxCollectionSeparator->addItem( tr("Add a new separator..." ) );

    // Collections tab is hidden where multiple collections are defined - edited via Collection Editor
    if ( cols.size() > 1 )
        ui->tabCollections->hide();
    else if ( cols.size() == 1 )
    {
        const Database_CollectionInfo& col = cols.first();

        ui->leCollectionPath->setText( col.rootPath );
        ui->boxColelctionZIP->setChecked( col.scanZips );
        ui->boxCollectionDetectLang->setChecked( col.detectLanguage );

        if ( !col.defaultLanguage.isEmpty() && m_langdetector )
            ui->boxCollectionLanguage->setCurrentText( col.defaultLanguage );

        // Is current separator there?
        if ( ui->boxCollectionSeparator->findData( col.artistTitleSeparator ) == -1 )
            ui->boxCollectionSeparator->insertItem( ui->boxCollectionSeparator->count() - 1, collectionSeparatorString(col.artistTitleSeparator), col.artistTitleSeparator );

        // And show it as current
        ui->boxCollectionSeparator->setCurrentIndex( ui->boxCollectionSeparator->findData( col.artistTitleSeparator  ) );
    }

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

void SettingsDialog::scanCollectionProgress(unsigned long directoriesScanned, unsigned long karaokeFilesFound, unsigned long filesProcessed, unsigned long filesSubmitted)
{
    ui->lblCollectionInfo->setText( tr("Scan progress: %1 directories scanned, %2 karaoke files found, %3 processed, %4 submitted")
                                                                .arg( directoriesScanned )
                                                                .arg( karaokeFilesFound )
                                                                .arg( filesProcessed)
                                                                .arg( filesSubmitted) );
}

void SettingsDialog::refreshDatabaseInformation()
{
    qint64 count = pDatabase->getSongCount();

    if ( count == 0 )
    {
        ui->lblCollectionInfo->setText( tr("Song collection is empty") );
    }
    else
    {
        ui->lblCollectionInfo->setText( tr("Song collection: %1 songs, last updated %2")
                            .arg( count )
                            .arg( pDatabase->lastDatabaseUpdate() > 0 ?
                                      QDateTime::fromMSecsSinceEpoch( pDatabase->lastDatabaseUpdate() * 1000).toString( "yyyy-MM-dd hh:mm:ss")
                                        : tr("never") ) );
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
    QList<Database_CollectionInfo> collections;

    if ( ui->leCollectionPath->text().isEmpty() )
    {
        pDatabase->setCollections( collections );
        return true;    // valid but empty
    }

    // Path must be valid
    Database_CollectionInfo col;
    col.rootPath = ui->leCollectionPath->text();

    if ( !QFileInfo(col.rootPath).exists() )
    {
        QMessageBox::critical( 0, "The collection path does not exist", "Cannot use this collection - the path does not exist" );

        // Switch to our tab and focus on value
        ui->tabWidget->setCurrentIndex( 0 );
        ui->leCollectionPath->setFocus();
        return false;
    }

    col.scanZips = ui->boxColelctionZIP->isChecked();
    col.detectLanguage = ui->boxCollectionDetectLang->isChecked();

    if ( ui->boxCollectionLanguage->currentIndex() > 0 && m_langdetector )
        col.defaultLanguage = ui->boxCollectionLanguage->currentText();
    else
        col.defaultLanguage.clear();

    col.artistTitleSeparator = ui->boxCollectionSeparator->currentData().toString();

    collections.append( col );
    pDatabase->setCollections( collections );

    return true;
}


void SettingsDialog::browseCollectionTestFileName()
{
    QString e = QFileDialog::getOpenFileName( 0, "Choose the file from your collection" );

    if ( e.isEmpty() )
        return;

    m_testFileName = e;
    updateCollectionTestOutput();
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

void SettingsDialog::updateCollectionTestOutput()
{
    if ( m_testFileName.isEmpty() )
        return;

    QString artist, title;

    if ( !SongDatabaseScanner::guessArtistandTitle( m_testFileName, ui->boxCollectionSeparator->currentData().toString(), artist, title ) )
        ui->lblTestArtistTitle->setText( tr("File <i>%1</i>\nCannot detect artist/title in this file using the separator above")
                                         .arg(m_testFileName.toHtmlEscaped()) );
    else
        ui->lblTestArtistTitle->setText( tr("File <i>%1</i><br>Detected artist: <b>%2</b>, title: <b>%3</b>")
                                         .arg(m_testFileName.toHtmlEscaped()) .arg(artist.toHtmlEscaped()) .arg(title.toHtmlEscaped()) );
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
