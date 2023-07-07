#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPushButton>
#include <QDebug>
#include "addserverdialog.h"
#include <QListWidget>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("BlockByte Launcher");
    connect(ui->addServerButton, &QPushButton::clicked, this, &MainWindow::slot_add_server_dialog);
    connect(ui->refreshServersButton, &QPushButton::clicked, this, &MainWindow::slot_refresh_all);
    this->loadServers();
    auto appData = QDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]);
    appData.mkpath("binaries");
    load_binaries();
    m_running_process = nullptr;
    QSettings settings;
    auto selectedBinary = settings.value("selectedBinary");
    if(selectedBinary.isNull()){
        //todo: select newest
    } else {
        ui->versionSelector->setCurrentText(selectedBinary.toString());
    }
    connect(ui->versionSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::slot_change_binary);
}
MainWindow::~MainWindow()
{
    delete ui;
    if(m_running_process){
        delete m_running_process;
        m_running_process = nullptr;
    }
}
void MainWindow::load_binaries(){
    ui->versionSelector->clear();
    auto appData = QDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]);
    appData.cd("binaries");
    for(const auto& binary : appData.entryList(QDir::Filter::Files)){
        ui->versionSelector->addItem(binary);
    }
}
void MainWindow::slot_change_binary(){
    QSettings settings;
    settings.setValue("selectedBinary", ui->versionSelector->currentText());
}
void MainWindow::slot_add_server_dialog(){
    AddServerDialog* dialog = new AddServerDialog;
    connect(dialog, &AddServerDialog::signal_add_server, this, &MainWindow::slot_add_server);
    dialog->show();
}
void MainWindow::slot_add_server(QString name, QString address){
    ServerEntry* entry = new ServerEntry(*this, QUuid::createUuid(), name, address);
    this->m_servers.insert(entry->getID(), entry);
    this->rebuild_server_list_widget();
    this->saveServers();
}
void MainWindow::rebuild_server_list_widget(){
    this->ui->serverList->clear();
    for(const auto& server : this->m_servers.values()){
        auto uuid = server->getID();
        QListWidgetItem* item = new QListWidgetItem;
        this->ui->serverList->addItem(item);
        auto* layout = new QHBoxLayout;
        auto* textLayout = new QVBoxLayout;
        auto* textWidget = new QWidget;
        textLayout->addWidget(new QLabel(server->getName() + " - " + server->getAddress()));
        bool connectable = server->getRefreshStage()==RefreshStage::Connectable;
        bool assetsDownloaded = server->areAssetsDownloaded();
        textLayout->addWidget(new QLabel(connectable?server->getMotd():server->getRefreshStageString()));
        textWidget->setLayout(textLayout);
        layout->addWidget(textWidget);
        auto* joinButton = new QPushButton(m_running_process?"running":(assetsDownloaded?"join":"no assets"));
        joinButton->setEnabled(connectable && (!m_running_process) && assetsDownloaded);
        connect(joinButton, &QPushButton::clicked, [this,uuid]{this->join_server(uuid);});
        layout->addWidget(joinButton);
        auto* downloadAssetsButton = new QPushButton("download assets");
        downloadAssetsButton->setEnabled(connectable);
        connect(downloadAssetsButton, &QPushButton::clicked, [this,uuid]{this->download_server_assets(uuid);});
        layout->addWidget(downloadAssetsButton);
        auto* refreshButton = new QPushButton("refresh");
        connect(refreshButton, &QPushButton::clicked, [this,uuid]{this->refresh_server(uuid);});
        layout->addWidget(refreshButton);
        auto* deleteButton = new QPushButton("delete");
        connect(deleteButton, &QPushButton::clicked, [this,uuid]{this->delete_server(uuid);});
        layout->addWidget(deleteButton);
        auto* widget = new QWidget;
        widget->setLayout(layout);
        item->setSizeHint(widget->sizeHint());
        this->ui->serverList->setItemWidget(item, widget);
    }
}
void MainWindow::join_server(QUuid uuid){
    auto* server = this->m_servers[uuid];
    if(server && m_running_process == nullptr){
        this->m_running_process = new QProcess();
        auto binary = QDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]);
        binary.cd("binaries");
        auto assets = QDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]);
        assets.cd("assets");
        assets.cd(server->getAssetsHash());
        QStringList arguments;
        arguments << assets.absolutePath() << server->getAddress();
        connect(m_running_process, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, &MainWindow::slot_process_exit);
        connect(m_running_process, &QProcess::readyReadStandardOutput, this, &MainWindow::slot_binary_ready_stdout);
        connect(m_running_process, &QProcess::readyReadStandardError, this, &MainWindow::slot_binary_ready_stderr);
        /*auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("RUST_BACKTRACE", "1");
        this->m_running_process->setProcessEnvironment(environment);*/
        this->m_running_process->start(binary.filePath(ui->versionSelector->currentText()), arguments);
        rebuild_server_list_widget();
    }
}
void MainWindow::download_server_assets(QUuid uuid){
    auto* server = this->m_servers[uuid];
    if(server)
        server->downloadAssets();
}
void MainWindow::slot_process_exit(int exitCode, QProcess::ExitStatus status){
    if(m_running_process){
        delete this->m_running_process;
        this->m_running_process = nullptr;
        rebuild_server_list_widget();
    }
}
void MainWindow::slot_binary_ready_stdout(){
    qDebug() << "[OUT]" << m_running_process->readAllStandardOutput();
}
void MainWindow::slot_binary_ready_stderr(){
    qDebug() << "[ERR]" << m_running_process->readAllStandardError();
}
void MainWindow::refresh_server(QUuid uuid){
    auto* server = this->m_servers[uuid];
    if(server)
        server->refresh();
}
void MainWindow::delete_server(QUuid uuid){
    this->m_servers.remove(uuid);
    this->rebuild_server_list_widget();
    this->saveServers();
}
void MainWindow::slot_refresh_all(){
    for(const auto& server : this->m_servers.values()){
        server->refresh();
    }
}
void MainWindow::saveServers(){
    QSettings settings;
    QJsonObject json;
    for(const auto& server : this->m_servers){
        QJsonObject serverJson;
        serverJson["name"] = server->getName();
        serverJson["address"] = server->getAddress();
        json[server->getID().toString()] = serverJson;
    }
    QJsonDocument jsonDocument(json);
    settings.setValue("savedServers", jsonDocument);
}
void MainWindow::loadServers(){
    QSettings settings;
    auto savedServers = settings.value("savedServers");
    if(!savedServers.isNull()){
        QJsonObject json = savedServers.toJsonDocument().object();
        for(const auto& serverId : json.keys()){
            auto serverJson = json[serverId].toObject();
            auto id = QUuid::fromString(QString::fromStdString(serverId.toStdString()));
            auto* server = new ServerEntry(*this, id, serverJson["name"].toString(), serverJson["address"].toString());
            this->m_servers.insert(id, server);
        }
        slot_refresh_all();
    }
}

