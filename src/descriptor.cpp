#include <clap-rci/descriptor.h>
#include <cstdlib>
#include <cstring>

CLAP_RCI_BEGIN_NAMESPACE

Descriptor::Descriptor() = default;

Descriptor::Descriptor(
    std::string_view id, std::string_view name, std::string_view vendor,
    std::string_view version, std::initializer_list<std::string> features
)
    : mId(id)
    , mName(name)
    , mVendor(vendor)
    , mVersion(version)
    , mFeatures(features)
{
    for (const auto& s : mFeatures)
        mBridge.push_back(s.data());
    mBridge.push_back(nullptr);
    mDescriptor = {
        .clap_version = CLAP_VERSION,
        .id = mId.data(),
        .name = mName.data(),
        .vendor = mVendor.data(),
        .url = nullptr,
        .manual_url = nullptr,
        .support_url = nullptr,
        .version = mVersion.data(),
        .description = nullptr,
        .features = mBridge.data()
    };
}

Descriptor::Descriptor(
    std::string_view id, std::string_view name, std::string_view vendor,
    std::string_view url, std::string_view manualUrl,
    std::string_view supportUrl, std::string_view version,
    std::string_view description, std::initializer_list<std::string> features
)
    : mId(id)
    , mName(name)
    , mVendor(vendor)
    , mUrl(url)
    , mManualUrl(manualUrl)
    , mSupportUrl(supportUrl)
    , mVersion(version)
    , mDescription(description)
    , mFeatures(features)
{
    for (const auto& s : mFeatures)
        mBridge.push_back(s.data());
    mBridge.push_back(nullptr);
    mDescriptor = {
        .clap_version = CLAP_VERSION,
        .id = mId.data(),
        .name = mName.data(),
        .vendor = mVendor.data(),
        .url = mUrl.data(),
        .manual_url = mManualUrl.data(),
        .support_url = mSupportUrl.data(),
        .version = mVersion.data(),
        .description = mDescription.data(),
        .features = mBridge.data()
    };
}

Descriptor::Descriptor(const clap_plugin_descriptor& desc)
{
    mDescriptor.clap_version = desc.clap_version;
    if (desc.features) {
        size_t count = 0;
        while (desc.features[count] != nullptr)
            ++count;
        mFeatures.reserve(count);
        mBridge.reserve(count + 1);
        for (size_t i = 0; i < count; ++i)
            mFeatures.push_back(desc.features[i]);
        for (const auto& s : mFeatures)
            mBridge.push_back(s.data());
        mBridge.push_back(nullptr);
        mDescriptor.features = mBridge.data();
    }

    mDescriptor.clap_version = desc.clap_version;
    if (desc.id) {
        mId = desc.id;
        mDescriptor.id = mId.data();
    }
    if (desc.name) {
        mName = desc.name;
        mDescriptor.name = mName.data();
    }
    if (desc.vendor) {
        mVendor = desc.vendor;
        mDescriptor.vendor = mVendor.data();
    }
    if (desc.url) {
        mUrl = desc.url;
        mDescriptor.url = mUrl.data();
    }
    if (desc.manual_url) {
        mManualUrl = desc.manual_url;
        mDescriptor.manual_url = mManualUrl.data();
    }
    if (desc.support_url) {
        mSupportUrl = desc.support_url;
        mDescriptor.support_url = mSupportUrl.data();
    }
    if (desc.version) {
        mVersion = desc.version;
        mDescriptor.version = mVersion.data();
    }
    if (desc.description) {
        mDescription = desc.description;
        mDescriptor.description = mDescription.data();
    }
}

Descriptor::~Descriptor() = default;

Descriptor& Descriptor::withId(std::string_view id)
{
    mId = id;
    mDescriptor.id = mId.data();
    return *this;
}

Descriptor& Descriptor::withName(std::string_view name)
{
    mName = name;
    mDescriptor.id = mName.data();
    return *this;
}

Descriptor& Descriptor::withVendor(std::string_view vendor)
{
    mVendor = vendor;
    mDescriptor.vendor = mVendor.data();
    return *this;
}

Descriptor& Descriptor::withUrl(std::string_view url)
{
    mUrl = url;
    mDescriptor.url = mUrl.data();
    return *this;
}

