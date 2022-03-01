using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace ProjectGen
{
    class Program
    {

        static string _codeDir;

        static readonly Configuration[] DefaultConfigurations = new Configuration[] { 
                Quake3Configs.Debug,
                Quake3Configs.Profile,
                Quake3Configs.Release
        };

        static bool HasGit() { return Directory.Exists(Path.Combine(_codeDir, ".git")); }
        static bool HasWin8() { return Directory.Exists(Path.Combine(_codeDir, "code\\win8")); }
        static bool HasUWP() { return Directory.Exists(Path.Combine(_codeDir, "code\\uwp")); }
        static bool HasHoloLens() { return Directory.Exists(Path.Combine(_codeDir, "code\\hololens")); }
        static bool HasXboxOne() { return Directory.Exists(Path.Combine(_codeDir, "code\\xbo")); }

        static void GenerateSln(
            Quake3Target target,
            VisualStudioVersion vsVersion,
            IEnumerable<Architecture> architectures,
            IEnumerable<Configuration> configurations)
        {
            Quake3Configuration config = new Quake3Configuration();
            config.BindToSCC = !HasGit();
            config.VisualStudioVersion = vsVersion;
            config.Architectures = architectures;
            config.Configurations = configurations;
            config.Target = target;

            switch (target)
            {
                case Quake3Target.Desktop:
                    config.Platform = Quake3Platforms.Desktop;
                    break;
                case Quake3Target.WinStore:
                    config.Platform = Quake3Platforms.WinStore81;
                    config.WantSeparateGameDLLs = false;
                    config.DirectX = DirectXLevel.DX11Only;
                    break;
                case Quake3Target.Win10UWP:
                    config.Platform = Quake3Platforms.Win10UWP;
                    config.WantSeparateGameDLLs = false;
                    break;
                case Quake3Target.HoloLens:
                    config.Platform = Quake3Platforms.HoloLens;
                    config.WantSeparateGameDLLs = false;
                    config.DirectX = DirectXLevel.DX11Only;
                    break;
                case Quake3Target.XboxOne:
                    config.Platform = Quake3Platforms.XboxOneERA;
                    config.WantXInputLib = false;
                    config.WantAccountLib = false;
                    config.DirectX = DirectXLevel.DX12Only;
                    break;
            }

            Quake3Sln slnGen = new Quake3Sln(config);
            SolutionExporter exporter = new SolutionExporter();
            exporter.Export(slnGen.Generate(), true);
        }

        static void GoDesktop()
        {
            var archs = new Architecture[] {
                    Quake3Architectures.x86,
                    Quake3Architectures.x64 };
            var archs15 = new Architecture[] {
                    Quake3Architectures.x64 };
            
            GenerateSln(Quake3Target.Desktop, VisualStudioVersion.VS2013, archs, DefaultConfigurations);
            GenerateSln(Quake3Target.Desktop, VisualStudioVersion.VS2015, archs15, DefaultConfigurations);
        }
        
        static void GoWin81()
        {
            if (HasWin8())
            {
                var archs = new Architecture[] {
                    Quake3Architectures.ARM,
                    Quake3Architectures.x86,
                    Quake3Architectures.x64 };

                GenerateSln(Quake3Target.WinStore, VisualStudioVersion.VS2013, archs, DefaultConfigurations);
            }
        }

        static void GoXBO()
        {
            if (HasXboxOne())
            {
                var archs = new Architecture[] {
                    Quake3Architectures.XboxOne };
                
                GenerateSln(Quake3Target.XboxOne, VisualStudioVersion.VS2012, archs, DefaultConfigurations);
                GenerateSln(Quake3Target.XboxOne, VisualStudioVersion.VS2015, archs, DefaultConfigurations);
            }
        }

        static void GoUWP()
        {
            if (HasUWP())
            {
                var archs = new Architecture[] {
                    Quake3Architectures.ARM,
                    Quake3Architectures.x86,
                    Quake3Architectures.x64 };

                GenerateSln(Quake3Target.Win10UWP, VisualStudioVersion.VS2015, archs, DefaultConfigurations);
            }
        }

        static void GoHoloLens()
        {
            if (HasHoloLens())
            {
                var archs = new Architecture[] {
                    Quake3Architectures.HoloLens };

                GenerateSln(Quake3Target.HoloLens, VisualStudioVersion.VS2015, archs, DefaultConfigurations);
            }
        }

        static void CreateIfPossible(string dir)
        {
            try { Directory.CreateDirectory(dir); } catch (Exception) { }
        }
        static void Main(string[] args)
        {
            if (!Directory.Exists("code"))
                throw new FileNotFoundException("You need to be in the root Q3 directory.");

            _codeDir = Environment.CurrentDirectory;

            CreateIfPossible("projects");
            Environment.CurrentDirectory = "projects";

            GoDesktop();
            GoXBO();
            //GoWin81();
            GoUWP();
            GoHoloLens();
        }
    }
}
