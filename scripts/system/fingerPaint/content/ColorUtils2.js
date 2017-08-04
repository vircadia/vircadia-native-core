//converts hsv color into rgb color space, expects hsv with the following ranges
//H(0, 359), S(0, 1), V(0, 1) to R(0, 255), G(0, 255), B(0, 255)
hsv2rgb = function (hsvColor) {
    var c = hsvColor.value * hsvColor.saturation;
    var x = c * (1 - Math.abs((hsvColor.hue/60) % 2 - 1));
    var m = hsvColor.value - c;
    var rgbColor = new Object();
    if (hsvColor.hue >= 0 && hsvColor.hue < 60) {
        rgbColor.red = Math.ceil((c + m) * 255);
        rgbColor.green = Math.ceil((x + m) * 255);
        rgbColor.blue = Math.ceil((0 + m) * 255);
    } else if (hsvColor.hue >= 60 && hsvColor.hue < 120) {
        rgbColor.red = Math.ceil((x + m) * 255);
        rgbColor.green = Math.ceil((c + m) * 255);
        rgbColor.blue = Math.ceil((0 + m) * 255);
    } else if (hsvColor.hue >= 120 && hsvColor.hue < 180) {
        rgbColor.red = Math.ceil((0 + m) * 255);
        rgbColor.green = Math.ceil((c + m) * 255);
        rgbColor.blue = Math.ceil((x + m) * 255);
    } else if (hsvColor.hue >= 180 && hsvColor.hue < 240) {
        rgbColor.red = Math.ceil((0 + m) * 255);
        rgbColor.green = Math.ceil((x + m) * 255);
        rgbColor.blue = Math.ceil((c + m) * 255);
    } else if (hsvColor.hue >= 240 && hsvColor.hue < 300) {
        rgbColor.red = Math.ceil((x + m) * 255);
        rgbColor.green = Math.ceil((0 + m) * 255);
        rgbColor.blue = Math.ceil((c + m) * 255);
    } else if (hsvColor.hue >= 300 && hsvColor.hue < 360) {
        rgbColor.red = Math.ceil((c + m) * 255);
        rgbColor.green = Math.ceil((0 + m) * 255);
        rgbColor.blue = Math.ceil((x + m) * 255);
    } 
    return rgbColor;
}


//converts rgb color into hsv color space, expects rgb with the following ranges
//R(0, 255), G(0, 255), B(0, 255) to H(0, 359), S(0, 1), V(0, 1)
rgb2hsv = function (rgbColor) {
    var r = rgbColor.red   / 255.0;
    var g = rgbColor.green / 255.0;
    var b = rgbColor.blue  / 255.0;

    var cMax = Math.max(r, Math.max(g, b));
    var cMin = Math.min(r, Math.min(g, b));
    var deltaC = cMax - cMin;

    var hsvColor = new Object();
    //hue calculatio
    if (deltaC == 0) {
        hsvColor.hue = 0;

    } else if (cMax == r) {
        hsvColor.hue = 60 * (((g-b)/deltaC) % 6);

    } else if (cMax == g) {
        hsvColor.hue = 60 * (((b-r)/deltaC) + 2);

    } else if (cMax == b) {
        hsvColor.hue = 60 * (((r-g)/deltaC) + 4);
    }

    if (hsvColor.hue < 0) {
        hsvColor.hue += 360;
    }
    //saturation calculation
    if (cMax == 0) {
        hsvColor.saturation = 0;
    } else {
        hsvColor.saturation = (deltaC/cMax);
    }

    hsvColor.value = (cMax);

    return hsvColor;
}