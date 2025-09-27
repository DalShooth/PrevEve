#include "MainWindow.h"

#include <qdbusreply.h>
#include "KWinManager.h"

MainWindow::MainWindow()
{
    qInfo() << "CONSTRUCTOR [MainWindow]";

    //= UI
    m_Ui = new Ui_MainWindow();
    m_Ui->setupUi(this); // UI deviens Ui_MainWindow
    setFixedSize(450, 200);
    //=

    // Connecte le bouton test
    connect(
        m_Ui->SetupPreviewsButton,
        &QPushButton::clicked,
        this,
        [] { qInfo() << "SetupPreviewsButton clicked"; StreamManager::Instance().SetupPreviews(); });
}
