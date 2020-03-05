#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QThread>

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

protected:
    void paintEvent(QPaintEvent *p_event) override;

private slots:
    void slotGetOneFrame(QImage image);

signals:
    void startVideo(void * p_parm);

private:
    QImage m_image;
    QThread m_thread;
};

#endif // WIDGET_H
