#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQmlEngine>
#include <QDir>
#include <QDebug>
#include <QIcon>
#include <opencv2/opencv.hpp>
#include "image_processor.hpp"
#include "image_provider.hpp"
#include "globe_define_words.h"

void createDirectoryIfNotExists()
{
    QDir directory(QString::fromStdString(cache_file_path));
    if (!directory.exists())
    {
        if (directory.mkpath(QString::fromStdString(cache_file_path)))
            qDebug() << "Directory created:" << QString::fromStdString(cache_file_path);
        else
            qWarning() << "Failed to create directory:" << QString::fromStdString(cache_file_path);
    }
    else
    {
        qDebug() << "Directory already exists:" << QString::fromStdString(cache_file_path);
    }
}

void clearCacheDirectory()
{
    QDir directory(QString::fromStdString(cache_file_path));
    QStringList files = directory.entryList(QDir::Files);
    foreach (QString filename, files)
    {
        if (directory.remove(filename))
            qDebug() << "Removed file:" << filename;
        else
            qWarning() << "Failed to remove file:" << filename;
    }
}

int main(int argc, char* argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("图像处理实验平台");
    QGuiApplication::setWindowIcon(QIcon(":/icons/icon.png"));

    // 屏蔽 OpenCV 的 INFO 级控制台输出（并行后端等可选插件加载失败的提示属正常噪音）
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    QQuickStyle::setStyle("Fusion");

    createDirectoryIfNotExists();
    clearCacheDirectory();

    ImageProcessor processor;

    // 注册类型，使 QML 可通过 ImageProcessor.BASED_ROTATE 等类型级枚举访问
    qmlRegisterUncreatableType<ImageProcessor>("App", 1, 0, "ImageProcessor",
            "ImageProcessor 仅用于枚举访问");

    QQmlApplicationEngine engine;
    auto* provider = new ImageProvider();
    engine.addImageProvider("processor", provider);
    // ImageProcessor 在 setSourceImage/setResultImage/setHistogramImage 时
    // 同步把图像副本推入 provider（主线程），requestImage 在渲染线程读取，
    // 通过 provider 自持副本 + 互斥锁避免跨线程数据竞争。
    processor.setImageProvider(provider);
    engine.rootContext()->setContextProperty("processor", &processor);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
