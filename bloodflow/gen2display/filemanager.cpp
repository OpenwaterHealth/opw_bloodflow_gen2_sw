// cryptomanager.cpp
#include "filemanager.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
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
#include <QTimer>
#include <QStorageInfo>
#include <QProcess>


FileManager::FileManager(QQmlApplicationEngine* _engine)
{
    engine = _engine;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FileManager::checkFlashDrive);
    timer->start(100);
}

QString FileManager::update(const QString &jsonString)
{
    QString scanFilePath;
    if(nvmeDriveConnected){    
        scanFilePath = "/run/media/nvme0n1/output/device001/";
        qDebug() << "NVMe drive detected";
    }
    else {
        scanFilePath = "/opt/vision_apps/output/device001/";
    }

    QDir directory(scanFilePath);
    QStringList patientFiles = directory.entryList(QDir::NoDotAndDotDot | QDir::Dirs);\
    scanTree.clearModel();

    for (int i = 0; i < patientFiles.length(); ++i) {
        QList<QStandardItem *> patientRow;
        QString patientFolder = patientFiles[i];
        QString readablePatientName = patientFiles[i].remove(0,18); // remove date information from patient name for patient list

        patientRow.append(new QStandardItem(readablePatientName));

        // List scans per patient
        QDir patientDirectory(scanFilePath + "/"+patientFolder);
        QStringList scanFiles = patientDirectory.entryList(QDir::NoDotAndDotDot | QDir::Dirs);
        //qDebug() << "Number of scans" << scanFiles.length() << " in " << patientDirectory.absolutePath();
        for(int j = 0; j < scanFiles.length(); j++){
            QString scanTime = scanFiles[j];
            QString fullPath = scanFilePath + patientFolder + "/" + scanTime;
            scanTree.addRow(readablePatientName,scanTime,fullPath);
          //qDebug() << "File added to tree" + scanFiles[j] + "";
        }
        //qDebug() << "patient name: " + readablePatientName + " added to tree";
    }

    engine->rootContext()->setContextProperty("fileSystemModel", &scanTree);
    scanTree.testUpdate();
    return QString("");
}

void FileManager::addIndex(int index)
{
    indexList.append(index);
}

void FileManager::removeIndex(int index)
{
    int listIndex = indexList.indexOf(index);
    if (listIndex != -1) {
        indexList.removeAt(listIndex);
    }
}

void FileManager::deleteList(){
    // Start file delete modal TODO
    for(int i = 0;i<indexList.length();i++){
        scanTree.removeRow(indexList[i]);
    }
    qDebug() << "Files Deleted";

    indexList.clear();
    this->update("");
}

void FileManager::transferList(){

    // Do not transfer if no flash drive connected
    if(!this->flashDriveStatus){
        return;
    }

    // Start file transfer modal TODO
    QProcess process;
    QString compressTool = "tar";
    process.setWorkingDirectory("/run/media/sda1");
    for(int i = 0;i<indexList.length();i++){
        QString filePath = scanTree.getPath(indexList[i]);
        QStringList arguments;
        arguments << "-cvf";
        arguments << scanTree.getCompressableName(indexList[i]) + ".tgz";
        arguments << filePath;
        process.start(compressTool, arguments);
        process.waitForFinished();
        process.start("sync");
        qDebug() << "Transferring: " << filePath << " at index " << indexList[i];
    }
    qDebug() << "Files Transferred";

    indexList.clear();
}

QString bytesToGigabytes(qint64 bytes)
{
    double gigabytes = static_cast<double>(bytes) / (1024 * 1024 * 1024); // Convert to gigabytes
    QString formatted = QString::number(gigabytes, 'f', 1); // Format with one decimal point
    return formatted + " GB";
}

void FileManager::checkFlashDrive()
{
    // Check on flash drive and internal storage statistics
    bool flashDriveConnected = false;
    QString flashDriveFreeStorage = "";
    QString internalStorageFree = "";

    // Iterate through the list to find and spit out drive info
    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &drive : drives) {
        if (drive.isReady()) {
            if(drive.displayName() == "/run/media/nvme0n1"){ // nvme drive will come up second and overwrite if connected, otherwise will show root drive
                internalStorageFree = bytesToGigabytes(drive.bytesAvailable());
                nvmeDriveConnected = true;

            }
            if(drive.displayName() == "/")
                internalStorageFree = bytesToGigabytes(drive.bytesAvailable());
            if(drive.displayName() == "/run/media/sda1") {
                flashDriveFreeStorage = bytesToGigabytes((drive.bytesAvailable()));
                flashDriveConnected = true;
            }
        }
    }
    this->flashDriveStatus = flashDriveConnected;
    // Get handle on UI elements and write to them
    QObject *rootObject = engine->rootObjects().first();
    if(rootObject){
        rootObject->setProperty("flashDriveConnected", flashDriveConnected);
        rootObject->setProperty("flashDriveFreeStorage", flashDriveFreeStorage);
        rootObject->setProperty("internalStorageFree", internalStorageFree);
    }
    else {
        qDebug() << "could not find root object";
    }
}
