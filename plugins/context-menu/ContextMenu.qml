pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: root
    required property var viewModel
    property var vm: viewModel
    anchors.fill: parent
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        propagateComposedEvents: true
        onClicked: menu.popup()
    }
    Menu {
        id: menu
        MenuItem { text: "Open File"; onTriggered: root.vm.openFile() }
        MenuItem { text: "Open Network Stream…"; onTriggered: root.vm.openNetwork() }
        MenuSeparator {}
        MenuItem { text: "Play / Pause"; onTriggered: root.vm.togglePause() }
        MenuItem { text: "Toggle Fullscreen"; onTriggered: root.vm.toggleFullscreen() }
        MenuItem { text: "Mute"; checkable: true; checked: root.vm !== null && root.vm.muted; onTriggered: root.vm.toggleMute() }
        MenuItem { text: "Normalize"; checkable: true; checked: root.vm !== null && root.vm.normalizeEnabled; onTriggered: root.vm.toggleNormalize() }
        MenuItem { text: "Subtitles"; checkable: true; checked: root.vm !== null && root.vm.subtitlesEnabled; onTriggered: root.vm.toggleSubtitles() }
        MenuSeparator {}
        Repeater {
            model: root.vm !== null ? root.vm.extraItems : []
            MenuItem {
                required property var modelData
                text: modelData.hotkey ? modelData.label + "    " + modelData.hotkey : modelData.label
                onTriggered: root.vm.invokeExtra(modelData.index)
            }
        }
        MenuSeparator {}
        MenuItem { text: "Quit"; onTriggered: root.vm.quit() }
    }
}
