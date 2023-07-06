#ifndef ADDSERVERDIALOG_H
#define ADDSERVERDIALOG_H

#include <QDialog>

namespace Ui {
class AddServerDialog;
}

class AddServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddServerDialog(QWidget *parent = nullptr);
    ~AddServerDialog();
private:
    Ui::AddServerDialog *ui;
private slots:
    void slot_button_accept();
signals:
    void signal_add_server(QString name, QString address);
};

#endif // ADDSERVERDIALOG_H
