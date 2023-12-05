#include <iostream>
#include <QtDebug>
#include <QThread>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <queue>

#include "owconnector.h"

extern QObject * topLevel;
std::queue<QString> commandQueue;

OWConnector::OWConnector(QObject *parent): QObject(parent)
{
    m_quit = false;
    initialized = false;
    init();
}

OWConnector::~OWConnector()
{
    m_quit = false;
    quit();
}

int OWConnector::init(void){
    client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        return 1;
    }
    // Bind socket to address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12325); // Port number

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        close(server_socket);
        initialized = false;
        qInfo() << "Socket binding failed\n";
        return 0;
    }
    qInfo() << "Opening server connection\n";

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        close(server_socket);
        initialized = false;
        qInfo() << "Failed to connect to OW App\n";
        return 0;
    }

    qInfo() << "Lisening for application...\n";

    // Accept client connection
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket == -1) {
        perror("Accept failed");
        close(server_socket);
        initialized = false;
        qInfo() << "Failed to accept client\n";
        return 0;
    }
    initialized = true;
    qInfo() << "App connected!";
    return 1;
}

// main loop. pull data from stream and copy into next queue slot when
//  message is received (message terminated by \n)
void OWConnector::pullData()
{
    QByteArray rxBytes;
    const uint32_t bufLen = 1024;
    uint8_t readBuf[bufLen];
    int32_t len;
    while (m_quit == 0) {
        if ((len = recv(client_socket,readBuf,bufLen,0)) <= 0) {
            usleep(50000);
            continue;
        }
        qDebug() << "Read " << len << " bytes";
        for (int32_t i=0; i<len; i++) {
            char c = readBuf[i];
            if (c == '\r') {
                continue;
            } else
                if (c == '\n') {
                // copy rxBytes to queue then advance write position
                commandQueue.push(QString(rxBytes));
                rxBytes.clear();
                if((char) readBuf[i+1] == '\n'){
                    i=i+1; // TODO, remove this later, the packets that come out of the TDA4 have double \n at the end sometimes
                    qInfo() << "Double newline detected in packet from system!";
                }
            } else {
                rxBytes.append(c);
            }
        }
    }
    closeSerialPort();
}

bool OWConnector::messageAvailable()
{
    return !commandQueue.empty();
}

// return next message and advance queue read position
QString OWConnector::getNextMessage()
{
    QString retVal = commandQueue.front();
    commandQueue.pop();
    return retVal;
}

void OWConnector::sendMsg(QString msg)
{
    msg.append( "\n" );
    // Writing the message as qstring skips \0 because string length
    //  stops at \0
    qInfo() << "send(): Sending " << msg;

    if (send(client_socket, msg.toUtf8().data(), msg.length(), 0) == -1) {
        perror("Send failed");
        close(client_socket);
        close(server_socket);

    }
    return;
}


void OWConnector::closeSerialPort()
{
    if (initialized) {
        close(client_socket);
        close(server_socket);
    }
}

void OWConnector::quit()
{
    m_quit = 1;
}
