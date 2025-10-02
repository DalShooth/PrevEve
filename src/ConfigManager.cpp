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
    qInfo() << "loadThumbnailsSize()";

    const QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat);

    const int width  = settings.value("ThumbnailWidth", 320).toInt();
    const int height = settings.value("ThumbnailHeight", 180).toInt();

    qInfo() << "Thumbnails Size loaded -> w:" << width << "h:" << height;

    return QSize(width, height);
}

QStringList ConfigManager::loadThumbnailsProfiles() {
    qInfo() << "loadThumbnailsProfilesList()";

    QSettings settings(
        QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat
    );

    // Récupère la liste, sinon retourne une QStringList vide
    QStringList thumbnailsProfiles = settings.value("ThumbnailsProfiles/List").toStringList();

    qInfo() << "[loadThumbnailsProfilesList] -> loaded" << thumbnailsProfiles.size() << "ThumbnailsProfiles";
    return thumbnailsProfiles;
}


QPoint ConfigManager::loadThumbnailPosition(const QString &profile) const {
    qInfo() << "loadThumbnailPosition()" << profile;

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
                       QSettings::IniFormat);

    settings.beginGroup("ThumbnailsPositions");
    int x = settings.value(profile + "/x", 0).toInt();
    int y = settings.value(profile + "/y", 0).toInt();
    settings.endGroup();

    return QPoint(x, y);
}



void ConfigManager::saveThumbnailsPositions(const QList<ThumbnailPosition>& thumbnailsPositions) const {
    qInfo() << "saveThumbnailsPositions()";

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf", QSettings::IniFormat);

    // On écrit les nouvelles
    settings.beginGroup("ThumbnailsPositions");
    for (const ThumbnailPosition& pos : thumbnailsPositions) {
        QString key = pos.profile;
        settings.setValue(key + "/x", pos.position.x());
        settings.setValue(key + "/y", pos.position.y());
    }
    settings.endGroup();

    settings.sync();
}

void ConfigManager::saveThumbnailsProfiles(const QStringList& ThumbnailsProfiles) const {
    qInfo() << "[saveThumbnailsProfilesList]";

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf", QSettings::IniFormat);

    // Écrit la liste sous forme de QStringList (QSettings sait les stocker)
    settings.setValue("ThumbnailsProfiles/List", ThumbnailsProfiles);

    settings.sync();
}


