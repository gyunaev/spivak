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
#include <QMessageBox>

#include "karaokeplayable.h"
#include "queuekaraokewidget_addeditdialog.h"
#include "ui_queuekaraokewidget_addeditdialog.h"


QueueKaraokeWidget_AddEditDialog::QueueKaraokeWidget_AddEditDialog(const QStringList &singers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QueueKaraokeWidget_AddEditDialog)
{
    ui->setupUi(this);

    connect( ui->btnShowSearch, SIGNAL(clicked()), this, SLOT(showSearch()) );
    connect( ui->btnSearch, SIGNAL(clicked()), this, SLOT(buttonSearch()) );
    connect( ui->btnDiskFile, SIGNAL(clicked(bool)), this, SLOT(buttonAddFile()) );
    connect( ui->tableSearch, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(searchEntryDoubleClicked(QModelIndex)) );

    // Fill up the existing singers
    if ( !singers.isEmpty() )
    {
        ui->boxSinger->addItems( singers );
        ui->boxSinger->setCurrentText("");
    }

    // Hide the search field when we start
    ui->groupSearch->hide();
    adjustSize();

    // Search model
    m_modelSearch = new TableModelSearch( this );
    ui->tableSearch->setModel( m_modelSearch );

    setWindowTitle( "Add a new singer and song");
}

QueueKaraokeWidget_AddEditDialog::~QueueKaraokeWidget_AddEditDialog()
{
    delete ui;
}

void QueueKaraokeWidget_AddEditDialog::setCurrentParams(const SongQueueItem &song)
{
    m_params = song;

    ui->boxSinger->setCurrentText( m_params.singer );
    ui->leKaraoke->setText( tr("\"%1\" by %2") .arg(m_params.title) .arg(m_params.artist) );

    setWindowTitle( "Edit a singer and/or song");
}

SongQueueItem QueueKaraokeWidget_AddEditDialog::params() const
{
    return m_params;
}

void QueueKaraokeWidget_AddEditDialog::showSearch()
{
    ui->groupSearch->show();
    ui->leSearch->setFocus();
}

void QueueKaraokeWidget_AddEditDialog::buttonSearch()
{
    QString text = ui->leSearch->text();

    if ( text.isEmpty() )
        return;

    m_modelSearch->performSearch( text );
}

void QueueKaraokeWidget_AddEditDialog::buttonAddFile()
{
    QString path = QFileDialog::getOpenFileName( 0,
                                                 tr("Choose a karaoke file") );

    if ( path.isEmpty() )
        return;

    // If this is a video file, there is no lyrics
    if ( !KaraokePlayable::isVideoFile( path ) )
    {
        QScopedPointer<KaraokePlayable> karaoke( KaraokePlayable::create( path ) );

        if ( !karaoke || !karaoke->parse() )
        {
            QMessageBox::critical( 0,
                                   tr("Not a valid Karaoke file"),
                                   tr("Cannot use this file - either not a Karaoke file, or music/lyrics not found") );
            return;
        }
    }

    m_params.songid = 0;
    m_params.artist = tr("Own file");
    m_params.title = path;
    m_params.file = path;

    ui->leKaraoke->setText( path );
}

void QueueKaraokeWidget_AddEditDialog::searchEntryDoubleClicked(QModelIndex item)
{
    if ( !item.isValid() )
        return;

    const Database_SongInfo& info = m_modelSearch->infoAt( item );

    m_params.songid = info.id;
    m_params.artist = info.artist;
    m_params.title = info.title;
    m_params.file = info.filePath;

    // Use tooltip as text
    ui->leKaraoke->setText( m_modelSearch->data( item, Qt::ToolTipRole ).toString() );
}

void QueueKaraokeWidget_AddEditDialog::accept()
{
    if ( ui->boxSinger->currentText().isEmpty() )
    {
        QMessageBox::critical( 0,
                               tr("Missing singer name"),
                               tr("Please enter the singer name") );
        return;
    }

    if ( ui->leKaraoke->text().isEmpty() )
    {
        QMessageBox::critical( 0,
                               tr("Missing karaoke"),
                               tr("Please choose the Karaoke file or song") );
        return;
    }

    m_params.singer = ui->boxSinger->currentText();
    QDialog::accept();
}
