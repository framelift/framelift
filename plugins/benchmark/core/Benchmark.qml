pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import FrameLift.Controls

Item {
    id: root
    required property var viewModel
    property var vm: viewModel
    anchors.fill: parent
    visible: vm !== null && vm.open
    GlassPanel {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 18
        width: 330
        height: 250
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Text { text: "Benchmark"; color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                ActionButton { text: "Close"; onClicked: root.vm.close() }
            }
            Text { Layout.fillWidth: true; Layout.fillHeight: true; text: root.vm !== null ? root.vm.summary : ""; color: Theme.textMuted; font.family: "monospace"; lineHeight: 1.4 }
            RowLayout {
                ActionButton { text: "Load file"; onClicked: root.vm.chooseFile() }
                ActionButton { text: "Reset"; onClicked: root.vm.resetRun() }
                Text {
                    text: root.vm !== null && root.vm.complete ? "Complete"
                          : root.vm !== null && root.vm.accumulating ? "Recording" : "Idle"
                    color: root.vm !== null && root.vm.accumulating ? Theme.accent : Theme.textMuted
                }
            }
        }
    }
}
