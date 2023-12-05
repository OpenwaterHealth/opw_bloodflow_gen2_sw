#ifndef FWUPDATE_H
#define FWUPDATE_H

#include <QObject>

class FwUpdate : public QObject
{
    Q_OBJECT
protected:
    char device_letter;
    int partition_number;
    QString get_path(QString name);
public:
    explicit FwUpdate(QObject *parent = 0);

    Q_INVOKABLE bool available();
    Q_INVOKABLE bool copy();

signals:

public slots:
};

#endif // FWUPDATE_H
