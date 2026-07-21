import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import App 1.0

FileDialog {
    title: "打开图像"
    currentFolder: processor.currentFolder
    nameFilters: ["图像文件 (*.jpg *.png *.bmp *.jpeg *.gif *.tif)"]
    onAccepted: {
        var f = selectedFile
        if (!f && selectedFiles.length > 0)
            f = selectedFiles[0]
        processor.openImage(f)
    }
}
