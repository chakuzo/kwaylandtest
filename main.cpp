#include "testserver.h"

#include <QCoreApplication>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    auto arguments = QApplication::arguments();

    arguments.removeFirst();
    if (arguments.size() < 1) {
        return 1;
    }

    TestServer *server = new TestServer(&app);
    server->init();
    server->startTestApp(arguments.takeFirst(), arguments);

    return app.exec();
}
