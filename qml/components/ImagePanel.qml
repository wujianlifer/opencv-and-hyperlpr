import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 通用图像预览面板：原图 / 处理结果 共用
Frame {
    id: panel
    property string title: ""
    property url source: ""
    property bool showEmpty: false
    property string emptyText: ""
    property bool clickEnabled: true
    property bool busy: false
    // 显示模式：adaptScale<0 自适应填满(PreserveAspectFit，能看全)；
    // adaptScale>=0 时按真实像素×该比例显示。
    // 结果图把 adaptScale 设为原图面板的 paintScale，即"在原图自适应比例基础上继续缩放"
    property real adaptScale: -1
    property real paintScale: 1.0   // 对外暴露本面板实际显示比例（原图自适应用）
    signal clicked()

    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    background: Rectangle { color: "#2b2b2b" }

    Label {
        text: panel.title
        color: "#cccccc"
        anchors { top: parent.top; left: parent.left; margins: 6 }
    }

    Image {
        id: img
        source: panel.source
        cache: false
        clip: true
        // 自适应模式：PreserveAspectFit 填满；缩放模式：按真实像素×adaptScale 显示
        fillMode: panel.adaptScale < 0 ? Image.PreserveAspectFit : Image.Stretch
        anchors.fill: panel.adaptScale < 0 ? parent : undefined
        anchors.centerIn: panel.adaptScale < 0 ? undefined : parent
        anchors.margins: panel.adaptScale < 0 ? 24 : 0
        width: panel.adaptScale < 0 ? undefined : sourceSize.width * panel.adaptScale
        height: panel.adaptScale < 0 ? undefined : sourceSize.height * panel.adaptScale
        // 自适应模式下，把实际显示比例(painted/原图)暴露给外部，供结果图对齐
        onPaintedWidthChanged: panel.paintScale = (img.sourceSize.width > 0)
                ? img.paintedWidth / img.sourceSize.width : 1
    }

    Text {
        visible: panel.showEmpty
        text: panel.emptyText
        color: "#777777"
        font.pixelSize: 15
        horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
    }

    BusyIndicator {
        visible: panel.busy
        running: panel.busy
        anchors.centerIn: parent
        width: 64; height: 64
    }

    MouseArea {
        anchors.fill: parent
        enabled: panel.clickEnabled
        cursorShape: Qt.PointingHandCursor
        onClicked: panel.clicked()
    }
}
