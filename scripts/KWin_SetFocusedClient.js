const TARGET = "%1"; // remplacé par Caption

for (const w of workspace.stackingOrder) {
    if (w.caption === TARGET) {
        workspace.activeWindow = w;
        break;
    }
}
