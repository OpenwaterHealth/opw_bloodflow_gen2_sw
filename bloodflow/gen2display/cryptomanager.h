// cryptomanager.h
#include <QObject>

class CryptoManager : public QObject
{
    Q_OBJECT

public:
    explicit CryptoManager(QObject *parent = nullptr);

    Q_INVOKABLE QString encrypt(const QString &plaintext);

};
