#include "MainWindow.h"

#include <qdbusreply.h>
#include "KWinManager.h"

MainWindow::MainWindow()
{
    qInfo() << "CONSTRUCTOR [MainWindow]";

    //= UI
    m_Ui = new Ui_MainWindow();
    m_Ui->setupUi(this); // UI deviens Ui_MainWindow
    setFixedSize(400, 200);
    //=

    // Connecte le bouton test
    connect(
        m_Ui->SetupPreviewsButton,
        &QPushButton::clicked,
        this,
        [] {
            qInfo() << "SetupPreviewsButton clicked";
            if (StreamManager::Instance().getScreenCastState() == ScreenCastState::Idle) {
                StreamManager::Instance().SetupPreviews();
            } else if (StreamManager::Instance().getScreenCastState() == ScreenCastState::Active) {
                StreamManager::Instance().ClosePreviews();
            }
    });
}
