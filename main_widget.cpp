#include "widget.h"
#include <QTextEdit>
#include <QPainter>
#include "ffmpeg_control.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    resize(600, 400);
    Worker *worker = new Worker;
    worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Widget::startVideo, worker, &Worker::doWork);
    connect(worker, &Worker::resultReady, this, &Widget::slotGetOneFrame);
    m_thread.start();
    worker->SetFileName(QString("D:/test.mp4"));
    emit startVideo(nullptr);
}

Widget::~Widget()
{
    m_thread.quit();
    m_thread.wait();
}

void Widget::paintEvent(QPaintEvent *p_event)
{
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, width(), height());
    if (m_image.size().width() <= 0) {
        return;
    }

    QImage img = m_image.scaled(this->size(),Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;

    painter.drawImage(QPoint(x,y),img);
}

void Widget::slotGetOneFrame(QImage image)
{
    m_image = image;
    update();
}
