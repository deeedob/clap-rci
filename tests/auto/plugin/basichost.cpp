#include "basichost.h"
#include <QDebug>

BasicHost::BasicHost(QAnyStringView pluginPath)
    : mPluginPath(pluginPath.toString())
{
    mHost.host_data = this;
    mHost.clap_version = CLAP_VERSION;
    mHost.name = "BasicHost";
    mHost.version = "0.0.1";
    mHost.vendor = "BasicHost Vendor";
    mHost.url = "https://www.basic.host/";
    mHost.get_extension = clapHostExtensions;
}

BasicHost::~BasicHost() = default;


bool BasicHost::loadPlugin(quint32 index)
{
    mLoader.setFileName(mPluginPath);
    mLoader.setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::DeepBindHint);
    if (!mLoader.load()) {
        qDebug() << "Failed to load plugin: " << mPluginPath << " Error: " << mLoader.errorString();
        return false;
    }

    // :::::::::::::::::: ENTRY ::::::::::::::::::
    mPluginEntry = reinterpret_cast<const struct clap_plugin_entry *>(
        mLoader.resolve("clap_entry")
    );
    if (!mPluginEntry) {
        qDebug() << "Failed to resolve clap_plugin_entry";
        return false;
    }
    // Call clap_plugin_entry::init
    if (!mPluginEntry->init(mPluginPath.toStdString().c_str())) {
        qDebug() << "Failed to init plugin entry";
        return false;
    }
    // :::::::::::::::::: FACTORY ::::::::::::::::::
    mPluginFactory = static_cast<const struct clap_plugin_factory *>(
            mPluginEntry->get_factory(CLAP_PLUGIN_FACTORY_ID)
    );
    if (!mPluginFactory) {
        qDebug() << "Failed to get plugin factory";
        return false;
    }

    // Call clap_plugin_factory::get_plugin_count
    const uint32_t count = mPluginFactory->get_plugin_count(mPluginFactory);
    if (index > count) {
        qDebug() << "Invalid plugin index: " << index << " > " << count;
        return false;
    }

    // Call clap_plugin_descriptor::get_plugin_descriptor
    const auto *descriptor = mPluginFactory->get_plugin_descriptor(mPluginFactory, index);
    if (!descriptor) {
        qDebug() << "Failed to get plugin descriptor";
        return false;
    }

    if (!clap_version_is_compatible(descriptor->clap_version)) {
        qDebug() << "Plugin version not compatible";
        qDebug() << "Host: " << CLAP_VERSION.major << "." << CLAP_VERSION.minor
                 << "." << CLAP_VERSION.revision;
        qDebug() << "Plugin: " << descriptor->clap_version.major << "."
                 << descriptor->clap_version.minor << "."
                 << descriptor->clap_version.revision;
        return false;
    }

    // Call clap_plugin_factory::create_plugin
    mPluginInstance = mPluginFactory->create_plugin(mPluginFactory, &mHost, descriptor->id);
    if (!mPluginInstance) {
        qDebug() << "Failed to create plugin instance with id " << descriptor->id;
        return false;
    }

    // :::::::::::::::::: PLUGIN ::::::::::::::::::
    // Call DSOs clap_plugin::init
    if (!mPluginInstance->init(mPluginInstance)) {
        qDebug() << "Failed to init plugin instance with id " << descriptor->id;
        return false;
    }

    // Init extensions
    initPluginExtensions();
    return true;
}

// ################## PRIVATE ##################

template<typename T>
void BasicHost::initPluginExtension(const T *&extension, const char *extensionId)
{
    if (!extension)
        extension = static_cast<const T*>(mPluginInstance->get_extension(mPluginInstance, extensionId));
}

void BasicHost::initPluginExtensions()
{
    initPluginExtension(mPluginGui, CLAP_EXT_GUI);
    initPluginExtension(mPluginParams, CLAP_EXT_PARAMS);
}

// We use this function to get the host from the clap_host struct
// This is needed because we can't use a static function as a callback
// because we need to access the host instance. This is called a glue routine.
BasicHost *BasicHost::fromHost(const clap_host *host)
{
    if (!host)
        throw std::invalid_argument("host is null");
    auto *h = static_cast<BasicHost *>(host->host_data);
    if (!h)
        throw std::invalid_argument("host_data is null");
    if (!h->mPluginInstance)
        throw std::invalid_argument("plugin instance is null");
    return h;
}

const void *BasicHost::clapHostExtensions(const clap_host *host, const char *extension)
{
    auto *h = fromHost(host);
    if (!strcmp(extension, CLAP_EXT_GUI))
        return &h->mHostGui;
    if (!strcmp(extension, CLAP_EXT_PARAMS))
        return &h->mHostParams;
    return nullptr;
}


void BasicHost::guiResizeHintsChanged(const clap_host *host) {

}

bool BasicHost::guiRequestResize(const clap_host *host, uint32_t width, uint32_t height) {
    return false;
}

bool BasicHost::guiRequestShow(const clap_host *host) {
    return false;
}

bool BasicHost::guiRequestHide(const clap_host *host) {
    return false;
}

void BasicHost::guiClosed(const clap_host *host, bool wasDestroyed) {

}

void BasicHost::paramRescan(const clap_host *host, clap_param_rescan_flags flags) {

}

void BasicHost::paramClear(const clap_host *host, clap_id paramId, clap_param_clear_flags flags) {

}

void BasicHost::paramRequestFlush(const clap_host *host) {

}
