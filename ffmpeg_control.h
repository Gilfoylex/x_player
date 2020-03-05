#ifndef FFMPGE_CONTROL_H
#define FFMPGE_CONTROL_H
#include <QObject>
#include <QImage>

class Worker : public QObject
{
    Q_OBJECT
public:
    void SetFileName(const QString & str_file_path);
    QString GetFileName() const;

public slots:
    void doWork(void *p_param);

signals:
    void resultReady(const QImage &);

private:
    QString m_str_file_path;
};


#endif // FFMPGE_CONTROL_H
