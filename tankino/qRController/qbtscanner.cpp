#include "qbtscanner.h"
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QTreeWidgetItem>

qBtScanner::qBtScanner(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	_cmd = "bluez-test-discovery";
	_btscanner = new QProcess(this);

	QTreeWidgetItem *header = new QTreeWidgetItem();
	header->setText(0, tr("Name"));
	header->setText(1, tr("Alias"));
	header->setText(2, tr("Address"));
	header->setText(3, tr("RSSI"));
	header->setText(4, tr("Paired"));
	header->setText(5, tr("Trusted"));
	ui.scanTreeWidget->setHeaderItem(header);

	connect(ui.connectButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(ui.quitButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(ui.scanButton, SIGNAL(clicked()), this, SLOT(start_scan()));
	connect(ui.stopButton, SIGNAL(clicked()), _btscanner, SLOT(kill()));
	connect(_btscanner, SIGNAL(finished(int)), this, SLOT(scan_finished()));
}

qBtScanner::~qBtScanner()
{
}

void qBtScanner::parse_info(const QString input)
{
	QFile scanfile(input);
	if (scanfile.open(QIODevice::WriteOnly)) {
		QTextStream ts(&scanfile);
		ts << _btscanner->readAll();
		scanfile.close();
		QSettings scanIni(input, QSettings::IniFormat);
		QStringList groups = scanIni.childGroups();
		for (int i = 0; i < groups.size(); i++) {
			scanIni.beginGroup(groups[i]);
			QStringList keys = scanIni.childKeys();
			QTreeWidgetItem *item = new QTreeWidgetItem();
			for (int j = 0; j < keys.size(); j++) {
				QString value = scanIni.value(keys[j]).toString();
				qDebug() << keys[j] << "=" << value;
				if (keys[j] == "Name")
					item->setText(0, value);
				else if (keys[j] == "Alias")
					item->setText(1, value);
				else if (keys[j] == "Address")
					item->setText(2, value);
				else if (keys[j] == "RSSI")
					item->setText(3, value);
				else if (keys[j] == "Paired")
					item->setText(4, value);
				else if (keys[j] == "Trusted")
					item->setText(5, value);
			}
			scanIni.endGroup();
			ui.scanTreeWidget->addTopLevelItem(item);
		}
		scanfile.remove();
	}
}

void qBtScanner::start_scan()
{
	ui.scanTreeWidget->clear();
	_btscanner->start(_cmd);
	ui.scanButton->setEnabled(false);
	ui.connectButton->setEnabled(false);
	qDebug() << "Start Scan";
}

void qBtScanner::scan_finished()
{
	parse_info("scan.ini");
	ui.scanButton->setEnabled(true);
	ui.connectButton->setEnabled(true);
	qDebug() << "Scan Finished";
}
