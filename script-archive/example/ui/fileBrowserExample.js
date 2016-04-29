var file = Window.browse("File Browser Example", "/");
if (file === null) {
    Window.alert("No file was selected");
} else {
    Window.alert("Selected file: " + file);
}

file = Window.browse("Relative Directory Example", "./images", "PNG or JPG files(*.png *.jpg);;SVG files (*.svg)");
if (file === null) {
    Window.alert("No file was selected");
} else {
    Window.alert("Selected file: " + file);
}
