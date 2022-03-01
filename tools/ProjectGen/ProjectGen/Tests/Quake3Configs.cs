using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    static class Quake3Configs
    {
        static readonly Configuration BaseConfiguration = new Configuration()
        {
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "_CRT_SECURE_NO_WARNINGS;PRIORITIZE_LOOSE_FILES" },
                    { "PrecompiledHeader", "NotUsing" },
                    { "DisableSpecificWarnings", "4201;4204;4467" }
                }),
                new FxCompile(new PropertyCollection()
                {
                    { "ObjectFileOutput", "$(OutDir)baseq3\\hlsl\\%(Filename).cso" }
                })
            }
        };

        static readonly Configuration ReleaseConfiguration = new Configuration()
        {
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "TreatWarningAsError", "true" }
                })
            }
        };

        public static readonly Configuration Debug = Configuration.Merge("Debug", Configuration.Debug, BaseConfiguration);
        public static readonly Configuration Profile = Configuration.Merge("Profile", Configuration.Profile, BaseConfiguration);
        public static readonly Configuration Release = Configuration.Merge("Release", Configuration.Release, BaseConfiguration, ReleaseConfiguration);
    }

    static class Quake3Platforms
    {
        static readonly Platform DesktopPlatformSettings = new Platform()
        {
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "WarningLevel", "Level4" }
                }),
                new Link(new PropertyCollection()
                {
                    { "AdditionalDependencies", "xaudio2.lib;xinput.lib;winmm.lib;wsock32.lib" }
                })
            }
        };

        static readonly Platform WinRTPlatformSettings = new Platform()
        {
            IsWinRT = true,
            RequiresPFX = true,
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "Q_WINRT_PLATFORM;NO_GETENV" }
                }),
                new Link(new PropertyCollection()
                {
                    { "AdditionalDependencies", "xaudio2.lib;xinput.lib" }
                })
            }
        };

        static readonly Platform XboxOnePlatformSettings = new Platform()
        {
            IsWinRT = true,
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "Q_WINRT_PLATFORM;Q_WINRT_FORCE_DIPS_EQUAL_PIXELS;Q_WINRT_FORCE_WIN32_THREAD_API;NO_GETENV" },
                    { "DisableSpecificWarnings", "4204" }
                }),
                new Link(new PropertyCollection()
                {
                    { "AdditionalDependencies", "xg_x.lib;ws2_32.lib;xaudio2.lib;vccorlib.lib;pixEvt.lib;combase.lib;kernelx.lib;uuid.lib" }
                })
            }
        };

        static readonly Platform MixedD3DPlatformSettings = new Platform()
        {
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "Q_HAS_D3D11;Q_HAS_D3D12" }
                }),
                new Link(new PropertyCollection()
                {
                    { "AdditionalDependencies", "dxgi.lib;d3d11.lib;d3d12.lib"}
                })
            }
        };

        static readonly Platform D3D12XPlatformSettings = new Platform()
        {
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "Q_HAS_D3D12;Q_HAS_D3D12_X" }
                }),
                new Link(new PropertyCollection()
                {
                    { "AdditionalDependencies", "d3d12_x.lib"}
                })
            }
        };

        static readonly Platform HoloLensPlatformSettings = new Platform()
        {
            GlobalProperties = new PropertyCollection()
            {
                { "Keyword", "HolographicApp" }
            },
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "Q_HAS_D3D11" }
                }),
                new Link(new PropertyCollection()
                {
                    { "AdditionalDependencies", "d3d11.lib"},
                    { "IgnoreSpecificDefaultLibraries", "mincore.lib;kernel32.lib;ole32.lib;" }
                })
            }
        };

        public static readonly Platform Desktop = Platform.Merge("Desktop", Platform.WindowsDesktop, DesktopPlatformSettings, MixedD3DPlatformSettings);
        public static readonly Platform WinStore81 = Platform.Merge("Win81", Platform.WindowsStore81, WinRTPlatformSettings, MixedD3DPlatformSettings);
        public static readonly Platform Win10UWP = Platform.Merge("UWP", Platform.Windows10Universal, WinRTPlatformSettings, MixedD3DPlatformSettings);
        public static readonly Platform XboxOneERA = Platform.Merge("XboxOne", Platform.XboxOneERA, XboxOnePlatformSettings, D3D12XPlatformSettings);
        public static readonly Platform HoloLens = Platform.Merge("HoloLens", Platform.Windows10Universal, WinRTPlatformSettings, HoloLensPlatformSettings);
    }

    static class Quake3Architectures
    {
        static readonly Architecture FixedDisplaySettings = new Architecture()
        {
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "Q_WINRT_FIXED_DISPLAY" }
                })
            }
        };

        static readonly Architecture SkipDlightProjection = new Architecture()
        {
            CompilationStages = new CompilationStageCollection()
            {
                new ClCompile(new PropertyCollection()
                {
                    { "PreprocessorDefinitions", "Q_SKIP_DLIGHT_PROJECTION" }
                })
            }
        };

        public static readonly Architecture ARM = Architecture.Merge(Architecture.ARM.Name, Architecture.ARM, FixedDisplaySettings);
        public static readonly Architecture x86 = Architecture.Merge(Architecture.x86.Name, Architecture.x86, SkipDlightProjection);
        public static readonly Architecture x64 = Architecture.Merge(Architecture.x64.Name, Architecture.x64, SkipDlightProjection);
        public static readonly Architecture XboxOne = Architecture.Merge(Architecture.XboxOne.Name, Architecture.XboxOne, SkipDlightProjection, FixedDisplaySettings);
        public static readonly Architecture HoloLens = Architecture.Merge(Architecture.x86.Name, Architecture.x86, FixedDisplaySettings);
    }

    enum Quake3Target
    {
        Desktop,
        WinStore,
        Win10UWP,
        XboxOne,
        HoloLens
    }

    enum DirectXLevel
    {
        DX11Only,
        DX12Only,
        Both
    }

    class Quake3Configuration
    {
        public Quake3Target Target;
        public bool BindToSCC = false;
        public bool WantAccountLib = true;
        public bool WantXInputLib = true;
        public bool WantSeparateGameDLLs = true;
        public DirectXLevel DirectX = DirectXLevel.Both;
        public VisualStudioVersion VisualStudioVersion;
        public Platform Platform;
        public IEnumerable<Architecture> Architectures;
        public IEnumerable<Configuration> Configurations;
    }
}
