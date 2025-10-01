#include "ConfigManager.h"

#include <qdir.h>
#include <qpoint.h>
#include <QSettings>

#include "KWinManager.h"

ConfigManager::ConfigManager()
{
    qInfo() << "CONSTRUCTOR [ConfigManager]";
}

void ConfigManager::saveThumnailsSize(const int width, const int height) const {
    qInfo() << "[saveThumnailsSize]";

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf", QSettings::IniFormat);
    settings.setValue("ThumbnailWidth", width);
    settings.setValue("ThumbnailHeight", height);

    settings.sync();
}

QSize ConfigManager::loadThumbnailsSize() const {
    qInfo() << "loadThumbnailsSize()";

    const QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat);

    const int width  = settings.value("ThumbnailWidth", 320).toInt();
    const int height = settings.value("ThumbnailHeight", 180).toInt();

    qInfo() << "Thumbnails Size loaded -> w:" << width << "h:" << height;

    return QSize(width, height);
}

QPoint ConfigManager::loadThumbnailPosition(const QString &caption) const {
    qInfo() << "loadThumbnailPosition()";

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
                       QSettings::IniFormat);

    settings.beginGroup("ThumbnailsPositions");
    QPoint pos = settings.value(caption, QPoint(0, 0)).toPoint();
    settings.endGroup();

    qInfo() << "Thumbnails Position loaded -> " << pos;

    return pos;
}


void ConfigManager::saveThumbnailsPositions(const QString &caption, const int x, const int y) const {
    qInfo() << "[saveThumbnailsPositions]";

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
                       QSettings::IniFormat);

    settings.beginGroup("ThumbnailsPositions");

    // Sauvegarde QPoint directement
    settings.setValue(caption, QPoint(x, y));

    settings.endGroup();
    settings.sync();
}

