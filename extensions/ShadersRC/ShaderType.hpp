#pragma once

namespace vkbase::ShadersRC
{
    enum class ShaderType {
        Vertex,
        Fragment,
        Compute,
        Geometry,
        TessControl,
        TessEvaluation
    };
}