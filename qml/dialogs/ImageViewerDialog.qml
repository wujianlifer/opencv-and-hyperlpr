import QtQuick
import QtQuick.Controls
import QtQuick.Window
import App 1.0

// 大图预览（滚轮缩放 · 拖拽平移 · 双击复位 · 点击关闭）
// 用普通 Item 作裁剪视口，平移改 image.x/y、缩放改 image.scale，完全自控、无回弹
// 以 Popup 形式覆盖在「原图 + 处理结果图」双图预览区域上，尺寸/位置跟随该区域
Popup {
    id: dlg
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0
    // 对齐目标：主窗口中的双图预览 RowLayout
    property var anchorItem: null
    property string imageUrl: ""

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1 }

    // 打开前把位置/尺寸对齐 anchorItem（Popup 的 x/y 相对其父级=ApplicationWindow 中央 contentItem）
    onAboutToShow: {
        if (anchorItem) {
            var p = anchorItem.mapToItem(dlg.parent, 0, 0)
            dlg.x = p.x
            dlg.y = p.y
            dlg.width = anchorItem.width
            dlg.height = anchorItem.height
        }
    }

    // 操作提示
    Label {
        text: "大图预览 · 滚轮缩放 · 拖拽平移 · 双击复位 · 点击关闭"
        color: "#cccccc"
        font.pixelSize: 12
        anchors { top: parent.top; left: parent.left; margins: 6 }
        z: 2
    }

    // 裁剪视口
    Item {
        id: viewport
        anchors.fill: parent
        clip: true

        Image {
            id: image
            source: dlg.imageUrl
            cache: false
            fillMode: Image.PreserveAspectFit
            transformOrigin: Item.TopLeft
            width: sourceSize.width
            height: sourceSize.height
            scale: 1
            smooth: true

            // 适配缩放：图片大于视口则缩放到完整可见；小于视口则保持原尺寸
            function fitScale() {
                if (sourceSize.width <= 0 || sourceSize.height <= 0)
                    return 1
                var f = Math.min(viewport.width / sourceSize.width,
                                 viewport.height / sourceSize.height)
                return f < 1 ? f : 1
            }

            // 边界钳制（软）：横竖均可自由平移，仅防止图片被整个拖出视口
            function clampPan() {
                var sw = sourceSize.width * scale
                var sh = sourceSize.height * scale
                var minX = Math.min(0, viewport.width - sw)
                var maxX = Math.max(0, viewport.width - sw)
                image.x = Math.max(minX, Math.min(maxX, image.x))
                var minY = Math.min(0, viewport.height - sh)
                var maxY = Math.max(0, viewport.height - sh)
                image.y = Math.max(minY, Math.min(maxY, image.y))
            }

            // 复位：适配缩放并居中（居中等钳制兜底）
            function recenter() {
                scale = fitScale()
                image.x = (viewport.width - sourceSize.width * scale) / 2
                image.y = (viewport.height - sourceSize.height * scale) / 2
                clampPan()
            }

            onStatusChanged: if (status === Image.Ready) recenter()
        }

        // 视口尺寸确定后再校正一次居中，避免初始 recenter 用到旧的视口尺寸
        Connections {
            target: viewport
            function onWidthChanged() { if (image.status === Image.Ready) image.recenter() }
            function onHeightChanged() { if (image.status === Image.Ready) image.recenter() }
        }

        MouseArea {
            anchors.fill: parent

            // 单击关闭（延时，避免与双击冲突；拖拽时不关闭）
            property bool dragged: false
            onClicked: if (!dragged) closeTimer.restart()

            // 双击复位适配
            onDoubleClicked: {
                closeTimer.stop()
                image.recenter()
            }

            // 滚轮缩放（以光标为锚点）
            onWheel: function(wheel) {
                var oldScale = image.scale
                var factor = wheel.angleDelta.y > 0 ? 1.15 : 1 / 1.15
                var ns = Math.min(Math.max(oldScale * factor, 0.05), 8)
                // 光标在图片原生坐标系中的位置（以图片左上角为原点）
                var cx = (wheel.x - image.x) / oldScale
                var cy = (wheel.y - image.y) / oldScale
                image.scale = ns
                image.x = wheel.x - cx * ns
                image.y = wheel.y - cy * ns
                image.clampPan()
            }

            // 拖拽平移（直接移动 image.x/y，无回弹）
            property point last: Qt.point(0, 0)
            onPressed: function(mouse) {
                last = Qt.point(mouse.x, mouse.y)
                dragged = false
            }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    var dx = mouse.x - last.x
                    var dy = mouse.y - last.y
                    if (Math.abs(dx) > 3 || Math.abs(dy) > 3)
                        dragged = true
                    image.x += dx
                    image.y += dy
                    image.clampPan()
                    last = Qt.point(mouse.x, mouse.y)
                }
            }
        }
    }

    Timer {
        id: closeTimer
        interval: 250
        onTriggered: dlg.close()
    }

    // 打开后延迟校正：兜底首帧 sourceSize/视口尺寸尚未就绪导致未适配的情况
    Timer {
        id: fitTimer
        interval: 120
        onTriggered: image.recenter()
    }

    onOpened: {
        image.recenter()
        fitTimer.restart()
    }
}
