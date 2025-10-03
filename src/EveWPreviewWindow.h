#pragma once

#include "StreamManager.h"
#include "ui_EveWPreviewWindow.h"
#include <QMainWindow>
#include <QObject>

class EveWPreviewWindow final : public QMainWindow
{
    Q_OBJECT
public:
    EveWPreviewWindow();
    ~EveWPreviewWindow() override {
        qInfo() << "DE-CONSTRUCTOR [EveWPreviewWindow]";
    };

    Ui_EveWPreviewWindow* m_Ui;
    StreamManager* m_StreamManager;

    signals:
    void onThumbnailsSizeSettingsChanged(int Width, int Height);

private:
    void onSavePositionButtonClicked();
};
