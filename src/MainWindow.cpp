#include "MainWindow.h"

MainWindow::MainWindow()
{
    ui.setupUi(this);   // plus de "new", plus de "->"

    connect(
        ui.pushButton,
        &QPushButton::clicked,
        this,
        [] { qInfo() << "Button clicked"; ScreenCastHandler::instance()->init(); });

    QImage img(200, 200, QImage::Format_RGB32);
    img.fill(Qt::red);

    connect(
        ScreenCastHandler::instance(),
        &ScreenCastHandler::videoFrameAvailable,
        this,
        [this](const QImage& image) {
            ui.label->setPixmap(QPixmap::fromImage(image));
    });

    // connect(
    //     ui.pushButton_2,
    //     &QPushButton::clicked,
    //     this,
    //     [] { qInfo() << "Button_2 clicked"; ScreenCastHandler::instance()->test(); });
}
