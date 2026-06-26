pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FrameLift.Controls
import FrameLift.Plugins.SettingsMenu

Item {
    id: root
    required property var viewModel

    ListView {
        anchors.fill: parent
        model: root.viewModel !== null ? root.viewModel.fields : []
        spacing: 8
        clip: true

        delegate: SettingRow {
            id: fieldDelegate
            required property var modelData
            width: ListView.view.width
            title: fieldDelegate.modelData.label
            description: fieldDelegate.modelData.description

            Loader {
                sourceComponent: fieldDelegate.modelData.type === 0 ? boolEditor
                               : fieldDelegate.modelData.type === 1 ? intEditor
                               : fieldDelegate.modelData.type === 2 ? floatEditor
                                                                    : stringEditor
            }

            Component {
                id: boolEditor
                Switch {
                    checked: fieldDelegate.modelData.value
                    onToggled: root.viewModel.setFieldValue(fieldDelegate.modelData.key, checked)
                }
            }
            Component {
                id: intEditor
                SpinBox {
                    value: fieldDelegate.modelData.value
                    from: -100000
                    to: 100000
                    implicitWidth: 140
                    onValueModified: root.viewModel.setFieldValue(fieldDelegate.modelData.key, value)
                }
            }
            Component {
                id: floatEditor
                TextField {
                    text: Number(fieldDelegate.modelData.value).toString()
                    validator: DoubleValidator {}
                    implicitWidth: 180
                    onEditingFinished: root.viewModel.setFieldValue(fieldDelegate.modelData.key, Number(text))
                }
            }
            Component {
                id: stringEditor
                TextField {
                    text: fieldDelegate.modelData.value
                    implicitWidth: 240
                    onEditingFinished: root.viewModel.setFieldValue(fieldDelegate.modelData.key, text)
                }
            }
        }
    }
}
