function applyKeepAbove() {
    const wins = workspace.stackingOrder;
    var allOk = true;

    for (var i = 0; i < wins.length; ++i) {
        var w = wins[i];
        if (!w || !w.caption) continue;

        if (w.caption.indexOf("Thumbnail") === 0) {
            if (!w.keepAbove) {
                w.keepAbove = true;
                w.skipTaskbar = true;
                w.skipSwitcher = true;
                w.specialWindow = true;
                w.windowType = dock;
                allOk = false; // au moins une n’était pas encore en keepAbove
            }
        }
    }

    // Tant qu'il reste des fenêtres non fixées, on relance le timer
    if (!allOk) {
        var t = new QTimer();
        t.singleShot = true;
        t.timeout.connect(applyKeepAbove);
        t.start(50); // réessaie dans 300ms
    }
}

// premier déclenchement après 3s
var startTimer = new QTimer();
startTimer.singleShot = true;
startTimer.timeout.connect(applyKeepAbove);
startTimer.start(500);
