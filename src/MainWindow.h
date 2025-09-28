#pragma once

#include "ui_MainWindow.h"
#include "StreamManager.h"

class QIntValidator;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow();

    Ui_MainWindow GetUi() const { return *m_Ui; }

    signals:
    void onThumbnailsSizeSettingsChanged(int Width, int Height);

private:
    Ui_MainWindow* m_Ui;   // objet direct
    QIntValidator* m_SizeValidator;
};
