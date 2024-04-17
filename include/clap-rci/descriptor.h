#pragma once

#include <clap-rci/global.h>

#include <clap/plugin-features.h>
#include <clap/plugin.h>

#include <span>
#include <string>
#include <string_view>
#include <vector>

CLAP_RCI_BEGIN_NAMESPACE

class Descriptor
{
    Descriptor();

public:
    Descriptor(
        std::string_view id, std::string_view name, std::string_view vendor,
        std::string_view version,
        std::initializer_list<std::string> features = {}
    );
    Descriptor(
        std::string_view id, std::string_view name, std::string_view vendor,
        std::string_view url, std::string_view manualUrl,
        std::string_view supportUrl, std::string_view version,
        std::string_view description,
        std::initializer_list<std::string> features = {}
    );
    ~Descriptor();

    Descriptor(const Descriptor&) = default;
    Descriptor& operator=(const Descriptor&) = default;

    Descriptor(Descriptor&&) noexcept = default;
    Descriptor& operator=(Descriptor&&) noexcept = default;

    explicit Descriptor(const clap_plugin_descriptor& desc);
    explicit(false) operator const clap_plugin_descriptor&() const noexcept;
    explicit(false) operator const clap_plugin_descriptor*() const noexcept;

    [[nodiscard]] std::string_view id() const noexcept;
    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] std::string_view vendor() const noexcept;
    [[nodiscard]] std::string_view url() const noexcept;
    [[nodiscard]] std::string_view manualUrl() const noexcept;
    [[nodiscard]] std::string_view supportUrl() const noexcept;
    [[nodiscard]] std::string_view version() const noexcept;
    [[nodiscard]] std::string_view description() const noexcept;
    [[nodiscard]] std::span<const std::string> features() const noexcept;

    Descriptor& withId(std::string_view id);
    Descriptor& withName(std::string_view name);
    Descriptor& withVendor(std::string_view vendor);
    Descriptor& withUrl(std::string_view url);
    Descriptor& withManualUrl(std::string_view manualUrl);
    Descriptor& withSupportUrl(std::string_view supportUrl);
    Descriptor& withVersion(std::string_view version);
    Descriptor& withDescription(std::string_view description);
    Descriptor& withFeature(std::string_view features);
    Descriptor& withFeatures(std::initializer_list<std::string> features);

private:
    std::string mId;
    std::string mName;
    std::string mVendor;
    std::string mUrl;
    std::string mManualUrl;
    std::string mSupportUrl;
    std::string mVersion;
    std::string mDescription;
    std::vector<std::string> mFeatures;
    std::vector<const char*> mBridge;
    clap_plugin_descriptor mDescriptor = {};

    friend bool
    operator==(const Descriptor& lhs, const clap_plugin_descriptor& rhs);
    friend bool
    operator!=(const Descriptor& lhs, const clap_plugin_descriptor& rhs);
    friend bool operator==(const Descriptor& lhs, const Descriptor& rhs);
    friend bool operator!=(const Descriptor& lhs, const Descriptor& rhs);
};

inline Descriptor::operator const clap_plugin_descriptor&() const noexcept
{
    return mDescriptor;
}

inline Descriptor::operator const clap_plugin_descriptor*() const noexcept
{
    return &mDescriptor;
}

inline std::string_view Descriptor::id() const noexcept
{
    return mDescriptor.id;
}

inline std::string_view Descriptor::name() const noexcept
{
    return mDescriptor.name;
}

inline std::string_view Descriptor::vendor() const noexcept
{
    return mDescriptor.vendor;
}

inline std::string_view Descriptor::url() const noexcept
{
    return mDescriptor.url;
}

inline std::string_view Descriptor::manualUrl() const noexcept
{
    return mDescriptor.manual_url;
}

inline std::string_view Descriptor::supportUrl() const noexcept
{
    return mDescriptor.support_url;
}

inline std::string_view Descriptor::version() const noexcept
{
    return mDescriptor.version;
}

inline std::string_view Descriptor::description() const noexcept
{
    return mDescriptor.description;
}

inline std::span<const std::string> Descriptor::features() const noexcept
{
    return mFeatures;
}

CLAP_RCI_END_NAMESPACE
