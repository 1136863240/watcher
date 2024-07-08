#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    this->setWindowFlag(Qt::FramelessWindowHint);
    this->setWindowFlag(Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->setWindowFlag(Qt::Tool);
    this->setWindowOpacity(0.5);
    this->setWindowIcon(QIcon(":/system.ico"));
    this->setStyleSheet("Widget{background-color: black;}");

    this->tray = new QSystemTrayIcon(QIcon(":/system.ico"), this);
    this->tray->show();

    //下面的构造函数中进行执行
    quitAction=new QAction("退出", this);
    connect(quitAction, &QAction::triggered, this, &QApplication::quit);   //应用程序的退出

    //创建菜单，添加菜单项
    trayIconMenu=new QMenu(this);
    trayIconMenu->addAction(quitAction);
    //给系统托盘添加右键菜单
    this->tray->setContextMenu(trayIconMenu);
    this->tray->setToolTip("Watcher");

    int screenWidth = QApplication::primaryScreen()->geometry().width();
    this->setGeometry(screenWidth - 216, 2, 210, 95);

    this->timer = new QTimer(this);
    this->timer->setInterval(1000);
    this->connect(this->timer, &QTimer::timeout, this, &Widget::refreshUsage);
    this->timer->start();
}

Widget::~Widget()
{
    this->timer->stop();
    delete this->timer;
    delete this->tray;
    delete ui;
}

void Widget::closeEvent(QCloseEvent *e) {
    if (this->tray->isVisible()) {
        this->hide();
        e->ignore();
    }
}

void Widget::paintEvent(QPaintEvent *) {
    int fontHeight = this->fontMetrics().height();
    int y = fontHeight + 2;

    QPainter p(this);
    p.setPen(QColor(255, 255, 255));
    p.setBrush(Qt::white);
    p.drawText(1, y, "CPU Usage");

    int cpuUsage = (int)this->GetCpuUsage();
    QString cpuStr = QString::number(cpuUsage) + "%";
    p.drawText(this->width() - this->fontMetrics().horizontalAdvance(cpuStr) - 2, y, cpuStr);

    y += fontHeight + 2;

    double memUsage = 0;
    double vmemUsage = 0;
    this->printMemoryUsage(&memUsage, &vmemUsage);

    p.drawText(1, y, "RAM Usage");
    QString memStr = QString::number(memUsage) + "%";
    p.drawText(this->width() - this->fontMetrics().horizontalAdvance(memStr) - 2, y, memStr);

    y += fontHeight + 2;

    p.drawText(1, y, "SWAP Usage");
    QString vmemStr = QString::number(vmemUsage) + "%";
    p.drawText(this->width() - this->fontMetrics().horizontalAdvance(vmemStr) - 2, y, vmemStr);

    y += fontHeight + 2;

    unsigned long upload = 0;
    unsigned long download = 0;
    this->printNetworkUsage(&download, &upload);

    p.drawText(1, y, "Upload");
    QString uploadStr = QString::number(upload) + " kb/s";
    p.drawText(this->width() - this->fontMetrics().horizontalAdvance(uploadStr) - 2, y, uploadStr);

    y += fontHeight + 2;

    p.drawText(1, y, "Download");
    QString downloadStr = QString::number(download) + " kb/s";
    p.drawText(this->width() - this->fontMetrics().horizontalAdvance(downloadStr) - 2, y, downloadStr);
}

double Widget::GetCpuUsage() {
    static PDH_HQUERY cpuQuery;
    static PDH_HCOUNTER cpuTotal;
    static bool initialized = false;

    if (!initialized) {
        PdhOpenQuery(NULL, NULL, &cpuQuery);
        PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
        PdhCollectQueryData(cpuQuery);
        initialized = true;
        return 0.0;
    }

    PDH_FMT_COUNTERVALUE counterVal;

    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);

    return counterVal.doubleValue;
}

void Widget::printMemoryUsage(double *physMemUsage, double *virtualMemUsage) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;
    DWORDLONG virtualMemUsed = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;

    *physMemUsage = (double)physMemUsed / totalPhysMem * 100.0;
    *virtualMemUsage = (double)virtualMemUsed / totalVirtualMem * 100.0;
}

void Widget::printNetworkUsage(unsigned long *dwBandIn, unsigned long *dwBandOut)
{
    *dwBandIn = 0;
    *dwBandOut = 0;
    static qint64 totalSent = 0;
    static qint64 totalReceived = 0;
    MIB_IFTABLE *pIfTable;
    MIB_IFROW *pIfRow;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    pIfTable = (MIB_IFTABLE *)malloc(sizeof(MIB_IFTABLE));
    if (pIfTable == nullptr) {
        qCritical() << "Error allocating memory needed to call GetIfTable";
        return;
    }

    if (GetIfTable(pIfTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIfTable);
        pIfTable = (MIB_IFTABLE *)malloc(dwSize);
        if (pIfTable == nullptr) {
            qCritical() << "Error allocating memory needed to call GetIfTable";
            return;
        }
    }

    if ((dwRetVal = GetIfTable(pIfTable, &dwSize, 0)) == NO_ERROR) {
        qint64 totalBytesSent = 0;
        qint64 totalBytesReceived = 0;

        for (int i = 0; i < (int)pIfTable->dwNumEntries; i++) {
            pIfRow = (MIB_IFROW *)&pIfTable->table[i];
            totalBytesSent += pIfRow->dwOutOctets;
            totalBytesReceived += pIfRow->dwInOctets;
        }

        if (totalSent == 0) {
            *dwBandOut = 0;
            totalSent = totalBytesSent;
        } else {
            *dwBandOut = (unsigned long)((totalBytesSent - totalSent) / 1024);
            totalSent = totalBytesSent;
        }
        if (totalReceived == 0) {
            *dwBandIn = 0;
            totalReceived = totalBytesReceived;
        } else {
            *dwBandIn = (unsigned long)((totalBytesReceived - totalReceived) / 1024);
            totalReceived = totalBytesReceived;
        }
    } else {
        qCritical() << "GetIfTable failed with error:" << dwRetVal;
    }

    if (pIfTable != nullptr) {
        free(pIfTable);
        pIfTable = nullptr;
    }
}

void Widget::refreshUsage() {
    this->update();
}
