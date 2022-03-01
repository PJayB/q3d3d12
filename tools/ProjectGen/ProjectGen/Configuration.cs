using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    // You should clone and extend these if you want new build flavors.
    class Configuration
    {
        public string Name;

        public CompilationStageCollection CompilationStages = new CompilationStageCollection();
        public PropertyCollection ProjectProperties = new PropertyCollection();
        public PropertyCollection ConfigurationProperties = new PropertyCollection();

        public Configuration()
        {
        }

        public static Configuration Merge(string finalName, params Configuration[] configs)
        {
            Configuration result = new Configuration();
            result.Name = finalName;

            foreach (Configuration config in configs)
            {
                result.CompilationStages.Merge(config.CompilationStages);
                result.ProjectProperties.Add(config.ProjectProperties);
                result.ConfigurationProperties.Add(config.ConfigurationProperties);
            }

            return result;
        } 

        public static readonly Configuration Debug = new Configuration()
        {
            Name = "Debug",
            ProjectProperties = new PropertyCollection()
            {
                { "IncrementalLinking", "true" }
            },
            ConfigurationProperties = new PropertyCollection()
            {
                { "UseDebugLibraries", "true" }
            },
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "Optimization", "Disabled" },
                    { "RuntimeLibrary", "MultiThreadedDebugDLL" },
                    { "TreatWarningsAsErrors", "false" },
                    { "PreprocessorDefinitions", "DEBUG;_DEBUG" }
                }),
                new FxCompile(new PropertyCollection()
                {
                    { "DisableOptimizations", "true" },
                    { "EnableDebuggingInformation", "true" }
                }),
                new Link(new PropertyCollection()
                {
                    { "GenerateDebugInformation", "true" },
                    { "IncrementalLinking", "false" }
                })
            }
        };
        
        public static readonly Configuration Profile = new Configuration()
        {
            Name = "Profile",
            ProjectProperties = new PropertyCollection()
            {
                { "IncrementalLinking", "true" }
            },
            ConfigurationProperties = new PropertyCollection()
            {
                { "UseDebugLibraries", "true" }
            },
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "Optimization", "MaxSpeed" },
                    { "RuntimeLibrary", "MultiThreadedDLL" },
                    { "TreatWarningsAsErrors", "false" },
                    { "PreprocessorDefinitions", "NDEBUG;PROFILE" },
                    { "DebugInformationFormat", "ProgramDatabase" },
                    { "BasicRuntimeChecks", "Default" }
                }),
                new FxCompile(new PropertyCollection()
                {
                    { "EnableDebuggingInformation", "true" }
                }),
                new Link(new PropertyCollection()
                {
                    { "GenerateDebugInformation", "true" },
                    { "IncrementalLinking", "false" },
                    { "UseDebugLibraries", "false" }
                })
            }
        };
        
        public static readonly Configuration Release = new Configuration()
        {
            Name = "Release",
            ProjectProperties = new PropertyCollection()
            {
                { "IncrementalLinking", "true" }
            },
            ConfigurationProperties = new PropertyCollection()
            {
                { "UseDebugLibraries", "true" }
            },
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "Optimization", "MaxSpeed" },
                    { "RuntimeLibrary", "MultiThreadedDLL" },
                    { "TreatWarningsAsErrors", "true" },
                    { "PreprocessorDefinitions", "NDEBUG" },
                    { "DebugInformationFormat", "ProgramDatabase" },
                    { "BasicRuntimeChecks", "Default" }
                }),
                new Link(new PropertyCollection()
                {
                    { "GenerateDebugInformation", "true" },
                    { "IncrementalLinking", "false" },
                    { "UseDebugLibraries", "false" }
                })
            }
        };
    }
}
