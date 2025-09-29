#include "ConfigManager.h"

#include <qdir.h>
#include <qpoint.h>
#include <QSettings>

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
    qInfo() << "[loadThumbnailsSize]";

    const QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat);

    const int width  = settings.value("ThumbnailWidth", 320).toInt();
    const int height = settings.value("ThumbnailHeight", 180).toInt();

    return QSize(width, height);
}

QPoint ConfigManager::loadThumbnailPosition(const QString &caption) const {
    qInfo() << "[loadThumbnailPosition]";

    QSettings settings(
        QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat
    );

    settings.beginGroup("ThumbnailsPositions");
    const QString value = settings.value(caption).toString(); // valeur de la clé
    settings.endGroup();

    if (value.isEmpty()) // valeur par défaut si pas trouvé
        return QPoint(-1, -1);

    const QStringList parts = value.split(",");
    if (parts.size() != 2)
        return QPoint(-1, -1);

    bool ok1 = false, ok2 = false;
    int x = parts[0].toInt(&ok1);
    int y = parts[1].toInt(&ok2);

    if (ok1 && ok2)
        return QPoint(x, y);

    return QPoint(-1, -1); // fallback si parsing échoue
}

