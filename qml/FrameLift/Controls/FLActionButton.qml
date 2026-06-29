import QtQuick
import QtQuick.Controls

Button {
    id: control
    implicitHeight: 36
    padding: 10
    font.pixelSize: 13

    property color accentColor: FLTheme.accent

    contentItem: Text {
        text: control.text
        color: FLTheme.text
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: 8
        color: control.down ? Qt.darker(control.accentColor, 1.25)
                            : control.hovered ? Qt.lighter(control.accentColor, 1.08) : control.accentColor
        opacity: control.enabled ? 1 : 0.4
    }
}
