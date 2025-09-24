#include "PipeWirePreview.h"
#include <QWindow>
#include <KWindowSystem>
#include <qtimer.h>

PipeWirePreview::PipeWirePreview(QMainWindow *parent) : QWidget(parent)
{
    //ui.setupUi(this); // UI deviens Ui_MainWindow

    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint);

    setStyleSheet("background-color: rgba(50, 150, 250, 180);");
    resize(200, 150);

    // QTimer::singleShot(0, this, [this]() {
    //     WId wid = this->winId();
    //     KWindowSystem::setState(wid, NET::KeepAbove);
    // });
}

void PipeWirePreview::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        window()->windowHandle()->startSystemMove();
        event->accept();
    }
    if (event->buttons() & Qt::LeftButton) {
        // switch
    }
}
