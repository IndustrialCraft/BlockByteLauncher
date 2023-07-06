#include "addserverdialog.h"
#include "ui_addserverdialog.h"

AddServerDialog::AddServerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddServerDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddServerDialog::slot_button_accept);
}
AddServerDialog::~AddServerDialog()
{
    delete ui;
}
void AddServerDialog::slot_button_accept(){
    emit signal_add_server(this->ui->name->text(), this->ui->address->text());
}
