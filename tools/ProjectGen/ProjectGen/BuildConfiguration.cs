using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class BuildConfiguration
    {
        public readonly string Name;

        public readonly Platform PlatformTarget;
        public readonly Architecture Architecture;
        public readonly Configuration Configuration;
        public readonly CompilationStageCollection CompilationStages;
        public readonly PropertyCollection ConfigurationProperties;
        public readonly PropertyCollection ProjectProperties;

        public BuildConfiguration(
            ConfigurationType type, 
            Platform platform, 
            Architecture arch, 
            Configuration config)
        {
            Architecture = arch;
            Configuration = config;
            PlatformTarget = platform;

            Name = Configuration.Name + "|" + Architecture.Name;
            
            // Merge configuration properties
            ConfigurationProperties = new PropertyCollection();
            ConfigurationProperties.Add(platform.ConfigurationProperties);
            ConfigurationProperties.Add(arch.ConfigurationProperties);
            ConfigurationProperties.Add(config.ConfigurationProperties);

            // Merge project properties
            ProjectProperties = new PropertyCollection();
            ProjectProperties.Add(platform.ProjectProperties);
            ProjectProperties.Add(arch.ProjectProperties);
            ProjectProperties.Add(config.ProjectProperties);

            // Merge the compilation settings
            CompilationStages = new CompilationStageCollection();
            CompilationStages.Merge(DefaultConfiguration.Settings);
            CompilationStages.Merge(platform.CompilationStages);
            CompilationStages.Merge(arch.CompilationStages);
            CompilationStages.Merge(config.CompilationStages);

            // If we're building specific types of project then we need 
            // to override some properties
            if (type == ConfigurationType.StaticLibrary)
            {
                AddBuildProperty("ClCompile", "CompileAsWinRT", "false"); // Not supported.
                AddBuildProperty("ClCompile", "PreprocessorDefinitions", "_LIB");
            }
            else if (type == ConfigurationType.DynamicLibrary)
            {
                AddBuildProperty("ClCompile", "PreprocessorDefinitions", "_USRDLL");
            }
        }

        private void AddBuildStageWithProperty(string stageName, string propertyName, string value)
        {
            CompilationStage stage = new CompilationStage(stageName);
            CompilationStages.Add(stage);

            stage.Properties.Add(propertyName, value);
        }

        public void AddBuildProperty(string stageName, string propertyName, string value)
        {
            foreach (CompilationStage stage in CompilationStages)
            {
                if (stage.Name == stageName)
                {
                    stage.Properties.Add(propertyName, value);
                    return;
                }
            }

            AddBuildStageWithProperty(stageName, propertyName, value);
        }

        public void AddProjectProperty(string propertyName, string value)
        {
            ProjectProperties.Add(propertyName, value);
        }

        public void AddConfigurationProperty(string propertyName, string value)
        {
            ConfigurationProperties.Add(propertyName, value);
        }
    }
}
