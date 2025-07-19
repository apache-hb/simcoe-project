#pragma once

#include "core/error/error.hpp"

namespace sm::docker {
    class DockerError : public sm::errors::Error<DockerError> {
        using Super = sm::errors::Error<DockerError>;
    public:
        using Exception = class DockerException;

        template<typename... A> requires (sizeof...(A) > 0)
        DockerError(fmt::format_string<A...> fmt, A&&... args)
            : Super(fmt::vformat(fmt, fmt::make_format_args(args...)))
        { }

        DockerError(std::string message)
            : Super(std::move(message))
        { }
    };

    class DockerException : public sm::errors::Exception<DockerError> {
        using Super = sm::errors::Exception<DockerError>;
    public:
        using Super::Super;

        template<typename... A> requires (sizeof...(A) > 0)
        DockerException(fmt::format_string<A...> fmt, A&&... args)
            : Super(DockerError(fmt::vformat(fmt, fmt::make_format_args(args...))))
        { }

        DockerException(std::string message)
            : Super(DockerError(std::move(message)))
        { }
    };
}
