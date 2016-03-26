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

#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>

#include "util.h"
#include "settings.h"
#include "multimediatestwidget.h"
#include "welcome_wizard.h"
#include "ui_welcome_wizard.h"

WelcomeWizard::WelcomeWizard(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::WelcomeWizard)
{
    ui->setupUi(this);

    //setStartId(Page_Intro);
    setPixmap( QWizard::WatermarkPixmap, QPixmap(":/images/welcomewizard.jpg") );

    // Set up the test wizard page
    ui->wizardTestSound->layout()->addWidget( new MultimediaTestWidget(this) );

#ifndef Q_OS_MAC
    setWizardStyle( QWizard::ModernStyle );
#endif

    // Platform example directory
    QString platformDir = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + Util::separator() + "Karaoke";

    // Collection help string
    QString collectiontext = ui->lblCollection->text().replace( "{PLATFORMDIR}", platformDir.toHtmlEscaped() );


    // Preferences located differently on different OS
#ifndef Q_OS_MAC
    collectiontext.replace( "{PREFERENCES}", tr("the Preferences submenu located in File menu"));
#else
    collectiontext.replace( "{PREFERENCES}", tr("the Preferences submenu located in the action bar"));
#endif

    ui->lblCollection->setText( collectiontext );
}

WelcomeWizard::~WelcomeWizard()
{
    delete ui;
}

void WelcomeWizard::accept()
{
    pSettings->firstTimeWizardShown = true;
    pSettings->save();

    QWizard::accept();
}

void WelcomeWizard::reject()
{
    if ( !pSettings->firstTimeWizardShown )
    {
        if ( QMessageBox::question( 0,
                                  tr("Tutorial cancelled"),
                                  tr("Do you want to show this tutorial next time when the player starts?\nNote that you can always access it through Help menu")) == QMessageBox::No )
        {
            pSettings->firstTimeWizardShown = true;
            pSettings->save();
        }
    }

    QWizard::reject();
}
