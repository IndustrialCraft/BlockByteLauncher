#pragma once

#include <QMainWindow>
#include <QUuid>
#include <QWebSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ServerEntry;
enum RefreshStage{
    Refreshing = 0,
    Unreachable = 1,
    Connectable = 2
};
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    QHash<QUuid,ServerEntry*> m_servers;
private slots:
    void slot_add_server_dialog();
    void slot_add_server(QString name, QString address);
    void slot_refresh_all();
private:
    void join_server(QUuid uuid);
    void refresh_server(QUuid uuid);
    void delete_server(QUuid uuid);
public:
    void rebuild_server_list_widget();
    void saveServers();
    void loadServers();
};

class ServerEntry : public QObject{
    Q_OBJECT
public:
    ServerEntry(MainWindow& main_window, QUuid id, QString name, QString address, QObject *parent = nullptr);
private:
    MainWindow& m_main_window;
    QUuid m_id;
    QString m_name;
    QString m_address;
    QWebSocket m_websocket;
    QString m_motd;
    RefreshStage m_refresh_stage;
public:
    void refresh();
private slots:
    void slot_ws_connect();
    void slot_ws_error(QAbstractSocket::SocketError error);
    void slot_ws_receive_text(QString json);
public:
    QUuid const& getID() const;
    QString const& getName() const;
    QString const& getAddress() const;
    QString const& getMotd() const;
    RefreshStage getRefreshStage() const;
    char const* getRefreshStageString() const;
}
;

