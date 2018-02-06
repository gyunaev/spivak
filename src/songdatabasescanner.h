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

#ifndef SONGDATABASESCANNER_H
#define SONGDATABASESCANNER_H

#include <QMap>
#include <QQueue>
#include <QTimer>
#include <QObject>
#include <QAtomicInt>
#include <QWaitCondition>
#include <QMutex>
#include <QDateTime>

#include "collectionentry.h"

class SongDatabaseScannerWorkerThread;
class Interface_LanguageDetector;

class SongDatabaseScanner : public QObject
{
    Q_OBJECT

    public:
        explicit SongDatabaseScanner( QObject *parent = 0 );
        ~SongDatabaseScanner();

        class SongDatabaseEntry
        {
            public:
                int         colidx;     // collection index in m_collection array internally; changes to collection ID when calling updateDatabase
                QString     artist;
                QString     title;
                QString     filePath;   // main file - the compound file or lyrics
                QString     musicPath;  // if music file is separate, will be used to get artist/title if not available otherwise
                QString     type;       // karaoke format such as cdg, avi, or midi; could also be extended such as lrc/minus
                QString     language;   // the language value if unknown/impossible to detect
                int         flags;
        };

        // Find out the artist and title from lyrics, music or file path.
        static bool    guessArtistandTitle(const QString &filepath , const QString &separator, QString &artist, QString &title);

    public slots:
        bool    startScan();
        void    stopScan();

    private slots:
        void    updateScanProgress();

        // Those slots is used when downloading index using the provider
        void    providerFinished( int id, QString errmsg );
        void    providerProgress( int id, int percentage );

    private:
        friend class SongDatabaseScannerWorkerThread;

        // This thread scans directories and finds out karaoke files. It only performs directory enumeration,
        // and does not read any files nor accesses the database.
        void    scanCollectionsThread();

        // This thread scans the database and cleans up the entries if the files were removed from disk
        void    cleanupThread();

        // This thread (probably a few) reads the lyric files (if they're text files) and gets the artist/title
        // and language information if available. This is not possible for all formats (not for CD+G for example).
        // Those threads access database for reading only.
        void    processingThread();

        // This thread submits the new entries into the database.
        void    submittingThread();

        // Parses the collection index file to skip enumerator and processor
        void    parseCollectionIndex(const CollectionEntry &col, const QByteArray& indexdata );

        // Producer-consumer implementation of processing queue
        QMutex                      m_processingQueueMutex;
        QWaitCondition              m_processingQueueCond;
        QQueue<SongDatabaseEntry>   m_processingQueue;

        // Adding an entry into the processing queue
        void    addProcessing( const SongDatabaseEntry& entry );

        // Producer-consumer implementation of database submitting queue
        QMutex                      m_submittingQueueMutex;
        QWaitCondition              m_submittingQueueCond;
        QList<SongDatabaseEntry>    m_submittingQueue;

        // Adding an entry into the submitting queue
        void    addSubmitting( const SongDatabaseEntry& entry );

        // Scan runtime statistics
        QAtomicInt                  m_stat_directoriesScanned;
        QAtomicInt                  m_stat_karaokeFilesFound;
        QAtomicInt                  m_stat_karaokeFilesProcessed;
        QAtomicInt                  m_stat_karaokeFilesSubmitted;

        // A flag to abort scanning; is also used to finish scans (so true doesn't indicate stopScan)
        QAtomicInt                  m_finishScanning;

        // A flag which is set by StopScan together with finishScanning and indicates it was an abort
        QAtomicInt                  m_abortScanning;

        // Copy of collection for scanning
        QMap<int,CollectionEntry>   m_collection;

        // An optional plugin (auto-loaded) to detect the lyric language
        Interface_LanguageDetector    *       m_langDetector;

        // Thread pool of scanners
        QList< QThread * >          m_threadPool;

        // Number of threads completing the task (to send finished)
        QAtomicInt                  m_threadsRunning;

        // Scan information update timer
        QTimer                      m_updateTimer;

        // Used in tracking the collection provider; 0 means done/succeed, 1 done/error
        int                         m_providerStatus;

        // If non-empty contains the progress (from a provider)
        QString                     m_stringProgress;
};

#endif // SONGDATABASESCANNER_H
