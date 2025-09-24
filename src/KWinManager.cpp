#include "KWinManager.h"


void KWinManager::MakeThumbnailAlwaysOnTop(const QString &CharacterName)
{
    QString script = QString(R"(
        const wins = workspace.stackingOrder;
        const target = "Thumbnail - %1";
        for (var i = 0; i < wins.length; ++i) {
            var w = wins[i];
            if (w.caption === target) {
                w.keepAbove = true;
                print("Fenêtre trouvée: " + target + " → mise en avant");
            }
        }
    )").arg(CharacterName);


    qInfo() << script;
}
