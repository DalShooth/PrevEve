#pragma once

#include "ui_MainWindow.h"

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow();
    ~MainWindow() override {
        
    }

    Ui_MainWindow GetUi() const { return *m_Ui; }

    signals:
    void onThumbnailsSizeSettingsChanged(int Width, int Height);

private:
    void onSavePositionButtonClicked();

    Ui_MainWindow* m_Ui;
};
