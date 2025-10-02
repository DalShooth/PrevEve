const TARGET = "%1";        // remplacé par Caption
const newX   = %2;          // remplacé par X
const newY   = %3;          // remplacé par Y

for (const w of workspace.stackingOrder) {
    if (w.caption === TARGET && w.moveable) {
        const g = w.frameGeometry;
        w.frameGeometry = { x: newX, y: newY, width: g.width, height: g.height };
    }
}
