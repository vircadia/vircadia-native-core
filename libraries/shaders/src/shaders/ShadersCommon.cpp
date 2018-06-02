#include <shared/FileUtils.h>
#include <mutex>

namespace shader {

std::string loadShaderSource(uint32_t shaderId) {
    static std::once_flag once;
    std::call_once(once, [] {
        Q_INIT_RESOURCE(shaders);
    });
    auto shaderPath = std::string(":/shaders/") + std::to_string(shaderId);
    auto shaderData = FileUtils::readFile(shaderPath.c_str());
    return shaderData.toStdString();
}

}
