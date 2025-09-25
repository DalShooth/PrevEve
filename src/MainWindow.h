#pragma once

#include "ui_MainWindow.h"
#include "StreamManager.h"

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow();

private:
    Ui_MainWindow* m_Ui;   // objet direct
};
