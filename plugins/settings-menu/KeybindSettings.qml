pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root
    required property var viewModel
    AudioSettings {
        anchors.fill: parent
        viewModel: root.viewModel
    }
}
