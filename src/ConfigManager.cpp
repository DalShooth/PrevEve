#include "ConfigManager.h"

#include <qdir.h>
#include <qpoint.h>
#include <QSettings>

void ConfigManager::saveThumnailsSize(const int width, const int height) {
    qInfo() << "ConfigManager::saveThumnailsSize()";

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf", QSettings::IniFormat);
    settings.setValue("ThumbnailWidth", width);
    settings.setValue("ThumbnailHeight", height);

    settings.sync();
}

QSize ConfigManager::loadThumbnailsSize() {
    qInfo() << "ConfigManager::loadThumbnailsSize()";

    const QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat);

    const int width  = settings.value("ThumbnailWidth", 320).toInt();
    const int height = settings.value("ThumbnailHeight", 180).toInt();

    return QSize(width, height);
}

QStringList ConfigManager::loadCharacters() {
    qInfo() << "ConfigManager::loadCharacters()";

    QSettings settings(
        QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat
    );

    // Récupère la liste, sinon retourne une QStringList vide
    QStringList characters = settings.value("Characters/List").toStringList();

    return characters;
}


QPoint ConfigManager::loadThumbnailPosition(const QString &character) {
    qInfo() << "ConfigManager::loadThumbnailPosition()" << character;

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
                       QSettings::IniFormat);

    settings.beginGroup("ThumbnailsPositions");
    int x = settings.value(character + "/x", 0).toInt();
    int y = settings.value(character + "/y", 0).toInt();
    settings.endGroup();

    return QPoint(x, y);
}

void ConfigManager::saveThumbnailPosition(const QPoint thumbnailPosition, const QString &character)
{
    QSettings settings(
        QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf",
        QSettings::IniFormat
    );

    settings.beginGroup("ThumbnailsPositions");
    settings.setValue(character + "\\x", thumbnailPosition.x());
    settings.setValue(character + "\\y", thumbnailPosition.y());
    settings.endGroup();

    settings.sync();
}

void ConfigManager::saveCharacters(const QStringList& saveCharacters) {
    qInfo() << "ConfigManager::saveCharacters()";

    QSettings settings(QDir::homePath() + "/.config/eve-w-preview/eve-w-preview.conf", QSettings::IniFormat);

    // Écrit la liste sous forme de QStringList (QSettings sait les stocker)
    settings.setValue("Characters/List", saveCharacters);

    settings.sync();
}


