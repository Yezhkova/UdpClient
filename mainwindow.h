#pragma once
#include "IMessenger.h"
#include <netinet/in.h>
#include "qlistwidget.h"
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <thread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow, public MessengerDelegate
{
    Q_OBJECT

public:
    Ui::MainWindow    *ui;
    std::shared_ptr<IMessenger>        m_messenger;

//    int                                 m_sockfd;
//    std::string                         m_username;
    std::string                         m_destName;
//    struct sockaddr_in                  m_servaddr;
//    std::map<std::string, ClientData>   m_clientmap;
//    std::thread                         m_readingThread;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void onUserConnected (std::string username) override;

    void onUserDisconnected(std::string username) override;

    void onMessageReceived(std::string username, std::string message) override;

private slots:

    void on_connectButton_clicked();

    void on_sendButton_clicked();

    void on_disconnectButton_clicked();

    void on_sessionsList_itemClicked(QListWidgetItem *item);

    void addUserSlot(const QString user);

signals:
    void appendMessageText(QString temp);
    void addUser(QString user);
};
