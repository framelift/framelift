import QtQuick
import QtQuick.Controls

Button {
    id: control
    implicitHeight: 36
    padding: 10
    font.pixelSize: 13

    contentItem: Text {
        text: control.text
        color: Theme.text
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: 8
        color: control.down ? Qt.darker(Theme.accent, 1.25)
                            : control.hovered ? Qt.lighter(Theme.accent, 1.08) : Theme.accent
        opacity: control.enabled ? 1 : 0.4
    }
}
