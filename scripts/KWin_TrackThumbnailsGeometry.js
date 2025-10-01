// KWin_TrackAllMoves.js
var SERVICE = "org.example.EveWPreview";
var PATH    = "/EveWPreview";
var IFACE   = "org.example.EveWPreview";

const wins = workspace.stackingOrder;

for (var i = 0; i < wins.length; ++i) {
    var w = wins[i];
    if (!w || !w.caption) continue;

    if (w.caption.indexOf("Thumbnail-") === 0) { // startsWith
        var g  = w.frameGeometry || w.geometry || w;  // <<< il faut remettre cette ligne
        var x  = Math.floor(g.x);
        var y  = Math.floor(g.y);

        callDBus(SERVICE, PATH, IFACE, "GetThumbnailsPositionsResponse",
            w.caption, x, y);
    }
}

// test supplémentaire
// callDBus(SERVICE, PATH, IFACE, "GetThumbnailsPositionsResponse",
//     "[CACA]", 0, 0);
