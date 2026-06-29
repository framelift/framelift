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

    FLDrawer {
        id: drawer
        open: root.vm !== null && root.vm.open
        drawerWidth: 340
        onXChanged: if (root.vm !== null) root.vm.publishVisibleWidth(Math.max(0, width + x))

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                    text: "Playlist"
                    color: FLTheme.text
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }
                Text {
                    text: root.vm !== null && root.vm.currentIndex >= 0
                          ? (root.vm.currentIndex + 1) + " / " + root.vm.entries.length
                          : root.vm !== null ? root.vm.entries.length : 0
                    color: FLTheme.textMuted
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                FLActionButton { text: "Reload"; implicitHeight: 28; padding: 8; font.pixelSize: 12; onClicked: root.vm.Reload() }
                FLActionButton {
                    text: root.vm !== null && root.vm.shuffleEnabled ? "Shuffle on" : "Shuffle"
                    implicitHeight: 28; padding: 8; font.pixelSize: 12
                    onClicked: root.vm.ToggleShuffle()
                }
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
                    height: 44
                    radius: 6
                    color: row.modelData.current ? "#408B5CF6"
                          : mouse.containsMouse ? "#18FFFFFF" : "transparent"
                    Text {
                        anchors.fill: parent
                        anchors.margins: 12
                        text: row.modelData.label
                        color: FLTheme.text
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
