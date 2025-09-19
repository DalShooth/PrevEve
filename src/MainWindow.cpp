#include "MainWindow.h"

MainWindow::MainWindow()
{
    ui.setupUi(this);   // plus de "new", plus de "->"

    m_screencast = new ScreenCastHandler(this);

    // Quand on clique sur pushButton → on lance CreateSession
    connect(ui.pushButton, &QPushButton::clicked, this, [this]() {
        qInfo() << "Button clicked";
        m_screencast->createSession();
    });

    // Pour afficher quand les sources sont choisies
    connect(m_screencast, &ScreenCastHandler::sourcesSelected,
            this, [](const QString& reqHandle) {
        qDebug() << "[MainWindow] Sources sélectionnées, request =" << reqHandle;
    });
}
