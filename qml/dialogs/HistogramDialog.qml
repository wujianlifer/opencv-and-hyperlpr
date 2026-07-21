import QtQuick
import QtQuick.Controls
import App 1.0

Dialog {
    title: "灰度直方图"
    modal: false
    width: 460
    height: 500
    standardButtons: Dialog.Close
    contentItem: Item {
        anchors.fill: parent
        Image {
            anchors.centerIn: parent
            width: 400
            height: 400
            fillMode: Image.PreserveAspectFit
            source: processor.hasHistogram
                    ? ("image://processor/histogram?v=" + processor.histogramVersion)
                    : ""
            cache: false
        }
        Label {
            text: "横轴：灰度级(0~255)  纵轴：像素数量"
            color: "#666666"
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        }
    }
}
