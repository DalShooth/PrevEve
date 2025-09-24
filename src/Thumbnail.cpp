#include "Thumbnail.h"
#include <QWindow>

Thumbnail::Thumbnail(QMainWindow *parent, const int thumbnailId) : QWidget(parent)
{
    //ui.setupUi(this); // UI deviens Ui_MainWindow

    setWindowFlags(Qt::Window |
                   Qt::SplashScreen );


    setWindowTitle(QString("Thumbnail - %1").arg(thumbnailId));


    setStyleSheet("background-color: rgba(50, 150, 250, 180);");
    resize(200, 150);
}

void Thumbnail::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        window()->windowHandle()->startSystemMove();
        event->accept();
    }
    if (event->buttons() & Qt::LeftButton) {
        // switch
    }
}
