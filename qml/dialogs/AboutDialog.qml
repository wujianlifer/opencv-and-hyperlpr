import QtQuick
import QtQuick.Controls
import App 1.0

Dialog {
    title: "关于"
    standardButtons: Dialog.Ok
    modal: true
    width: 420
    contentItem: Label {
        text: "<h2>QtImageProcess</h2>" +
              "<p>基于 Qt 和 OpenCV 的数字图像处理教学软件</p>" +
              "<p><b>版本：</b>1.0</p>" +
              "<p><b>技术框架：</b></p>" +
              "<ul><li>Qt 6.10.3</li><li>OpenCV 4.12.0</li><li>MNN 3.6.0</li><li>HyperLPR 3.x</li></ul>" +
              "<p><b>许可证：</b>MIT</p><p><b>作者：</b>wj</p>"
        textFormat: Text.RichText
        wrapMode: Text.WordWrap
    }
}
