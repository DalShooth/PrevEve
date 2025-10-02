#pragma once

#include <QMap>
#include <QObject>
#include <qpoint.h>
#include <QSizeF>
#include "Data/ThumbnailPosition.h"

class ConfigManager final : public QObject
{
    Q_OBJECT
public:
    static QSize loadThumbnailsSize();
    static QStringList loadCharacters();
    static QPoint loadThumbnailPosition(const QString &character);

    static auto saveThumnailsSize(int width, int height) -> void;
    static void saveThumbnailPosition(QPoint thumbnailPosition, const QString &character);
    static void saveCharacters(const QStringList &saveCharacters);
};
