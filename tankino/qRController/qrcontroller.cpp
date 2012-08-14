#include "qrcontroller.h"
#include <QDebug>
#include <QTreeWidgetItem>

qRController::qRController(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.statusText->setText("Not connected");
	ui.speedDial->setRange(0, 9);
	ui.speedSpinBox->setRange(0, 9);

	_port = new QextSerialPort(QLatin1String("/dev/rfcomm0"), QextSerialPort::Polling);
	_port->setBaudRate(BAUD38400);
	_port->setFlowControl(FLOW_OFF);
	_port->setParity(PAR_NONE);
	_port->setDataBits(DATA_8);
	_port->setStopBits(STOP_1);
	_port->setTimeout(500);

	_rfbind = new QProcess(this);
	_cmd = "kdesudo";

	_speed = "0";
	_direction = "F";

	_address = _name = QString();

	connect(ui.scanButton, SIGNAL(clicked()), this, SLOT(scan()));
	connect(ui.quitButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui.disconnectButton, SIGNAL(clicked()), this, SLOT(disconnect()));
	connect(ui.speedDial, SIGNAL(sliderMoved(int)), ui.speedSpinBox, SLOT(setValue(int)));
	connect(ui.speedDial, SIGNAL(sliderMoved(int)), this, SLOT(set_speed(int)));
	connect(ui.speedSpinBox, SIGNAL(valueChanged(int)), ui.speedDial, SLOT(setValue(int)));
	connect(ui.speedSpinBox, SIGNAL(valueChanged(int)), this, SLOT(set_speed(int)));
	connect(ui.forwardButton, SIGNAL(pressed()), this, SLOT(set_motion()));
	connect(ui.backwordButton, SIGNAL(pressed()), this, SLOT(set_motion()));
	connect(ui.leftButton, SIGNAL(pressed()), this, SLOT(set_motion()));
	connect(ui.rightButton, SIGNAL(pressed()), this, SLOT(set_motion()));
	connect(ui.forwardButton, SIGNAL(released()), this, SLOT(stop_motion()));
	connect(ui.backwordButton, SIGNAL(released()), this, SLOT(stop_motion()));
	connect(ui.leftButton, SIGNAL(released()), this, SLOT(stop_motion()));
	connect(ui.rightButton, SIGNAL(released()), this, SLOT(stop_motion()));
}

qRController::~qRController()
{

}

void qRController::scan()
{
	QStringList args;
	QByteArray msg;
	qBtScanner bScanner(this);

	if (_name == QString()) {
		qBtScanner bScanner(this);
		bScanner.start_scan();
		ui.statusText->setText("Scanning");
		if (bScanner.exec() == QDialog::Accepted) {
			QTreeWidgetItem *dev = bScanner.ui.scanTreeWidget->currentItem();
			if (dev) {
				_name = dev->text(0);
				_address = dev->text(2);
				args << "rfcomm" << "bind" << "0" << _address;
				_rfbind->start(_cmd, args);
				if (!_rfbind->waitForStarted() || !_rfbind->waitForFinished())
					return;
			}
		}
	}

	if (_name != QString()) {
		if (!_port->isOpen())
			_port->open(QIODevice::WriteOnly | QIODevice::Unbuffered);
		if (_port->isOpen()) {
			ui.scanButton->setEnabled(false);
			msg.append(_direction).append("0").append("\n");
			tx_msg(msg);
			ui.statusText->setText(QString("Connected to %1 [%2]").arg(_name).arg(_address));
			qDebug() << "port status [" << _port->isOpen() << "]";

		}
	}
}

void qRController::disconnect()
{
	QStringList args;

	args << "rfcomm" << "release" << "rfcomm0";
	_rfbind->start(_cmd, args);
	if (!_rfbind->waitForStarted() || !_rfbind->waitForFinished())
		return;
	if (_port->isOpen())
		_port->close();
	_address = QString();
	_name = QString();
	ui.scanButton->setEnabled(true);
	ui.statusText->setText("Not connected");
	qDebug() << "disconnecting: port status [" << _port->isOpen() << "]";
}

void qRController::set_motion()
{
	QObject *caller = sender();
	QString name = caller->objectName();
	QByteArray msg;

	if (name == ui.forwardButton->objectName())
		_direction = "F";
	else if (name == ui.backwordButton->objectName())
		_direction = "B";
	else if (name == ui.leftButton->objectName())
		_direction = "L";
	else if (name == ui.rightButton->objectName())
		_direction = "R";
	msg.append(_direction).append(_speed).append("\n");
	tx_msg(msg);
	qDebug() << _direction << _speed;
}

void qRController::stop_motion()
{
	QByteArray msg;

	_direction = "F";
	msg.append(_direction).append("0").append("\n");
	tx_msg(msg);
	qDebug() << _direction << _speed;
}

void qRController::tx_msg(QByteArray msg)
{
	int sent = _port->write(msg);
	qDebug() << "sent " << sent;
}
