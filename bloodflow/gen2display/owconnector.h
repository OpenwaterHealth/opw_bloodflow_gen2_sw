

#ifndef OWCONNECTOR_H
#define OWCONNECTOR_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QDebug>
#include <arpa/inet.h>



class OWConnector : public QObject
{
    Q_OBJECT

public:
    explicit OWConnector(QObject *parent = nullptr);
    ~OWConnector();

    // send a message to the app
    Q_INVOKABLE void sendMsg(QString msg);

    // set quit flag and close port
    Q_INVOKABLE void quit();

    // check to see if we've received a message from the app. returns
    //  true if so, false otherwise
    Q_INVOKABLE bool messageAvailable();

    // returns next available message
    Q_INVOKABLE QString getNextMessage();

public slots:
    // main loop to pull data off serial port
    void pullData();

protected:

    void closeSerialPort();
    int init();

private:
    int initialized;     // == -1 when port not open

    bool m_quit;

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

};

#endif // OWCONNECTOR_H
