#include "App.h"
#include <framelift/Log.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QSurfaceFormat>
#include <QtQuick/QQuickWindow>

int main(int argc, char* argv[])
{
    // Force the single-threaded "basic" scene-graph render loop so all GL work — Qt's
    // adopted context and our raw-GL video node — stays on the GUI thread (the GL video
    // renderer / FFmpeg RenderFrame assume single-threaded main-thread GL).
    qputenv("QSG_RENDER_LOOP", "basic");

    // Force the OpenGL RHI so Qt creates a GL context our GlGraphicsBackend can adopt
    // (Qt6 otherwise defaults to D3D11 on Windows). Must be set before any QQuickWindow.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication qtApp(argc, argv);
    QGuiApplication::setOrganizationName("FrameLift");
    QGuiApplication::setApplicationName("FrameLift");
    // We drive shutdown explicitly (window close → App quit flow), so don't let Qt quit
    // out from under the host when the last window closes.
    QGuiApplication::setQuitOnLastWindowClosed(false);

    Log::Init();
    App app("FrameLift", 1280, 720, argc, argv);
    return app.Run();
}
