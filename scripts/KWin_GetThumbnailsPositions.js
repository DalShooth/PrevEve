var SERVICE = "org.example.EveWPreview";
var PATH    = "/EveWPreview";
var IFACE   = "org.example.EveWPreview";

const wins = workspace.stackingOrder;
var results = []; // tableau de résultats

for (var i = 0; i < wins.length; ++i) {
    var w = wins[i];
    if (!w || !w.caption) continue;

    if (w.caption.indexOf("Thumbnail-") === 0) {
        var g = w.frameGeometry || w.geometry || w;
        var x = Math.floor(g.x);
        var y = Math.floor(g.y);

        // push dans le tableau
        results.push([w.caption, x, y]);
    }
}

// 🔹 Un seul callDBus avec le tableau complet
if (results.length > 0) {
    callDBus.apply(null, [SERVICE, PATH, IFACE, "GetThumbnailsPositionsResponse"].concat(results));
}

//test supplémentaire
// callDBus(SERVICE, PATH, IFACE, "GetThumbnailsPositionsResponse",
//     "[CACA]", 0, 0);
