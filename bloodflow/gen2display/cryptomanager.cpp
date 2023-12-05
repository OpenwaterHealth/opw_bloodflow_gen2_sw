// cryptomanager.cpp
#include "cryptomanager.h"
#include <QCryptographicHash>


CryptoManager::CryptoManager(QObject *parent) : QObject(parent)
{

}

QString CryptoManager::encrypt(const QString &plaintext)
{
    QByteArray data = plaintext.toUtf8();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(data);

    QByteArray result = hash.result();
    QString hashString = result.toHex();

    return hashString;
}
