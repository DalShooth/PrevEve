const wins = workspace.stackingOrder;

for (var i = 0; i < wins.length; ++i) {
    var w = wins[i];
    if (!w || !w.caption) continue;

    if (w.caption.indexOf("Thumbnail - ") === 0) { // startsWith
        w.keepAbove = true;
    }
}