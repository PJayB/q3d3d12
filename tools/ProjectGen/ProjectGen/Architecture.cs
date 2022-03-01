using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class Architecture
    {
        public string Name;
        public CompilationStageCollection CompilationStages = new CompilationStageCollection();
        public PropertyCollection ConfigurationProperties = new PropertyCollection();
        public PropertyCollection ProjectProperties = new PropertyCollection();

        public static Architecture Merge(string finalName, params Architecture[] configs)
        {
            Architecture result = new Architecture();
            result.Name = finalName;

            foreach (Architecture config in configs)
            {
                result.CompilationStages.Merge(config.CompilationStages);
                result.ProjectProperties.Add(config.ProjectProperties);
                result.ConfigurationProperties.Add(config.ConfigurationProperties);
            }

            return result;
        } 

        public static readonly Architecture x86 = new Architecture()
        {
            Name = "Win32",
            CompilationStages = new CompilationStageCollection()
            {
                new FxCompile(new PropertyCollection()
                {
                    { "ShaderModel", "5.0" }
                }),
                new Link(new PropertyCollection()
                {
                    { "Subsystem", "Windows" },
                    { "TargetMachine", "MachineX86" }
                })
            }
        };

        public static readonly Architecture x64 = new Architecture()
        {
            Name = "x64",
            CompilationStages = new CompilationStageCollection()
            {
                new FxCompile(new PropertyCollection()
                {
                    { "ShaderModel", "5.0" }
                }),
                new Link(new PropertyCollection()
                {
                    { "Subsystem", "Windows" },
                    { "TargetMachine", "MachineX64" }
                })
            }
        };

        public static readonly Architecture ARM = new Architecture()
        {
            Name = "ARM",
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "_ARM_" }
                }),
                new FxCompile(new PropertyCollection()
                {
                    { "ShaderModel", "4.0_level_9_3" },
                    { "PreprocessorDefinitions", "_ARM_" }
                }),
                new Link(new PropertyCollection()
                {
                    { "Subsystem", "Windows" },
                    { "TargetMachine", "MachineARM" }
                })
            }
        };

        public static readonly Architecture XboxOne = new Architecture()
        {
            Name = "Durango",
            CompilationStages = new CompilationStageCollection()
            {
                new FxCompile(new PropertyCollection()
                {
                    { "ShaderModel", "5.0" }
                }),
                new Link(new PropertyCollection()
                {
                    { "Subsystem", "Windows" },
                    { "TargetMachine", "MachineX64" },
                    { "EnableCOMDATFolding", "true" },
                    { "OptimizeReferences", "true" },
                    { "GenerateWindowsMetadata", "false" }
                })
            }
        };
    }
}
