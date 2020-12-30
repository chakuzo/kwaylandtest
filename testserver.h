#ifndef TESTSERVER_H
#define TESTSERVER_H

#include <QHash>
#include <QObject>
#include <QPointF>
#include <QVector>
#include <QWidget>

class QElapsedTimer;
class QTimer;
class QWidget;
class QPainter;

namespace KWayland
{
namespace Server
{
class Display;
class SeatInterface;
class XdgShellInterface;
class XdgShellSurfaceInterface;
class BufferInterface;
}
}

class TestCompositor : public QWidget
{
    Q_OBJECT
public:
    explicit TestCompositor(QObject *parent);
    virtual ~TestCompositor();

    void refreshScreen(KWayland::Server::BufferInterface* a_buffer);

signals:
    void mouseClick(QPointF a_pos);
protected:
    void paintEvent(QPaintEvent *event);
private:
    void mousePressEvent(QMouseEvent *event);

    QImage m_image;
    QPainter* m_screen;
};

class TestServer : public QObject
{
    Q_OBJECT
public:
    explicit TestServer(QObject *parent);
    virtual ~TestServer();

    void init();
    void startTestApp(const QString &app, const QStringList &arguments);
public slots:
    void onMouseClick(QPointF a_mousePos);
private slots:
    void onTitleChanged(const QString& title);
private:
    void repaint();

    KWayland::Server::Display *m_display = nullptr;
    KWayland::Server::XdgShellInterface *m_shell = nullptr;
    KWayland::Server::SeatInterface *m_seat = nullptr;
    QVector<KWayland::Server::XdgShellSurfaceInterface*> m_shellSurfaces;
    QTimer *m_repaintTimer;
    QScopedPointer<QElapsedTimer> m_timeSinceStart;
    QPointF m_cursorPos;
    QHash<qint32, qint32> m_touchIdMapper;
    TestCompositor* m_compositor;
};

#endif
