using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class Platform
    {
        public string Name;
        public bool IsWinRT = false;
        public bool RequiresPFX = false;
        public CompilationStageCollection CompilationStages = new CompilationStageCollection();
        public PropertyCollection GlobalProperties = new PropertyCollection();
        public PropertyCollection ProjectProperties = new PropertyCollection();
        public PropertyCollection ConfigurationProperties = new PropertyCollection();
        public Dictionary<VisualStudioVersion, string> Toolsets = new Dictionary<VisualStudioVersion, string>();

        public static Platform Merge(string finalName, params Platform[] configs)
        {
            Platform result = new Platform();
            result.Name = finalName;

            foreach (Platform config in configs)
            {
                result.CompilationStages.Merge(config.CompilationStages);
                result.GlobalProperties.Add(config.GlobalProperties);
                result.ProjectProperties.Add(config.ProjectProperties);
                result.ConfigurationProperties.Add(config.ConfigurationProperties);
                result.IsWinRT = result.IsWinRT || config.IsWinRT;
                result.RequiresPFX = config.RequiresPFX;
                foreach (var kvp in config.Toolsets)
                {
                    if (!result.Toolsets.ContainsKey(kvp.Key))
                    {
                        result.Toolsets.Add(kvp.Key, kvp.Value);
                    }
                }
            }

            return result;
        } 

        public static readonly Platform WindowsDesktop = new Platform()
        {
            Name = "WindowsDesktop",
            IsWinRT = false,
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "CompileAsWinRT", "False" }
                })
            },
            Toolsets = new Dictionary<VisualStudioVersion, string>()
            {
                { VisualStudioVersion.VS2012, "v110" },
                { VisualStudioVersion.VS2013, "v120" },
                { VisualStudioVersion.VS2015, "v140" }
            }
        };

        public static readonly Platform WindowsStore81 = new Platform()
        {
            Name = "WindowsStore81",
            IsWinRT = true,
            RequiresPFX = true,
            GlobalProperties = new PropertyCollection()
            {
                { "AppContainerApplication", "true" },
                { "ApplicationType", "Windows Store" },
                { "ApplicationTypeRevision", "8.1" }
            },
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "WIN8" },
                    { "CompileAsWinRT", "True" }
                })
            },
            Toolsets = new Dictionary<VisualStudioVersion, string>()
            {
                { VisualStudioVersion.VS2013, "v120" }
            }
        };

        public static readonly Platform Windows10Universal = new Platform()
        {
            Name = "Windows10Universal",
            IsWinRT = true,
            RequiresPFX = true,
            GlobalProperties = new PropertyCollection()
            {
                { "AppContainerApplication", "true" },
                { "ApplicationType", "Windows Store" },
                { "EnableDotNetNativeCompatibleProfile", "true" },
                { "WindowsTargetPlatformVersion", "10.0.14393.0" },
                { "WindowsTargetPlatformMinVersion", "10.0.10586.0" },
                { "ApplicationTypeRevision", "10" }
            },
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "WIN10" },
                    { "CompileAsWinRT", "True" }
                })
            },
            Toolsets = new Dictionary<VisualStudioVersion, string>()
            {
                { VisualStudioVersion.VS2015, "v140" }
            }
        };

        public static readonly Platform XboxOneERA = new Platform()
        {
            Name = "XboxOneERA",
            IsWinRT = true,
            RequiresPFX = false,
            GlobalProperties = new PropertyCollection()
            {
                { "ApplicationEnvironment", "Title" },
                { "Keyword", "Win32Proj" },
                { "TargetRuntime", "Native" },
            },
            ConfigurationProperties = new PropertyCollection()
            {
                { "EmbedManifest", "false" },
                { "GenerateManifest", "false" },
                { "CharacterSet", "Unicode" }
            },
            ProjectProperties = new PropertyCollection()
            {
                { "ReferencePath", new BuildProperty(@"$(Console_SdkLibPath);$(Console_SdkWindowsMetadataPath)", ';') },
                { "LibraryPath", new BuildProperty(@"$(Console_SdkLibPath)", ';') },
                { "LibraryWPath", new BuildProperty(@"$(Console_SdkLibPath);$(Console_SdkWindowsMetadataPath)", ';') },
                { "IncludePath", new BuildProperty(@"$(Console_SdkIncludeRoot)", ';') },
                { "ExecutablePath", new BuildProperty(@"$(Console_SdkRoot)bin;$(VCInstallDir)bin\x86_amd64;$(VCInstallDir)bin;$(WindowsSDK_ExecutablePath_x86);$(VSInstallDir)Common7\Tools\bin;$(VSInstallDir)Common7\tools;$(VSInstallDir)Common7\ide;$(ProgramFiles)\HTML Help Workshop;$(MSBuildToolsPath32);$(FxCopDir);$(PATH);", ';') },
                { "LayoutDir", @"$(SolutionDir)Build\Layout\" }
            },
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "_XBOX_ONE;__WRL_NO_DEFAULT_LIB__" },
                    { "AdditionalUsingDirectories", "$(Console_SdkPackagesRoot);$(Console_SdkWindowsMetadataPath)" },
                    { "FunctionalLevelLinking", "True" },
                    { "IntrinsicFunctions", "True" },
                    { "CompileAsWinRT", "True" }
                }),
                new Link(new PropertyCollection()
                {
                    { "AdditionalDependencies", new BuildProperty( "combase.lib;kernelx.lib;uuid.lib", ';', PropertyInheritance.Overwrite ) }
                })
            },
            Toolsets = new Dictionary<VisualStudioVersion, string>()
            {
                { VisualStudioVersion.VS2012, "v110" },
                { VisualStudioVersion.VS2015, "v140" }
            }
        };
    }
}
