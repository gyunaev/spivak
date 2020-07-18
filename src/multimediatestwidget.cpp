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

#include <QTimer>
#include <QBuffer>
#include <QByteArray>
#include <QTextCursor>

#include "util.h"
#include "logger.h"
#include "multimediatestwidget.h"
#include "pluginmanager.h"
#include "ui_multimediatestwidget.h"

#define PLAY_FROM_RESOURCES

MultimediaTestWidget::MultimediaTestWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MultimediaTestWidget)
{
    ui->setupUi(this);

    // Files to test play, and current testing
    m_testExtensions << "wav" << "m4a" << "mp3" << "ogg" << "wma" << "mid";

    m_player = pPluginManager->createMediaPlayer();

    if ( m_player )
    {
        connect( m_player->qObject(), SIGNAL(error(QString)), this, SLOT(error(QString)) );
        connect( m_player->qObject(), SIGNAL(loaded()), this, SLOT(loaded()) );

        connect( ui->textBrowser, &QTextBrowser::anchorClicked, this, &MultimediaTestWidget::startTest );

        append( tr("Please <a href=\"#\">click here to start the test</a>. The automatic test will be completely silent.") );
    }
    else
        append( tr("Audio plugin is not loaded, audio test is not available.") );
}

MultimediaTestWidget::~MultimediaTestWidget()
{
    delete ui;
    delete m_player;
}

void MultimediaTestWidget::error(QString error )
{
    Logger::debug( "MultimediaTest: error returned: %s", qPrintable( error ) );

    if ( m_suppressErrors )
        return;

    m_suppressErrors = true;

    // Prepare the error message
    QString errormsg = tr("This means you will not be able to play files of this format.");

    // Specific tests require different handling
    if ( m_testExtensions[m_currentTesting] == "mp3" )
    {
        errormsg = tr("Your system cannot play MP3 files. This means relevant decoders are not installed.");

    #ifdef  Q_OS_LINUX
        errormsg += tr(" Most likely your GStreamer installation is crippled. Please google \"yourdistributionname mp3 playback\" and follow the instructions.");
    #endif
    }
    else if ( m_testExtensions[m_currentTesting] == "mid" )
    {
        errormsg = tr("Your system cannot play MIDI natively, which is pretty common. However Spivak has built-in MIDI player and will play MIDI files.");
        m_playableFormats.append( "mid" );
    }

    append( tr("<font color=\"red\">Failed</font></p><p>%1</p>")
                         .arg( errormsg.toHtmlEscaped() ), false );

    if ( (int) m_currentTesting < m_testExtensions.size() - 1 )
    {
        m_currentTesting++;
        QTimer::singleShot( 100, this, SLOT(testNext()));
    }
    else
    {
        append( tr("<p>Test finished. Supported formats: <b>%1</b>") .arg( m_playableFormats.join( "," ) ) );
    }
}

void MultimediaTestWidget::loaded()
{
    append( tr("<font color=\"green\">succeed</font></p>"), false );

    m_playableFormats.push_back( m_testExtensions[m_currentTesting] );

    if ( (int) m_currentTesting < m_testExtensions.size() - 1 )
    {
        m_currentTesting++;
        QTimer::singleShot( 100, this, SLOT(testNext()));
    }
    else
    {
        append( tr("Test finished. Supported formats: %1") .arg( m_playableFormats.join( "," ) ) );
    }
}

void MultimediaTestWidget::startTest()
{
    ui->textBrowser->clear();

    m_currentTesting = 0;
    m_playableFormats.clear();

    testNext();
}

void MultimediaTestWidget::testNext()
{
    Logger::debug( "MultimediaTest: testing %s", qPrintable(m_testExtensions[m_currentTesting]) );

    append( tr("<b>Testing %1 playback</b> ... " ).arg( m_testExtensions[m_currentTesting] ) );
    m_suppressErrors = false;

    // Load the file from resources
    QFile f ( QString(":/sounds/test.%1") .arg(m_testExtensions[m_currentTesting]) );

    if ( !f.open( QIODevice::ReadOnly ) )
    {
        Logger::error( "MultimediaTest: failed to open test file %s", qPrintable(f.fileName()) );
        append( "failed to open the file, please report" );
        return;
    }

    QBuffer * buf = new QBuffer();
    buf->setData( f.readAll() );
    buf->open( QIODevice::ReadOnly );

    Q_ASSERT( buf->data().size() > 0 );

    m_player->loadMedia( buf, MediaPlayer::LoadAudioStream );
}


void MultimediaTestWidget::append(const QString &text, bool newparagraph)
{
    ui->textBrowser->moveCursor( QTextCursor::End );

    if ( newparagraph )
        ui->textBrowser->textCursor().insertBlock();

    ui->textBrowser->textCursor().insertHtml( text );
    ui->textBrowser->ensureCursorVisible();

}
