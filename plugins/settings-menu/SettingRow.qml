pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import FrameLift.Controls

GlassPanel {
    id: root

    required property string title
    property string description: ""
    default property alias content: controlSlot.data

    Layout.fillWidth: true
    implicitHeight: row.implicitHeight + 24

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: 12
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: root.title
                color: Theme.text
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            Text {
                visible: root.description.length > 0
                text: root.description
                color: Theme.textMuted
                font.pixelSize: 12
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
        }

        Item {
            id: controlSlot
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
        }
    }
}
