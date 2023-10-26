#ifndef BASICHOST_H
#define BASICHOST_H

#include <clap/clap.h>
#include <QLibrary>
#include <QWindow>

class BasicHost
{
public:
    explicit BasicHost(QAnyStringView pluginPath);
    ~BasicHost();

    bool loadPlugin(quint32 index = 0);

private:
    template <typename T>
    void initPluginExtension(const T *&extension, const char *extensionId);
    void initPluginExtensions();
    static BasicHost *fromHost(const clap_host *host);
    static const void *clapHostExtensions(const clap_host *host, const char *extension);
    // Host GUI support
    static void guiResizeHintsChanged(const clap_host *host);
    static bool guiRequestResize(const clap_host *host, uint32_t width, uint32_t height);
    static bool guiRequestShow(const clap_host *host);
    static bool guiRequestHide(const clap_host *host);
    static void guiClosed(const clap_host *host, bool wasDestroyed);
    // Host Param support
    static void paramRescan(const clap_host *host, clap_param_rescan_flags flags);
    static void paramClear(const clap_host *host, clap_id paramId, clap_param_clear_flags flags);
    static void paramRequestFlush(const clap_host *host);
private:
    clap_host mHost;

    QWindow mHostedWindow;
    QLibrary mLoader;
    QString mPluginPath;

    static constexpr clap_host_gui mHostGui {
        guiResizeHintsChanged,
        guiRequestResize,
        guiRequestShow,
        guiRequestHide,
        guiClosed
    };

    static constexpr clap_host_params mHostParams {
        paramRescan,
        paramClear,
        paramRequestFlush
    };

    // Plugin
    const clap_plugin_entry* mPluginEntry = nullptr;
    const clap_plugin_factory *mPluginFactory = nullptr;
    const clap_plugin *mPluginInstance = nullptr;
    // Extension
    const clap_plugin_gui *mPluginGui = nullptr;
    const clap_plugin_params *mPluginParams = nullptr;
};

#endif // BASICHOST_H
