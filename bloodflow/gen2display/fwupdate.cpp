#include "fwupdate.h"
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QString>
#include <unistd.h>

FwUpdate::FwUpdate(QObject *parent) : QObject(parent)
{

}

QString FwUpdate::get_path(QString name)
{
    QString path("/run/media/sd");
    path += device_letter;
    if (partition_number > 0) {
        path += '0' + partition_number;
    }
    path += "/" + name;
    return path;
}

bool FwUpdate::available()
{
    for (device_letter = 'a'; device_letter < 'g'; device_letter++) {
        for (partition_number = 0; partition_number < 7; partition_number++) {
            if (QFile::exists(get_path("Gen2GUI.bin"))) {
                return true;
            }
        }
    }
    return false;
}

bool FwUpdate::copy()
{
    bool rc = QProcess::execute(QString("/data/bin/Gen2GUI"), 
            QStringList({"update"})) == 0;
    sleep(1);
    return rc;
}

