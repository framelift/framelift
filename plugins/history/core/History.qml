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

    Drawer {
        id: drawer
        open: root.vm !== null && root.vm.open
        rightSide: true
        drawerWidth: 380
        onXChanged: if (root.vm !== null) root.vm.publishVisibleWidth(Math.max(0, root.width - x))

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Text { text: "History"; color: Theme.text; font.pixelSize: 20; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                ActionButton { text: "Clear"; onClicked: root.vm.Clear() }
                ActionButton { text: "Close"; onClicked: root.vm.togglePanel() }
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: "Search recent files"
                text: root.vm !== null ? root.vm.search : ""
                onTextEdited: if (root.vm !== null) root.vm.search = text
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: root.vm !== null ? root.vm.entries : []
                delegate: Rectangle {
                    id: row
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    height: 68
                    radius: 8
                    color: row.modelData.selected ? "#408B5CF6" : mouse.containsMouse ? "#18FFFFFF" : "transparent"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2
                        Text { text: row.modelData.label; color: Theme.text; elide: Text.ElideMiddle; width: parent.width }
                        Text { text: row.modelData.directory; color: Theme.textMuted; elide: Text.ElideMiddle; width: parent.width; font.pixelSize: 11 }
                        Text { text: row.modelData.meta; color: Theme.textMuted; font.pixelSize: 11 }
                    }
                    MouseArea {
                        id: mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.vm.activateIndex(row.index)
                    }
                }
            }
        }
    }
}
