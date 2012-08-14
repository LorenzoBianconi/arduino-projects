#ifndef QRCONTROLLER_H
#define QRCONTROLLER_H

#include <QtGui/QWidget>
#include "ui_qrcontroller.h"
#include "qbtscanner.h"
#include "qextserialport.h"

class qRController : public QWidget
{
       Q_OBJECT
public:
       qRController(QWidget *parent = 0);
       ~qRController();

       void tx_msg(QByteArray);
private slots:
	void scan();
	void disconnect();
	void set_speed(int speed) { _speed.setNum(speed); }
	void set_motion();
	void stop_motion();
private:
        Ui::qRControllerClass ui;
        QProcess *_rfbind;
        QextSerialPort * _port;
        QString _cmd, _address, _name, _direction, _speed;
};

#endif // QRCONTROLLER_H