Descriptor& Descriptor::withManualUrl(std::string_view manualUrl)
{
    mManualUrl = manualUrl;
    mDescriptor.manual_url = mManualUrl.data();
    return *this;
}

Descriptor& Descriptor::withSupportUrl(std::string_view supportUrl)
{
    mSupportUrl = supportUrl;
    mDescriptor.support_url = mSupportUrl.data();
    return *this;
}

Descriptor& Descriptor::withVersion(std::string_view version)
{
    mVersion = version;
    mDescriptor.version = mVersion.data();
    return *this;
}

Descriptor& Descriptor::withDescription(std::string_view description)
{
    mDescription = description;
    mDescriptor.description = mDescription.data();
    return *this;
}

Descriptor& Descriptor::withFeature(std::string_view features)
{
    mFeatures.push_back(std::string(features));
    mBridge.clear();
    for (const auto& s : mFeatures)
        mBridge.push_back(s.data());
    mBridge.push_back(nullptr);
    mDescriptor.features = mBridge.data();
    return *this;
}

Descriptor& Descriptor::withFeatures(std::initializer_list<std::string> features
)
{
    mFeatures.assign(features);
    mBridge.clear();
    for (const auto& s : mFeatures)
        mBridge.push_back(s.data());
    mBridge.push_back(nullptr);
    mDescriptor.features = mBridge.data();
    return *this;
}

bool operator==(const Descriptor& lhs, const Descriptor& rhs)
{
    return lhs.mDescriptor.clap_version.major
               == rhs.mDescriptor.clap_version.major
           && lhs.mDescriptor.clap_version.minor
                  == rhs.mDescriptor.clap_version.minor
           && lhs.mDescriptor.clap_version.revision
                  == rhs.mDescriptor.clap_version.revision
           && lhs.mId == rhs.mId && lhs.mName == rhs.mName
           && lhs.mVendor == rhs.mVendor && lhs.mUrl == rhs.mUrl
           && lhs.mManualUrl == rhs.mManualUrl
           && lhs.mSupportUrl == rhs.mSupportUrl && lhs.mVersion == rhs.mVersion
           && lhs.mDescription == rhs.mDescription
           && lhs.mFeatures == rhs.mFeatures;
}

bool operator!=(const Descriptor& lhs, const Descriptor& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const Descriptor& lhs, const clap_plugin_descriptor& rhs)
{
    for (size_t i = 0; i < lhs.mFeatures.size(); ++i) {
        if (strcmp(lhs.mFeatures[i].data(), rhs.features[i]) != 0)
            return false;
    }
    if (rhs.features[lhs.mFeatures.size()] != nullptr)
        return false;

    bool ok = true;
    if (lhs.mDescriptor.id && rhs.id)
        ok = strcmp(lhs.mDescriptor.id, rhs.id) == 0;
    if (lhs.mDescriptor.name && rhs.name && ok)
        ok = strcmp(lhs.mDescriptor.name, rhs.name) == 0;
    if (lhs.mDescriptor.vendor && rhs.vendor && ok)
        ok = strcmp(lhs.mDescriptor.vendor, rhs.vendor) == 0;
    if (lhs.mDescriptor.url && rhs.url && ok)
        ok = strcmp(lhs.mDescriptor.url, rhs.url) == 0;
    if (lhs.mDescriptor.manual_url && rhs.manual_url && ok)
        ok = strcmp(lhs.mDescriptor.manual_url, rhs.manual_url) == 0;
    if (lhs.mDescriptor.support_url && rhs.support_url && ok)
        ok = strcmp(lhs.mDescriptor.support_url, rhs.support_url) == 0;
    if (lhs.mDescriptor.version && rhs.version && ok)
        ok = strcmp(lhs.mDescriptor.version, rhs.version) == 0;
    if (lhs.mDescriptor.description && rhs.description && ok)
        ok = strcmp(lhs.mDescriptor.description, rhs.description) == 0;

    if (!ok)
        return false;

    return lhs.mDescriptor.clap_version.major == rhs.clap_version.major
           && lhs.mDescriptor.clap_version.minor == rhs.clap_version.minor
           && lhs.mDescriptor.clap_version.revision
                  == rhs.clap_version.revision;
}

bool operator!=(const Descriptor& lhs, const clap_plugin_descriptor& rhs)
{
    return !(lhs == rhs);
}

CLAP_RCI_END_NAMESPACE
