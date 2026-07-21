import QtQuick
import QtQuick.Controls
import App 1.0

// 重要提示 / 错误弹窗（由 processor 的 message 信号驱动）
Dialog {
    id: dlg
    title: "提示"
    standardButtons: Dialog.Ok
    modal: true
    width: 420
    property string text: ""
    contentItem: Label {
        text: dlg.text
        wrapMode: Text.WordWrap
    }
}
