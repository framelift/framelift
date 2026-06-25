import QtQuick
import FrameLift.Controls

Item {
    id: root
    required property var viewModel
    property var vm: viewModel
    anchors.fill: parent
    GlassPanel {
        visible: root.vm !== null && root.vm.state >= 1 && root.vm.state !== 4
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        width: 280
        height: 44
        Text {
            anchors.centerIn: parent
            text: root.vm !== null ? root.vm.statusText : ""
            color: root.vm !== null && root.vm.state === 5 ? Theme.danger : Theme.text
        }
    }
}
