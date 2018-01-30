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

#ifndef QUEUEKARAOKEWIDGET_ADDEDITDIALOG_H
#define QUEUEKARAOKEWIDGET_ADDEDITDIALOG_H

#include <QDialog>

#include "songqueue.h"
#include "queuetableviewmodels.h"


namespace Ui {
class QueueKaraokeWidget_AddEditDialog;
}


class QueueKaraokeWidget_AddEditDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit QueueKaraokeWidget_AddEditDialog( const QStringList& singers, QWidget *parent = 0 );
        ~QueueKaraokeWidget_AddEditDialog();

        void    setCurrentParams( const SongQueueItem& song );
        SongQueueItem params() const;


    private slots:
        void    showSearch();
        void    buttonSearch();
        void    buttonAddFile();
        void    searchEntryDoubleClicked(QModelIndex item);
        void    accept();

    private:
        Ui::QueueKaraokeWidget_AddEditDialog * ui;

        SongQueueItem m_params;

        TableModelSearch *  m_modelSearch;
};

#endif // QUEUEKARAOKEWIDGET_ADDEDITDIALOG_H
