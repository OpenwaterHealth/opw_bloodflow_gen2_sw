#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlContext>
#include <QScreen>
#include <QDebug>
#include <QThread>
#include <qqml.h>
#include <QtGlobal>
#include <stdlib.h>
#include <syslog.h>
#include <QFileSystemModel>
#include <QStandardItemModel>

#include "serialcontroller.h"
#include "fwupdate.h"
#include "cryptomanager.h"
#include "filemanager.h"
#include "owconnector.h"

QObject * topLevel;

/// Qt Log Message handler
static void qtLogMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{

    QByteArray loc = msg.toUtf8();
    printf("%s\n", loc.constData());
    switch (type) {
    case QtDebugMsg:
        syslog (LOG_DEBUG, "%s", loc.constData());
        break;
    case QtInfoMsg:
        syslog (LOG_INFO, "%s", loc.constData());
        break;
    case QtWarningMsg:
        syslog (LOG_WARNING, "%s", loc.constData());
        break;
    case QtCriticalMsg:
        syslog (LOG_CRIT, "%s", loc.constData());
        break;
    case QtFatalMsg:
        syslog (LOG_ERR, "%s", loc.constData());
        break;
    }
}


int main(int argc, char *argv[])
{
    /// When starting the process
    openlog("OW_GUI_LOG", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

    /// Install Qt Log Message Handler in main.cpp
    qInstallMessageHandler(qtLogMessageHandler);

    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    engine.rootContext()->setContextProperty("screenWidth", 1280);
    engine.rootContext()->setContextProperty("screenHeight", 800);

    qmlRegisterType<CryptoManager>("Crypto", 1, 0, "CryptoManager");

    bool isDevelopmentHost = true;
    if (qgetenv("XDG_RUNTIME_DIR") == "/tmp/0-runtime-dir" ) {
        isDevelopmentHost = false;
        qDebug() << "Detected production environment";
    }
    else { qDebug() << "Detected development environment"; }

    engine.rootContext()->setContextProperty("isDevelopmentHost",isDevelopmentHost);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty()) {
      return -1;
    }

    qDebug() << "Main Init with " << screenGeometry.width() << "width and " << screenGeometry.height() <<"height\n";
    
    topLevel = engine.rootObjects().value(0);
    QQuickWindow * theWindow = qobject_cast < QQuickWindow * > (topLevel);

    if (theWindow == nullptr) {
      qDebug() << "Can't instantiate window";
    }

    QThread* thread = new QThread;
    OWConnector* uart = new OWConnector();
    uart->moveToThread(thread);
    //connect(uart, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
    QObject::connect(thread, SIGNAL(started()), uart, SLOT(pullData()));
    thread->start();

    QObject::connect(&engine, SIGNAL(destroyed()), uart, SLOT(quit()));

    engine.rootContext()->setContextProperty("uart", uart);

    FwUpdate fwUpdate;
    engine.rootContext()->setContextProperty("fwUpdate", &fwUpdate);

    FileManager fm(&engine);
    fm.update("");
    engine.rootContext()->setContextProperty("fileManager",&fm);


    // Send message to the UI to let it know that it just started
    QMetaObject::invokeMethod(theWindow, "onAlive");

    // Start the app - blocks until quit
    int rc = app.exec();

    //Cleanup on quit
    uart->quit();   // close serial connection cleanly
    closelog();
    return rc;
}
