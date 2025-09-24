#pragma once

#include <QMouseEvent>
#include "MainWindow.h"

class PipeWirePreview final : public QWidget
{
    Q_OBJECT

public:
    explicit PipeWirePreview(QMainWindow *parent = nullptr); // Constructor

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    Ui_MainWindow ui;

    QPoint m_dragOffset;
};
