#pragma once
#include <QDBusInterface>

class KWinManager
{
public:
    static void MakeThumbnailsAlwaysOnTop(const QString &CharacterName);
    static void SetWindowPosition(const QString &Caption, int X, int Y);
};
