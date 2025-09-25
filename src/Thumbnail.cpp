#include "Thumbnail.h"

#include <qtimer.h>
#include <QWindow>

Thumbnail::Thumbnail(QMainWindow *parent, const QString &CharacterName) : QWidget(parent)
{
    qInfo() << "CONSTRUCTOR [Thumbnail]";
    //ui.setupUi(this); // UI deviens Ui_MainWindow
    m_CharacterName = CharacterName;

    setWindowFlags(Qt::Window |
                   Qt::SplashScreen );

    setWindowTitle(QString("Thumbnail - %1").arg(CharacterName));

    setStyleSheet("background-color: rgba(50, 150, 250, 180);");
    resize(200, 150);

}

void Thumbnail::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        window()->windowHandle()->startSystemMove();
        event->accept();
    }
    if (event->buttons() & Qt::LeftButton) {
        // switch
    }
}

void Thumbnail::showEvent(QShowEvent *event) {
    qInfo() << "[showEvent]";

    QWidget::showEvent(event);
    QTimer::singleShot(100, this, [this] {
        KWinManager::MakeThumbnailsAlwaysOnTop(m_CharacterName);
    });
}
