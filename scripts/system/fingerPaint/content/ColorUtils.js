// creates color value from hue, saturation, brightness, alpha
function hsba(h, s, b, a) {
    var lightness = (2 - s)*b;
    var satHSL = s*b/((lightness <= 1) ? lightness : 2 - lightness);
    lightness /= 2;
    return Qt.hsla(h, satHSL, lightness, a);
}

function clamp(val, min, max){
    return Math.max(min, Math.min(max, val)) ;
}

function mix(x, y , a)
{
    return x * (1 - a) + y * a ;
}

function hsva2rgba(hsva) {
    var c = hsva.z * hsva.y ;
    var x = c * (1 - Math.abs( (hsva.x * 6) % 2 - 1 )) ;
    var m = hsva.z - c ;

    if (hsva.x < 1/6 )
        return Qt.vector4d(c+m, x+m, m, hsva.w) ;
    else if (hsva.x < 1/3 )
        return Qt.vector4d(x+m, c+m, m, hsva.w) ;
    else if (hsva.x < 0.5 )
        return Qt.vector4d(m, c+m, x+m, hsva.w) ;
    else if (hsva.x < 2/3 )
        return Qt.vector4d(m, x+m, c+m, hsva.w) ;
    else if (hsva.x < 5/6 )
        return Qt.vector4d(x+m, m, c+m, hsva.w) ;
    else
        return Qt.vector4d(c+m, m, x+m, hsva.w) ;

}

function rgba2hsva(rgba)
{
    var r = rgba.x;
    var g = rgba.y;
    var b = rgba.z;
    var max = Math.max(r, g, b), min = Math.min(r, g, b);
    var h, s, v = max;

    var d = max - min;
    s = max === 0 ? 0 : d / max;

    if(max == min){
        h = 0; // achromatic
    } else{
        switch(max){
            case r:
                h = (g - b) / d + (g < b ? 6 : 0);
                break;
            case g:
                h = (b - r) / d + 2;
                break;
            case b:
                h = (r - g) / d + 4;
                break;
        }
        h /= 6;
    }

    return Qt.vector4d(h, s, v, rgba.w);
}


// extracts integer color channel value [0..255] from color value
function getChannelStr(clr, channelIdx) {
    return parseInt(clr.toString().substr(channelIdx*2 + 1, 2), 16);
}

//convert to hexa with nb char
function intToHexa(val , nb)
{
    var hexaTmp = val.toString(16) ;
    var hexa = "";
    var size = hexaTmp.length
    if (size < nb )
    {
        for(var i = 0 ; i < nb - size ; ++i)
        {
            hexa += "0"
        }
    }
    return hexa + hexaTmp
}

function hexaFromRGBA(red, green, blue, alpha)
{
    return intToHexa(Math.round(red * 255), 2)+intToHexa(Math.round(green * 255), 2)+intToHexa(Math.round(blue * 255), 2);
}
