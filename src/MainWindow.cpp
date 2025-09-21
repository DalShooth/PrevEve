#include "MainWindow.h"

MainWindow::MainWindow()
{
    ui.setupUi(this);   // plus de "new", plus de "->"

    connect(
        ui.pushButton,
        &QPushButton::clicked,
        this,
        [] { qInfo() << "Button clicked"; ScreenCastHandler::instance()->init(); });
}
