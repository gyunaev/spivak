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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QTimer>
#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class Interface_LanguageDetector;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    private slots:
        void    backgroundChanged(int idx);
        void    collectionLanguageCheckboxChanged();
        void    collectionSeparatorChanged();
        void    collectionTypeChanged();

        void    browseCollection();
        void    collectionTestArtistTitle( QString );
        void    browseBackgroundImages();
        void    browseBackgroundVideos();
        void    browseLIRCmapping();
        void    browseMusicPath();

        void    startUpdateDatabase();
        void    eraseDatabase();

        // Scan collection slots from Eventor
        void    scanCollectionProgress(QString progress);
        void    refreshDatabaseInformation();

        void    lircChanged();
        void    updateLyricsPreview();

        void    accept();

    private:
        // Make sure the language detector could be loaded
        void    updateCollectionTestOutput();
        bool    validateAndStoreCollection();
        QString collectionSeparatorString( const QString& value );

        Ui::SettingsDialog *ui;

        // Used for collection testing
        QString m_testFileName;

        // Used for lyric rendering animation
        double  m_lyricAnimationValue;
        QTimer  m_lyricAnimationTimer;

        // Language detector plugin
        Interface_LanguageDetector * m_langdetector;
};

#endif // SETTINGSDIALOG_H
