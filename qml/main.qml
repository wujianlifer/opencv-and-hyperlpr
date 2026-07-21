import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import App 1.0

import "components"
import "dialogs"

ApplicationWindow {
    id: root
    width: 1055
    height: 720
    visible: true
    title: processor.windowTitle
    color: "#F0F0F0"

    // 绑定后端窗口标题
    Connections {
        target: processor
        function onWindowTitleChanged() { root.title = processor.windowTitle }
    }

    // 重要提示/错误用弹窗（监听规范化的 message 信号，避免脆弱的字符串判断）
    Connections {
        target: processor
        function onMessage(text, level) {
            if (level >= ImageProcessor.Warning) {
                alertDialog.text = text
                alertDialog.open()
            }
        }
    }

    menuBar: MainMenu {
        openDialog: openDialog
        saveDialog: saveDialog
        aboutDialog: aboutDialog
    }

    // ----------------------- 工具栏 -----------------------
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton { icon.source: "qrc:/icons/folder.png"; text: "打开"; onClicked: openDialog.open() }
            ToolButton { icon.source: "qrc:/icons/restore.png"; text: "恢复"; onClicked: processor.reset() }
            ToolButton { icon.source: "qrc:/icons/bar-chart.png"; text: "直方图"; onClicked: processor.showHistogram() }
            ToolButton { icon.source: "qrc:/icons/noise.png"; text: "噪声"; onClicked: processor.selectOperation(ImageProcessor.NOISE) }
        }
    }

    // ----------------------- 主内容 -----------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // 双图预览
        RowLayout {
            id: previewRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            ImagePanel {
                id: srcPanel
                title: "原图"
                source: processor.hasImage ? ("image://processor/source?v=" + processor.sourceVersion) : ""
                showEmpty: !processor.hasImage
                emptyText: "点击此处打开图片\n或拖拽图片到此处"
                onClicked: openDialog.open()
            }

            ImagePanel {
                title: "处理结果"
                adaptScale: srcPanel.paintScale
                source: processor.hasResult ? ("image://processor/result?v=" + processor.resultVersion) : ""
                clickEnabled: processor.hasResult
                busy: processor.processing
                onClicked: {
                    imageViewDialog.imageUrl = "image://processor/result?v=" + processor.resultVersion
                    imageViewDialog.open()
                }
            }
        }

        ParameterPanel { }
    }

    // ----------------------- 状态栏 -----------------------
    footer: ToolBar {
        background: Rectangle { color: "#E0E0E0" }
        Label {
            anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 8 }
            text: processor.statusMessage
            elide: Text.ElideRight
        }
    }

    // ----------------------- 对话框实例 -----------------------
    OpenImageDialog { id: openDialog }
    SaveImageDialog { id: saveDialog }
    AboutDialog { id: aboutDialog }
    AlertDialog { id: alertDialog }
    HistogramDialog { id: histogramDialog }

    Connections {
        target: processor
        function onHistogramImageChanged() {
            if (processor.hasHistogram)
                histogramDialog.open()
        }
    }

    ImageViewerDialog { id: imageViewDialog; anchorItem: previewRow }

    // 拖拽图片到窗口任意位置即可打开
    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0)
                processor.openImage(drop.urls[0])
        }
    }
}
