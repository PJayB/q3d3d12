using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    enum ConfigurationType
    {
        StaticLibrary,
        DynamicLibrary,
        Application,
        NMake
    }

    enum HlslShaderType
    {
        Vertex,
        Geometry,
        Domain,
        Hull,
        Pixel,
        Compute
    }
}
