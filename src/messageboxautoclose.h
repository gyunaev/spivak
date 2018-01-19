#ifndef MESSAGEBOXAUTOCLOSE_H
#define MESSAGEBOXAUTOCLOSE_H

#include <QObject>
#include <QMessageBox>

class MessageBoxAutoClose : public QMessageBox
{
    Q_OBJECT

    public:
        MessageBoxAutoClose( QWidget * parent = 0 );

        static void critical( const QString& title,
                              const QString& message );

    public slots:
        void    updatetime();

    private:
        unsigned int    m_remainingSec;
};

#endif // MESSAGEBOXAUTOCLOSE_H
