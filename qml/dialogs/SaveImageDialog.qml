import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import App 1.0

FileDialog {
    title: "保存图像"
    currentFolder: processor.currentFolder
    fileMode: FileDialog.SaveFile
    nameFilters: ["PNG (*.png)", "JPG (*.jpg)", "XPM (*.xpm)"]
    onAccepted: {
        var f = selectedFile
        if (!f && selectedFiles.length > 0)
            f = selectedFiles[0]
        processor.saveResult(f)
    }
}
