#pragma once

#include <stdexcept>
#include <string>

namespace rprpp {

class Error : public std::runtime_error {
public:
    explicit Error(int err, const std::string& message);
    int getErrorCode() const noexcept { return errorCode; }

private:
    int errorCode;
};

class InternalError : public Error {
public:
    explicit InternalError(const std::string& message);
};

class InvalidParameter : public Error {
public:
    explicit InvalidParameter(const std::string& parameterName, const std::string& message);
};

class InvalidDevice : public Error {
public:
    explicit InvalidDevice(uint32_t deviceId);
};

class ShaderCompilationError : public Error {
public:
    explicit ShaderCompilationError(const std::string& message);
};

class InvalidOperation : public Error {
public:
    explicit InvalidOperation(const std::string& message);
};

}
