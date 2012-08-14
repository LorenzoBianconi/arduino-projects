#ifndef QBTSCANNER_H
#define QBTSCANNER_H

#include <QtGui/QDialog>
#include <QProcess>
#include "ui_qbtscanner.h"

class qBtScanner : public QDialog
{
	Q_OBJECT
public:
        Ui::qBtScannerClass ui;

        qBtScanner(QWidget *parent = 0);
        ~qBtScanner();

        void parse_info(const QString input);
public slots:
	void start_scan();
	void scan_finished();
private:
	QProcess *_btscanner;
	QString _cmd;
};

#endif // QBTSCANNER_H
