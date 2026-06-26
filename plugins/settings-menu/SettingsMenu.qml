pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import FrameLift.Controls

// The compositor owns this invisible anchor item in the main window. The actual
// settings surface is a separate top-level Qt Quick window.
Item {
    id: root
    anchors.fill: parent
    visible: false
    required property var viewModel
    property var vm: viewModel

    ApplicationWindow {
        id: settingsWindow
        width: 980
        height: 680
        minimumWidth: 760
        minimumHeight: 520
        visible: root.vm !== null && root.vm.open
        title: "FrameLift Settings"
        color: Theme.canvas

        onClosing: function(close) {
            close.accepted = false
            if (root.vm !== null)
                root.vm.closeQml()
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                color: "#0D0D14"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14

                    Text {
                        text: "Settings"
                        color: Theme.text
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: root.vm !== null ? root.vm.pages : []
                        spacing: 4

                        delegate: Rectangle {
                            id: pageDelegate
                            required property string modelData
                            width: ListView.view.width
                            height: 38
                            radius: 8
                            color: root.vm !== null && root.vm.activePage === modelData
                                   ? "#408B5CF6"
                                   : pageMouse.containsMouse ? "#18FFFFFF" : "transparent"

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                text: pageDelegate.modelData.charAt(0).toUpperCase()
                                      + pageDelegate.modelData.slice(1)
                                color: Theme.text
                                verticalAlignment: Text.AlignVCenter
                            }
                            MouseArea {
                                id: pageMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    if (root.vm !== null)
                                        root.vm.activePage = pageDelegate.modelData
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 24

                Text {
                    property string page: root.vm !== null ? root.vm.activePage : ""
                    text: page.length > 0 ? page.charAt(0).toUpperCase() + page.slice(1) : ""
                    color: Theme.text
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }

                ListView {
                    visible: root.vm === null || root.vm.activePage !== "plugins"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.vm !== null ? root.vm.activeFields : []
                    spacing: 8
                    clip: true

                    delegate: GlassPanel {
                        id: fieldDelegate
                        required property var modelData
                        width: ListView.view.width
                        height: 58

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            Text {
                                text: fieldDelegate.modelData.label
                                color: Theme.text
                                Layout.fillWidth: true
                            }
                            Loader {
                                sourceComponent: fieldDelegate.modelData.type === 0 ? boolEditor
                                               : fieldDelegate.modelData.type === 1 ? intEditor
                                               : fieldDelegate.modelData.type === 2 ? floatEditor
                                                                                   : stringEditor
                            }
                        }

                        Component {
                            id: boolEditor
                            Switch {
                                checked: fieldDelegate.modelData.value
                                onToggled: {
                                    if (root.vm !== null)
                                        root.vm.setFieldValue(fieldDelegate.modelData.key, checked)
                                }
                            }
                        }
                        Component {
                            id: intEditor
                            SpinBox {
                                value: fieldDelegate.modelData.value
                                from: -100000
                                to: 100000
                                onValueModified: {
                                    if (root.vm !== null)
                                        root.vm.setFieldValue(fieldDelegate.modelData.key, value)
                                }
                            }
                        }
                        Component {
                            id: floatEditor
                            TextField {
                                text: Number(fieldDelegate.modelData.value).toString()
                                validator: DoubleValidator {}
                                onEditingFinished: {
                                    if (root.vm !== null)
                                        root.vm.setFieldValue(fieldDelegate.modelData.key, Number(text))
                                }
                            }
                        }
                        Component {
                            id: stringEditor
                            TextField {
                                text: fieldDelegate.modelData.value
                                onEditingFinished: {
                                    if (root.vm !== null)
                                        root.vm.setFieldValue(fieldDelegate.modelData.key, text)
                                }
                            }
                        }
                    }
                }

                ListView {
                    visible: root.vm !== null && root.vm.activePage === "plugins"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.vm !== null ? root.vm.plugins : []
                    spacing: 8
                    clip: true

                    delegate: GlassPanel {
                        id: pluginDelegate
                        required property var modelData
                        width: ListView.view.width
                        height: 72

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3
                                Text {
                                    text: pluginDelegate.modelData.name
                                    color: Theme.text
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: pluginDelegate.modelData.description
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: pluginDelegate.modelData.loadFailed
                                          ? "Failed to load"
                                          : pluginDelegate.modelData.loaded ? "Loaded" : "Pending restart"
                                    color: pluginDelegate.modelData.loadFailed ? Theme.danger : Theme.textMuted
                                    font.pixelSize: 12
                                }
                            }

                            Switch {
                                checked: pluginDelegate.modelData.enabled
                                onToggled: {
                                    if (root.vm !== null)
                                        root.vm.setPluginEnabled(pluginDelegate.modelData.id, checked)
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Item { Layout.fillWidth: true }
                    ActionButton {
                        text: "Reset all"
                        onClicked: if (root.vm !== null) root.vm.resetAllQml()
                    }
                    ActionButton {
                        text: root.vm !== null && root.vm.dirty ? "Save *" : "Save"
                        onClicked: if (root.vm !== null) root.vm.saveQml()
                    }
                    ActionButton {
                        text: "Close"
                        onClicked: if (root.vm !== null) root.vm.closeQml()
                    }
                }
            }
        }
    }
}
