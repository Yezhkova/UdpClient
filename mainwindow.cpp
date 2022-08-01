#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <arpa/inet.h>
#include <QDebug>
#include <iostream>
#include <sstream>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <unistd.h>

#define MAXLINE  (1024*1024)
#define LOG(expr) {std::stringstream ss; ss << expr; qDebug() << QString::fromStdString(ss.str());}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this, &MainWindow::appendMessageText, ui->feed, &QTextEdit::append);
    connect(this, &MainWindow::addUser, this, &MainWindow::addUserSlot);
    connect(this, &MainWindow::deleteUser, this, &::MainWindow::deleteUserSlot);
    m_messenger = createUdpMessenger(*this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addUserSlot(const QString user)
{
    ui->sessionsList->addItem(user);
}

void MainWindow::deleteUserSlot(const QString user)
{
    QList <QListWidgetItem *> list = ui->sessionsList->findItems(user, Qt::MatchCaseSensitive);
    int r = ui->sessionsList->row(list[0]);
    QListWidgetItem *it = ui->sessionsList->takeItem(r);
    delete it;
}


void MainWindow::onUserConnected(std::string username)
{
    ui->sessionsList->addItem(QString::fromStdString(username));
}

void MainWindow::on_connectButton_clicked()
{    
    std::string username = ui->myUsernameEdit->text().toStdString();
    std::string address = ui->addressEdit->text().toStdString();
    uint16_t port = ui->portEdit->text().toUShort();
    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);

    m_messenger->connect(username, address, port);
}

void MainWindow::onMessageReceived(std::string username, std::string message)
{
    std::string tmp = "<html><span style=\"color:#0033cc;font-weight:bold\">"
                                          + username + "</span>:<br><span style=\"white-space:pre\">"
                                          + message + "</span></html>";
    emit appendMessageText(QString::fromStdString(tmp));
}


void MainWindow::on_sendButton_clicked()
{
    if(!ui->typeMessage->toPlainText().toStdString().empty())
    {

        std::string tmp = "<html><span style=\"color:#8585ad;font-weight:bold\">to "
                                              + m_destName + "</span>: <span style=\"color:#999999;white-space:pre\">"
                                              + ui->typeMessage->toPlainText().toStdString() + "</span></html>";
        emit appendMessageText(QString::fromStdString(tmp));
        m_messenger->sendMessage(m_destName, ui->typeMessage->toPlainText().toStdString());
        ui->typeMessage->clear();
    }
}

void MainWindow::onUserDisconnected(std::string username)
{

    emit deleteUser(QString::fromStdString(username));
}


void MainWindow::on_disconnectButton_clicked()
{
    m_messenger->disconnect();
    this->close();
}


void MainWindow::on_sessionsList_itemClicked(QListWidgetItem *item)
{
    if(!ui->sendButton->isEnabled())
    {
            ui->sendButton->setEnabled(true);
    }
    m_destName = ui->sessionsList->currentItem()->text().toStdString();
}



