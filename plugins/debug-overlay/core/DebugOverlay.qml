import QtQuick
import FrameLift.Controls

Item {
    id: root
    required property var viewModel
    property var vm: viewModel
    anchors.fill: parent
    visible: vm !== null && vm.open
    GlassPanel {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        width: Math.min(520, text.implicitWidth + 32)
        height: text.implicitHeight + 28
        Text {
            id: text
            anchors.fill: parent
            anchors.margins: 14
            text: root.vm !== null ? root.vm.summary : ""
            color: Theme.text
            font.family: "monospace"
            font.pixelSize: 12
            lineHeight: 1.3
        }
    }
}
