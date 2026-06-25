pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FrameLift.Controls

Item {
    id: root
    required property var viewModel
    property var vm: viewModel
    anchors.fill: parent
    visible: vm !== null && vm.open
    GlassPanel {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 18
        height: Math.min(parent.height * 0.55, 520)
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            RowLayout {
                Layout.fillWidth: true
                Text { text: "Logs"; color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Filter"
                    text: root.vm !== null ? root.vm.filterText : ""
                    onTextEdited: if (root.vm !== null) root.vm.filterText = text
                }
                CheckBox { text: "Performance"; checked: root.vm !== null && root.vm.perfOnly; onToggled: if (root.vm !== null) root.vm.perfOnly = checked }
                ActionButton { text: "Clear"; onClicked: root.vm.clearLines() }
                ActionButton { text: "Close"; onClicked: root.vm.close() }
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.vm !== null ? root.vm.lines : []
                delegate: Text {
                    id: row
                    required property var modelData
                    width: ListView.view.width
                    text: row.modelData.message
                    color: row.modelData.level >= 3 ? Theme.danger : row.modelData.level === 2 ? "#FBBF24" : Theme.textMuted
                    font.family: "monospace"
                    font.pixelSize: 11
                    wrapMode: Text.WrapAnywhere
                }
                onCountChanged: positionViewAtEnd()
            }
        }
    }
}