ServerEntry::ServerEntry(MainWindow& main_window, QUuid uuid, QString name, QString address, QObject *parent) : m_main_window(main_window), m_name(name), m_address(address), m_id(uuid), QObject(parent){
    connect(&this->m_websocket, &QWebSocket::connected, this, &ServerEntry::slot_ws_connect);
    connect(&this->m_websocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &ServerEntry::slot_ws_error);
    connect(&this->m_websocket, &QWebSocket::textMessageReceived, this, &ServerEntry::slot_ws_receive_text);
    connect(&this->m_websocket, &QWebSocket::binaryMessageReceived, this, &ServerEntry::slot_ws_receive_binary);

    this->refresh();
}
void ServerEntry::refresh(){
    this->m_ws_assets = false;
    this->m_websocket.open(QUrl::fromUserInput("ws://"+this->m_address+"/"));
    this->m_refresh_stage = RefreshStage::Refreshing;
    m_main_window.rebuild_server_list_widget();
}
void ServerEntry::downloadAssets(){
    this->m_ws_assets = true;
    this->m_websocket.open(QUrl::fromUserInput("ws://"+this->m_address+"/"));
}
void ServerEntry::slot_ws_connect(){
    this->m_websocket.sendBinaryMessage(m_ws_assets?QByteArray("\x0D\x02"):QByteArray("\x0D\x01"));
}
void ServerEntry::slot_ws_error(QAbstractSocket::SocketError error){
    if(this->m_refresh_stage == RefreshStage::Refreshing){
        this->m_refresh_stage = RefreshStage::Unreachable;
        m_main_window.rebuild_server_list_widget();
    }
}
void ServerEntry::slot_ws_receive_text(QString json){
    QJsonDocument jsonDocument = QJsonDocument::fromJson(json.toUtf8());
    this->m_motd = jsonDocument["motd"].toString();
    this->m_assets_hash = jsonDocument["client_content_hash"].toString();
    this->m_refresh_stage = RefreshStage::Connectable;
    m_main_window.rebuild_server_list_widget();
}
void ServerEntry::slot_ws_receive_binary(QByteArray data){
    auto assets = QDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]);
    assets.mkdir("assets");
    assets.cd("assets");
    if(assets.cd(this->getAssetsHash())){
        assets.removeRecursively();
        assets.cdUp();
    }
    assets.mkdir(this->getAssetsHash());
    assets.cd(this->getAssetsHash());
    QFile file(assets.filePath("tmp.zip"));
    if ( file.open(QIODevice::WriteOnly) )
    {
        file.write(data);
        file.close();
    }
    {
        QProcess unzipper;
        QStringList unzipperArgs;
        unzipperArgs << assets.filePath("tmp.zip") << "-d" << assets.absolutePath();
        unzipper.start("unzip", unzipperArgs);
        unzipper.waitForFinished();
        assets.remove("tmp.zip");
    }
    {
        auto defaultAssets = assets;
        defaultAssets.cdUp();
        defaultAssets.cd("default");
        for(const auto& file : defaultAssets.entryList(QDir::Filter::Files)){
            QFile::copy(defaultAssets.filePath(file), assets.filePath(file));
        }
    }
    m_main_window.rebuild_server_list_widget();
}
bool ServerEntry::areAssetsDownloaded() const{
    if(getAssetsHash().isEmpty()){
        return false;
    }
    auto assets = QDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]);
    assets.cd("assets");
    return assets.exists(this->getAssetsHash());
}
QUuid const& ServerEntry::getID() const{
    return this->m_id;
}
QString const& ServerEntry::getName() const{
    return this->m_name;
}
QString const& ServerEntry::getAddress() const{
    return this->m_address;
}
QString const& ServerEntry::getMotd() const{
    return this->m_motd;
}
RefreshStage ServerEntry::getRefreshStage() const{
    return this->m_refresh_stage;
}
char const* ServerEntry::getRefreshStageString() const{
    switch(this->m_refresh_stage){
    case RefreshStage::Refreshing:
        return "refreshing";
    case RefreshStage::Unreachable:
        return "unreachable";
    case RefreshStage::Connectable:
        return "connectable";
    }
}
QString const& ServerEntry::getAssetsHash() const{
    return this->m_assets_hash;
}
