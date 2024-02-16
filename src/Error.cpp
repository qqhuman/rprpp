#include "Error.h"
#include "capi/rprpp.h"

namespace rprpp {

Error::Error(int err, const std::string& message)
: std::runtime_error(message),
  errorCode(err)
{
}

InternalError::InternalError(const std::string& message)
    : Error(RPRPP_ERROR_INTERNAL_ERROR, "Internal error. " + message)
{
}

InvalidParameter::InvalidParameter(const std::string& parameterName, const std::string& message)
    : Error(RPRPP_ERROR_INVALID_PARAMETER, parameterName + "is invalid parameter. " + message)
{
}

InvalidDevice::InvalidDevice(uint32_t deviceId)
    : Error(RPRPP_ERROR_INVALID_DEVICE, "DeviceId " + std::to_string(deviceId) + " doesn't exist.")
{
}

ShaderCompilationError::ShaderCompilationError(const std::string& message)
    : Error(RPRPP_ERROR_SHADER_COMPILATION, "Shader compilation error. " + message)
{
}

InvalidOperation::InvalidOperation(const std::string& message)
    : Error(RPRPP_ERROR_INVALID_OPERATION, "Invalid operation. " + message)
{
}

} // namespace
