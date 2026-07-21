import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import App 1.0

// 参数面板：根据 processor 暴露的当前操作配置动态显示参数控件
GroupBox {
    Layout.fillWidth: true
    title: processor.operationTitle
    background: Rectangle {
        color: "#ffffff"
        border.color: "#BDBDBD"
        radius: 5
    }
    ColumnLayout {
        width: parent.width
        spacing: 8

        RowLayout {
            visible: processor.param1Label !== ""
            Label { text: processor.param1Label; Layout.preferredWidth: 140 }
            SpinBox {
                id: param1Spin
                from: processor.param1Minimum
                to: processor.param1Maximum
                editable: true
                Component.onCompleted: value = processor.param1Value
                onValueModified: processor.param1Value = value
                Layout.fillWidth: true
                Connections {
                    target: processor
                    function onParam1ValueChanged() { param1Spin.value = processor.param1Value }
                }
            }
        }

        RowLayout {
            visible: processor.needsParam2
            Label { text: processor.param2Label; Layout.preferredWidth: 140 }
            TextField {
                id: param2Field
                Layout.fillWidth: true
                horizontalAlignment: TextInput.AlignHCenter
                validator: DoubleValidator { bottom: processor.param2Minimum; top: processor.param2Maximum; decimals: 2 }
                Component.onCompleted: text = processor.param2Value.toFixed(2)
                onEditingFinished: processor.param2Value = parseFloat(text)
                Connections {
                    target: processor
                    function onParam2ValueChanged() { param2Field.text = processor.param2Value.toFixed(2) }
                }
            }
        }

        RowLayout {
            visible: processor.needsOption
            Label { text: processor.optionLabel; Layout.preferredWidth: 140 }
            ComboBox {
                id: optionCombo
                Layout.fillWidth: true
                model: processor.optionList
                currentIndex: processor.optionIndex
                onActivated: processor.optionIndex = currentIndex
                Connections {
                    target: processor
                    function onOptionIndexChanged() { optionCombo.currentIndex = processor.optionIndex }
                    function onOperationConfigChanged() { optionCombo.currentIndex = processor.optionIndex }
                }
            }
        }

        Label {
            visible: processor.showPlateText
            Layout.fillWidth: true
            text: "识别车牌：" + (processor.plateText ? processor.plateText : "（点击“生成”后显示）")
            font.bold: true
            wrapMode: Text.WrapAnywhere
        }

        Button {
            text: processor.processing ? "处理中…" : "生成"
            enabled: processor.hasImage && !processor.processing
            Layout.alignment: Qt.AlignHCenter
            background: Rectangle {
                color: parent.pressed ? "#1B5E20" : (parent.hovered ? "#388E3C" : "#4CAF50")
                radius: 10
            }
            contentItem: Text { text: processor.processing ? "处理中…" : "生成"; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            onClicked: processor.apply()
        }
    }
}
