var width = 0,
    height = 0;

function onUpdate(dt) {
    if (width != Window.innerWidth || height != Window.innerHeight) {
        width = Window.innerWidth;
        height = Window.innerHeight;
        print("New window dimensions: " + width + ", " + height);
    }
}

Script.update.connect(onUpdate);
