#include "MainWindow.h"

#include <qdbusreply.h>
#include "KWinManager.h"

MainWindow::MainWindow()
{
    qInfo() << "CONSTRUCTOR [MainWindow]";

    m_Ui = new Ui_MainWindow();
    m_Ui->setupUi(this); // UI deviens Ui_MainWindow

    // Connecte le bouton test
    connect(
        m_Ui->SetupPreviewsButton,
        &QPushButton::clicked,
        this,
        [] { qInfo() << "Button clicked"; StreamManager::Instance().init(); });

    connect(
        m_Ui->SetupPreviewsButton,
        &QPushButton::clicked,
        this,
        [] { qInfo() << "Button_2 clicked"; KWinManager::MakeThumbnailsAlwaysOnTop("1"); });
}
