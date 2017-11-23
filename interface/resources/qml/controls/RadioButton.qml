import QtQuick.Controls 1.3 as Original
import QtQuick.Controls.Styles 1.3
import "../styles"
import "."
Original.RadioButton {
    text: "Radio Button"
    style: RadioButtonStyle {
        indicator: FontAwesome {
            text: control.checked ? "\uf046" : "\uf096"
        }
        label: Text {
            text: control.text
            renderType: Text.QtRendering
        }
    }

}
