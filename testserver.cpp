#include "testserver.h"
#include <KWayland/Server/display.h>
#include <KWayland/Server/compositor_interface.h>
#include <KWayland/Server/datadevicemanager_interface.h>
#include <KWayland/Server/idle_interface.h>
#include <KWayland/Server/fakeinput_interface.h>
#include <KWayland/Server/seat_interface.h>
#include <KWayland/Server/shell_interface.h>
#include <KWayland/Server/surface_interface.h>
#include <KWayland/Server/subcompositor_interface.h>
#include <KWayland/Server/buffer_interface.h>
#include <KWayland/Server/xdgshell_interface.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QProcess>
#include <QTimer>
#include <QBuffer>
#include <QDebug>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
// system
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace KWayland::Server;

TestServer::TestServer(QObject *parent)
    : QObject(parent)
    , m_repaintTimer(new QTimer(this))
    , m_timeSinceStart(new QElapsedTimer)
    , m_cursorPos(QPointF(0, 0))
{
    m_compositor = new TestCompositor(this);
    connect(m_compositor, &TestCompositor::mouseClick,
            this, &TestServer::onMouseClick );
}

TestServer::~TestServer() = default;

void TestServer::init()
{
    Q_ASSERT(!m_display);
    m_display = new Display(this);
    m_display->start(Display::StartMode::ConnectClientsOnly);
    m_display->createShm();
    m_display->createCompositor()->create();

    m_shell = m_display->createXdgShell(XdgShellInterfaceVersion::UnstableV6);
    connect(m_shell, &XdgShellInterface::surfaceCreated, this,
        [this] (XdgShellSurfaceInterface *surface) {
            m_shellSurfaces << surface;

            m_seat->setFocusedPointerSurface(surface->surface(), QPointF(1, 1));

            surface->configure(XdgShellSurfaceInterface::State::Fullscreen, QSize(400,400));

            connect(surface, &QObject::destroyed, this,
                [this, surface] {
                    m_shellSurfaces.removeOne(surface);
                }
            );

            connect(surface, &XdgShellSurfaceInterface::titleChanged,
                    this, &TestServer::onTitleChanged);
        }
    );

    m_shell->create();
    m_seat = m_display->createSeat(m_display);
    m_seat->setHasKeyboard(true);
    m_seat->setHasPointer(true);
    m_seat->setHasTouch(true);
    m_seat->create();
    m_display->createDataDeviceManager(m_display)->create();
    m_display->createIdle(m_display)->create();
    m_display->createSubCompositor(m_display)->create();

    auto output = m_display->createOutput(m_display);
    const QSize size(400, 400);
    output->setGlobalPosition(QPoint(0, 0));
    output->addMode(size);
    output->create();

    m_repaintTimer->setInterval(1000 / 60);
    connect(m_repaintTimer, &QTimer::timeout, this, &TestServer::repaint);
    m_repaintTimer->start();
    m_timeSinceStart->start();
}

void TestServer::startTestApp(const QString &app, const QStringList &arguments)
{
    int sx[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sx) < 0) {
        QCoreApplication::instance()->exit(1);
        return;
    }
    m_display->createClient(sx[0]);
    int socket = dup(sx[1]);
    if (socket == -1) {
        QCoreApplication::instance()->exit(1);
        return;
    }
    QProcess *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::ForwardedChannels);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("wayland"));
    environment.insert(QStringLiteral("WAYLAND_SOCKET"), QString::fromUtf8(QByteArray::number(socket)));
    p->setProcessEnvironment(environment);
    auto finishedSignal = static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished);
    connect(p, finishedSignal, QCoreApplication::instance(), &QCoreApplication::exit);
    connect(p, &QProcess::errorOccurred, this,
        [] {
            QCoreApplication::instance()->exit(1);
        }
    );
    p->start(app, arguments);
}

void TestServer::onMouseClick(QPointF a_mousePos)
{
    if(m_seat->pointerPos() != a_mousePos)
    {
        m_seat->setTimestamp(m_timeSinceStart->elapsed());
        m_seat->setPointerPos(a_mousePos);
    }


    m_seat->setTimestamp(m_timeSinceStart->elapsed());
    m_seat->pointerButtonPressed(Qt::LeftButton);
    m_seat->setTimestamp(m_timeSinceStart->elapsed());
    m_seat->pointerButtonReleased(Qt::LeftButton);
}

void TestServer::onTitleChanged(const QString &title)
{
    qDebug() << title;
}

void TestServer::repaint()
{
    for (auto it = m_shellSurfaces.constBegin(), end = m_shellSurfaces.constEnd(); it != end; ++it) {
        (*it)->surface()->frameRendered(m_timeSinceStart->elapsed());

        SurfaceInterface * surface = (*it)->surface();
        BufferInterface* buffInterface = surface->buffer();

        m_compositor->refreshScreen(buffInterface);
    }
}


TestCompositor::TestCompositor(QObject *parent)
{
    setWindowTitle("TestCompositor");
    show();
}

TestCompositor::~TestCompositor()
{

}

void TestCompositor::refreshScreen(BufferInterface *a_buffer)
{
    if(a_buffer)
    {
        m_image = a_buffer->data().copy();
        update();
    }
}

void TestCompositor::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawImage(0,0, m_image);
}

void TestCompositor::mousePressEvent(QMouseEvent *event)
{
    emit mouseClick(QPointF(event->x(), event->y()));
}
