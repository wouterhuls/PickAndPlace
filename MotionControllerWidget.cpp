#include "MotionControllerWidget.h"
#include "ui_MotionControllerWidget.h"

MotionControllerWidget::MotionControllerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MotionControllerWidget)
{
    ui->setupUi(this);
}

MotionControllerWidget::~MotionControllerWidget()
{
    delete ui;
}
