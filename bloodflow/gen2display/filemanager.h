// filemanager.h
#include <QObject>
#include <QQmlApplicationEngine>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QTimer>
#include "filelist.h"

class FileManager : public QObject
{
    Q_OBJECT

public:
    QQmlApplicationEngine* engine;
    FileList scanTree;
    QList<int> indexList;
    QTimer *timer;
    bool flashDriveStatus = false;
    bool nvmeDriveConnected = false;
    explicit FileManager(QQmlApplicationEngine* _engine);

    Q_INVOKABLE QString update(const QString &plaintext);
    Q_INVOKABLE void addIndex(int index);
    Q_INVOKABLE void removeIndex(int index);
    Q_INVOKABLE void deleteList();
    Q_INVOKABLE void transferList();

private slots:
    void checkFlashDrive();


};
