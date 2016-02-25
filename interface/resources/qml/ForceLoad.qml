import QtQuick 2.0
import QtMultimedia 5.5

Item {
    Audio {
        id: audio
        autoLoad: true
        autoPlay: true
        loops: Audio.Infinite
    }
}

