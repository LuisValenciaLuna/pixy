#include <stdexcept>
#include <QDebug>
#include <QMessageBox>
#include "mainwindow.h"
#include "videowidget.h"
#include "console.h"
#include "interpreter.h"
#include "chirpmon.h"
#include "dfu.h"
#include "ui_mainwindow.h"

extern ChirpProc c_grabFrame;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    setWindowTitle("PixyMon");

    m_interpreter = 0;
    m_pixyConnected = false;
    m_pixyDFUConnected = false;

    // start looking for devices
    m_connect = new ConnectEvent(this);

    m_console = new ConsoleWidget(this);
    m_video = new VideoWidget(this);

    m_ui->imageLayout->addWidget(m_video);
    m_ui->imageLayout->addWidget(m_console);

    m_ui->toolBar->addAction(m_ui->actionPlay_Pause);

    m_ui->actionConfigure->setIcon(QIcon(":/icons/icons/config.png"));
    m_ui->toolBar->addAction(m_ui->actionConfigure);

    updateButtons();
}

MainWindow::~MainWindow()
{
    if (m_interpreter)
        delete m_interpreter;
    if (m_connect)
        delete m_connect;
    delete m_ui;
}

void MainWindow::updateButtons()
{
    if (m_interpreter->programRunning())
    {
        m_ui->actionPlay_Pause->setIcon(QIcon(":/icons/icons/pause.png"));
    }
    else
    {
        m_ui->actionPlay_Pause->setIcon(QIcon(":/icons/icons/play.png"));
    }
}

void MainWindow::close()
{
    QCoreApplication::exit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    close();
}

void MainWindow::handleRunState(bool state)
{
    updateButtons();
}

void MainWindow::connectPixyDFU(bool state)
{
    // if we're disconnected, start the connect thread
    if (state)
    {
        try
        {
            Dfu *dfu;
            dfu = new Dfu();
            dfu->download("video.bin.hdr");
            m_pixyDFUConnected = true;
            delete dfu;
            m_connect = new ConnectEvent(this);
        }
        catch (std::runtime_error &exception)
        {
            m_pixyDFUConnected = false;
            QMessageBox::critical(this, "Error", exception.what());
        }
    }
    else
    {
        m_connect = new ConnectEvent(this);
        m_pixyDFUConnected = false;
    }

    updateButtons();
}

void MainWindow::connectPixy(bool state)
{
    if (state)
    {
        try
        {
            if (m_pixyDFUConnected) // we're in programming mode
            {
                // m_flash = new Flash(this);
            }
            else
            {
                m_interpreter = new Interpreter(m_console, m_video);
                // start with a program (normally would be read from a config file instead of hard-coded)
                m_interpreter->beginProgram();
                m_interpreter->call("cam_getFrame 33, 0, 0, 320, 200");
                m_interpreter->endProgram();
                m_interpreter->runProgram();

                connect(m_interpreter, SIGNAL(runState(bool)), this, SLOT(handleRunState(bool)));
            }
            m_pixyConnected = true;
        }
        catch (std::runtime_error &exception)
        {
            QMessageBox::critical(this, "Error", exception.what());
            m_pixyConnected = false;
        }

    }
    else
    {
        delete m_interpreter;
        m_interpreter = NULL;
        // if we're disconnected, start the connect thread
        m_connect = new ConnectEvent(this);
        m_pixyConnected = false;
    }

    updateButtons();
}

void MainWindow::handleConnected(ConnectEvent::Device device, bool state)
{
    // kill connect thread
    delete m_connect;
    m_connect = NULL;

    if (device==ConnectEvent::PIXY)
        connectPixy(state);
    else if (device==ConnectEvent::PIXY_DFU)
        connectPixyDFU(state);
}

void MainWindow::on_actionPlay_Pause_triggered()
{
    if (m_interpreter->programRunning())
        m_interpreter->stopProgram();
    else
        m_interpreter->resumeProgram();
    updateButtons();
}

void MainWindow::on_actionProgram_triggered()
{
    //program();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}
