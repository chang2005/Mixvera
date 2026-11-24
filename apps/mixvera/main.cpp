#include "mixvera/core/application.hpp"
#include "mixvera/engine/media_engine.hpp"
#include "mixvera/ui/main_window.hpp"

#include <QApplication>
#include <iostream>

int main(int argc, char** argv)
{
    QApplication qt(argc, argv);
    QCoreApplication::setApplicationName("Mixvera");
    QCoreApplication::setApplicationVersion("0.1.0");

    mixvera::engine::StubMediaEngine engine;
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize media engine\n";
        return 1;
    }

    auto& app = mixvera::core::Application::instance();
    if (app.initialize() != 0) {
        return 1;
    }
    app.run();

    mixvera::ui::MainWindow window;
    window.show();

    const int result = QApplication::exec();
    app.shutdown();
    engine.shutdown();
    return result;
}
