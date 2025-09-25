#pragma once

#include "KWinManager.h"
#include "ui_MainWindow.h"
#include "StreamManager.h"

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow();

private:
    Ui_MainWindow* ui;   // objet direct
};
