// KWin_TestHello.js
var SERVICE = "org.example.EveWPreview";
var PATH    = "/EveWPreview";
var IFACE   = "org.example.EveWPreview";

callDBus(SERVICE, PATH, IFACE, "onThumbnailMoved",
    "[CACA]", 0, 0, 0, 0);
