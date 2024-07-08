#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QMenu>
#include <QTimer>
#include <QPainter>
#include <QDebug>
#include <pdh.h>
#include <pdhmsg.h>
#include <windows.h>
#include <iphlpapi.h>


QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void closeEvent(QCloseEvent *e);
    void paintEvent(QPaintEvent *);

private:
    Ui::Widget *ui;

    double GetCpuUsage();
    void printMemoryUsage(double *physMemUsage, double *virtualMemUsage);
    void printNetworkUsage(unsigned long *dwBandIn, unsigned long *dwBandOut);
    void refreshUsage();
    QSystemTrayIcon *tray;
    QMenu *trayIconMenu;
    QAction *quitAction;
    QTimer *timer;
    double cpuUsage;
};
#endif // WIDGET_H
