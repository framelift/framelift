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
        drawerWidth: 340
        onXChanged: if (root.vm !== null) root.vm.publishVisibleWidth(Math.max(0, width + x))

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Playlist"
                    color: Theme.text
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.vm !== null && root.vm.currentIndex >= 0
                          ? (root.vm.currentIndex + 1) + " / " + root.vm.entries.length
                          : root.vm !== null ? root.vm.entries.length : 0
                    color: Theme.textMuted
                }
            }

            RowLayout {
                Layout.fillWidth: true
                ActionButton { text: "Reload"; onClicked: root.vm.Reload() }
                ActionButton {
                    text: root.vm !== null && root.vm.shuffleEnabled ? "Shuffle on" : "Shuffle"
                    onClicked: root.vm.ToggleShuffle()
                }
                Item { Layout.fillWidth: true }
                ActionButton { text: "Close"; onClicked: root.vm.togglePanel() }
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
                    height: 46
                    radius: 8
                    color: row.modelData.current ? "#408B5CF6"
                          : mouse.containsMouse ? "#18FFFFFF" : "transparent"
                    Text {
                        anchors.fill: parent
                        anchors.margins: 12
                        text: row.modelData.label
                        color: Theme.text
                        elide: Text.ElideMiddle
                        verticalAlignment: Text.AlignVCenter
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
