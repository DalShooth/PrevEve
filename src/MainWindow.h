#pragma once

#include "ui_MainWindow.h"
#include "ScreenCastHandler.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow();

private:
    Ui_MainWindow ui;   // objet direct
    ScreenCastHandler* m_screencast;
};
