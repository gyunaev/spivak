#include <QTimer>
#include <QPushButton>

#include "logger.h"
#include "messageboxautoclose.h"
#include "settings.h"

MessageBoxAutoClose::MessageBoxAutoClose( QWidget * parent )
    : QMessageBox( parent )
{

}

void MessageBoxAutoClose::critical(const QString &title, const QString &message)
{
    // Create a message box
    MessageBoxAutoClose * m = new MessageBoxAutoClose( 0 );
    m->setIcon( QMessageBox::Critical );

    m->setText( message );
    m->setWindowTitle( title );
    m->setStandardButtons( QMessageBox::Close );

    // Delete the dialog object once it is closed
    connect( m->button( QMessageBox::Close ), SIGNAL( clicked(bool)), m, SLOT(deleteLater()) );
    connect( m, SIGNAL(accepted()), m, SLOT(deleteLater()) );
    connect( m, SIGNAL(rejected()), m, SLOT(deleteLater()) );

    // Update the remaining time for autoclose and show it
    m->m_remainingSec = pSettings->dialogAutoCloseTimer;
    m->updatetime();

    // Show the dialog and wait until it returns
    m->exec();
}

void MessageBoxAutoClose::updatetime()
{
    if ( m_remainingSec <= 0 )
    {
        reject();
        return;
    }

    // Update the remaining text
    QAbstractButton * btn = button( QMessageBox::Close );
    btn->setText( tr("Close (%1)") .arg( m_remainingSec ));
    m_remainingSec--;

    QTimer::singleShot( 1000, this, SLOT( updatetime()) );
}
