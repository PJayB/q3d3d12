using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class DefaultConfiguration
    {
        public static readonly CompilationStageCollection Settings = new CompilationStageCollection()
        {
            new ClCompile(new PropertyCollection()
            {
                { "Optimization", "Full" },
                { "RuntimeLibrary", "MultiThreadedDLL" },
                { "TreatWarningsAsErrors", "true" },
                { "SuppressStartupBanner", "true" },
                { "PreprocessorDefinitions", "WIN32;_WINDOWS" }
            }),
            new FxCompile(new PropertyCollection()
            {
                { "SuppressStartupBanner", "true" },
                { "ShaderModel", "5.0" }
            }),
            new Link(new PropertyCollection()
            {
                { "SuppressStartupBanner", "true" },
                { "Subsystem", "Windows" }
            }),
            new Lib(new PropertyCollection()
            {
                { "SuppressStartupBanner", "true" }
            }),
            new ResourceCompile(new PropertyCollection()
            {
                { "SuppressStartupBanner", "true" },
                { "Culture", "0x0409" }
            })
        };
    }
}
