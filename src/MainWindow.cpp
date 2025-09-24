#include "MainWindow.h"

MainWindow::MainWindow()
{
    ui.setupUi(this); // UI deviens Ui_MainWindow

    // Connecte le bouton test
    connect(
        ui.pushButton,
        &QPushButton::clicked,
        this,
        [] { qInfo() << "Button clicked"; StreamManager::Instance().init(); });








    connect(
        &StreamManager::Instance(),
        &StreamManager::videoFrameAvailable,
        this,
        [this](const QImage& image) {
            ui.label->setPixmap(QPixmap::fromImage(image));
    });
}
