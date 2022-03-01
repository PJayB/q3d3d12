using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Xml;

namespace ProjectGen
{
    class Quake3Sln
    {
        public const string RelativeBasePath = "..\\..\\";
        public const string RelativeCodePath = RelativeBasePath + "code\\";

        public const string QabiOutputsPath = @"$(SolutionDir)..\build\qabi\$(SolutionName)\$(Configuration)\.";
        public const string QabiGenScript = @"$(SolutionDir)..\tools\genqabis.bat";

        enum Game
        {
            baseq3,
            missionpack
        }
        
        // Temporaries, outputs
        private string _outputPath;
        private Solution _sln;
        private Quake3Configuration _config;

        public Quake3Sln(
            Quake3Configuration config)
        {
            // Validate config
            if (config.VisualStudioVersion == null) throw new NullReferenceException("VisualStudioVersion must not be null");
            if (config.Platform == null) throw new NullReferenceException("Platform must not be null");
            if (config.Architectures == null) throw new NullReferenceException("Architectures must not be null");
            if (config.Configurations == null) throw new NullReferenceException("Configurations must not be null");

            // Validate architectures
            foreach (Architecture a in config.Architectures)
            {
                if (a != Quake3Architectures.ARM &&
                    a != Quake3Architectures.x64 &&
                    a != Quake3Architectures.x86 &&
                    a != Quake3Architectures.XboxOne &&
                    a != Quake3Architectures.HoloLens)
                {
                    throw new InvalidDataException("Use one of the predetermined architecture templates.");
                }
            }

            string slnFile = VSName(config.VisualStudioVersion.Name, config.Platform.Name, "Q3");

            // Create the temporary project directory
            _config = config;
            _outputPath = slnFile + '\\';
            _sln = new Solution(config.VisualStudioVersion, slnFile, slnFile + ".sln");
        }

        static string VSName(string vsVer, string platformName, string name)
        {
            return name + '_' + vsVer + '_' + platformName;
        }

        string SafeDefineName(string name)
        {
            StringBuilder nu = new StringBuilder();
            bool first = true;
            foreach (char c in name.ToUpper())
            {
                if (char.IsLetter(c))
                    nu.Append(c);
                if (char.IsNumber(c) && !first)
                    nu.Append(c);
                if (c == '_')
                    nu.Append(c);
                first = false;
            }
            return nu.ToString();
        }

        List<string> FindSources(string path, string patternStr, bool recurse)
        {
            string[] patterns = patternStr.Split(new char[] { ';' });

            List<string> output = new List<string>();
            foreach (string pattern in patterns)
            {
                var files = Directory.EnumerateFiles(
                    path,
                    pattern,
                    recurse ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly);

                output.AddRange(files);
            }

            return output;
        }

        private void SetupNewProject(Project proj, bool splitLibraries)
        {
            proj.BuildConfigurations = Project.CreateBuildConfigurationTable(
                proj.ConfigurationType,
                _config.Platform,
                _config.Architectures,
                _config.Configurations);

            string slnPath = @"$(SolutionName)\$(Platform)\$(Configuration)\";
            string intPath = @"$(SolutionName)\$(ProjectName)\$(Platform)\$(Configuration)\";
            if (splitLibraries && proj.ConfigurationType == ConfigurationType.StaticLibrary)
            {
                proj.OutDir = @"$(SolutionDir)..\build\lib\" + slnPath;
            }
            else
            {
                proj.OutDir = @"$(SolutionDir)..\build\bin\" + slnPath;
            }
            proj.IntermediateDir = @"$(SolutionDir)..\build\int\" + intPath;

            //
            // Set compilation settings
            //
            proj.AddBuildProperty(
                "*",
                "ClCompile",
                "PreprocessorDefinitions",
                SafeDefineName(proj.ProjectName));

            proj.AddBuildProperty(
                "*",
                "ClCompile",
                "AdditionalIncludeDirectories",
                QabiOutputsPath);

            // Add to source control
            //<SccProjectName>SAK</SccProjectName>
            //<SccAuxPath>SAK</SccAuxPath>
            //<SccLocalPath>SAK</SccLocalPath>
            //<SccProvider>SAK</SccProvider>
            if (_config.BindToSCC)
            {
                proj.GlobalProperties.Add("SccProjectName", "SAK");
                proj.GlobalProperties.Add("SccAuxPath", "SAK");
                proj.GlobalProperties.Add("SccLocalPath", "SAK");
                proj.GlobalProperties.Add("SccProvider", "SAK");
            }

        }

        private Guid GetExistingProjectGuid(string filePath)
        {
            using (StreamReader xml = new StreamReader(filePath))
            {
                string content = xml.ReadToEnd();

                int projectGuidTagLocation = content.IndexOf("<ProjectGuid>");
                if (projectGuidTagLocation == -1)
                    throw new KeyNotFoundException();

                int projectGuidLocation = projectGuidTagLocation + "<ProjectGuid>".Length;

                int projectGuidEndLocation = content.IndexOf("</ProjectGuid>", projectGuidLocation);
                if (projectGuidEndLocation == -1)
                    throw new KeyNotFoundException();

                return Guid.Parse(
                    content.Substring(projectGuidLocation, projectGuidEndLocation - projectGuidLocation));
            }
        }

        private Guid NewProjectGuid(Project proj, string name)
        {
            return proj.NewGuid(name);
        }

        private Project NewProject(string projectName, ConfigurationType type, bool splitLibraries = true)
        {
            string projectFN = projectName + ".vcxproj";
            string projectPath = _outputPath + projectFN;
            string guidPath = projectFN + ".guids";

            Project proj = new Project(
                _config.VisualStudioVersion,
                _config.Platform,
                type,
                _sln.NewGuid(projectPath),
                projectName, 
                projectPath);

            try { proj.LoadGuids(guidPath); } catch (Exception) { }
            
            SetupNewProject(proj, splitLibraries);

            return proj;
        }

        private FilterItem NewFilter(Project project, FilterItem parent, string name)
        {
            FilterItem filter = new FilterItem(name, NewProjectGuid(project, name));
            parent.AddItem(filter);
            return filter;
        }

        private FilterItem NewFilter(Project proj, string name)
        {
            FilterItem filter = GetFilter(proj, name);
            if (filter == null)
            {
                filter = new FilterItem(name, NewProjectGuid(proj, name));
                proj.RootLayoutItems.Add(filter);
            }
            return filter;
        }

        private FilterItem GetFilter(Project proj, string name)
        {
            foreach (FilterItem i in proj.RootLayoutItems)
            {
                if (i is FilterItem && i.Name == name)
                    return i;
            }
            return null;
        }

        private string StripAfterPath(string path, string src)
        {
            if (src.Contains(path))
            {
                src = src.Substring(src.IndexOf(path) + path.Length);
            }

            src = src.Trim(new char[] { '\\', '/' });
            src = src.Replace("\\\\", "\\");
            src = src.Replace("//", "/");
            src = src.Replace("/", "\\");

            return src;
        }

        private FilterItem AddFilters(Project project, FilterItem parent, string path)
        {
            int i;
            while ((i = path.IndexOf('\\')) != -1)
            {
                string nuFilter = path.Substring(0, i);

                if (nuFilter.Length > 0)
                {
                    parent = NewFilter(project, parent, nuFilter);
                }

                path = path.Substring(i + 1);
            }

            return parent;
        }

        private void AddSourcesOfTypeWithFilters(Project proj, FilterItem parent, string path, string pattern, string typeName)
        {
            List<string> sources = FindSources(RelativeCodePath + path, pattern, true);

            foreach (string src in sources)
            {
                // TODO: this won't work for shaders

                // Create a filter if necessary
                FilterItem srcParent = AddFilters(proj, parent, StripAfterPath(path, src));

                BuildItem item = new BuildItem(typeName, src);

                if (srcParent != null)
                    srcParent.AddItem(item);
                else
                    proj.RootLayoutItems.Add(item);
            }

            if (parent != null)
            {
                HashSet<string> uniqueExtensions = new HashSet<string>();

                string[] patternlist = pattern.Split(new char[] { ';' }, StringSplitOptions.RemoveEmptyEntries);
                foreach (string p in patternlist)
                {
                    string ext = Path.GetExtension(p);
                    if (!uniqueExtensions.Contains(ext))
                        uniqueExtensions.Add(ext);
                }

                foreach (string ext in uniqueExtensions)
                {
                    string trimmed = ext;
                    while (trimmed.StartsWith("."))
                        trimmed = trimmed.Substring(1);
                    if (trimmed.Length > 0)
                        parent.Extensions.Add(trimmed);
                }
            }
        }

        private void AddDefaultFileTypes(Project proj, string path)
        {
            AddSourcesOfTypeWithFilters(proj, NewFilter(proj, "Source Files"), path, "*.c;*.cpp", "ClCompile");
            AddSourcesOfTypeWithFilters(proj, NewFilter(proj, "Include Files"), path, "*.h;*.hpp", "ClInclude");
            AddSourcesOfTypeWithFilters(proj, NewFilter(proj, "Images"), path, "*.png;*.ico;*.tga;*.bmp", "Image");
            AddSourcesOfTypeWithFilters(proj, NewFilter(proj, "Resources"), path, "*.rc", "ResourceFile");
            AddSourcesOfTypeWithFilters(proj, GetFilter(proj, "Resources"), path, "*.appxmanifest", "AppxManifest");
            AddSourcesOfTypeWithFilters(proj, GetFilter(proj, "Resources"), path, "*.pfx", "None");
            AddSourcesOfTypeWithFilters(proj, GetFilter(proj, "Resources"), path, "*.txt;*.def", "Text");
            AddSourcesOfTypeWithFilters(proj, GetFilter(proj, "Resources"), path, "*.xml", "Text");
        }

        private void AddShadersOfTypeWithFilters(Project proj, FilterItem parent, string path, string pattern, HlslShaderType type)
        {
            List<string> sources = FindSources(RelativeCodePath + path, pattern, true);

            foreach (string src in sources)
            {
                // TODO: this won't work for shaders

                // Create a filter if necessary
                FilterItem srcParent = AddFilters(proj, parent, StripAfterPath(path, src));

                BuildItem item = new BuildItem("FxCompile", src);

                item.SetOverride("*", "ShaderType", type.ToString());

                if (srcParent != null)
                    srcParent.AddItem(item);
                else
                    proj.RootLayoutItems.Add(item);
            }
        }

        private Project GenerateShaderProject(SolutionFilter filter, string path, string name = "")
        {
            if (name == "") name = path;

            Project proj = NewProject(name, ConfigurationType.StaticLibrary, false);
            AddShadersOfTypeWithFilters(proj, NewFilter(proj, "Pixel Shaders"), path, "*_ps.hlsl", HlslShaderType.Pixel);
            AddShadersOfTypeWithFilters(proj, NewFilter(proj, "Vertex Shaders"), path, "*_vs.hlsl", HlslShaderType.Vertex);
            AddShadersOfTypeWithFilters(proj, NewFilter(proj, "Geometry Shaders"), path, "*_gs.hlsl", HlslShaderType.Geometry);
            AddShadersOfTypeWithFilters(proj, NewFilter(proj, "Domain Shaders"), path, "*_ds.hlsl", HlslShaderType.Domain);
            AddShadersOfTypeWithFilters(proj, NewFilter(proj, "Hull Shaders"), path, "*_hs.hlsl", HlslShaderType.Hull);
            AddShadersOfTypeWithFilters(proj, NewFilter(proj, "Compute Shaders"), path, "*_cs.hlsl", HlslShaderType.Compute);
            AddSourcesOfTypeWithFilters(proj, NewFilter(proj, "Header Files"), path, "*.h", "Text");

            proj.AddBuildProperty("*", "FxCompile", "AdditionalIncludeDirectories", RelativeCodePath + "hlsl");
            
            // Exclude all 5.0 shaders on ARM
            proj.ExecuteForBuildItems("*5.0\\*.hlsl", (BuildItem item) =>
                {
                    item.ExcludedFromBuild.Set("*|ARM", true);
                });

            // Exclude all 4.0 shaders on everything else
            foreach (BuildConfiguration bc in proj.BuildConfigurations)
            {
                if (bc.Architecture != Architecture.ARM)
                {
                    proj.ExecuteForBuildItems("*4.0_level_9_3\\*.hlsl", (BuildItem item) =>
                        {
                            item.ExcludedFromBuild.Set(bc.Name, true);
                        });
                }
            }

            // Exclude all Xbox One shaders on everything else
            if (_config.Target != Quake3Target.XboxOne)
            {
                foreach (BuildConfiguration bc in proj.BuildConfigurations)
                {
                    proj.ExecuteForBuildItems("*xbo\\*.hlsl", (BuildItem item) =>
                    {
                        item.ExcludedFromBuild.Set(bc.Name, true);
                    });
                }
            }

            proj.LayoutParent = filter;

            return proj;
        }

        private Project GenerateDefaultStaticLib(SolutionFilter filter, string path, string name = "")
        {
            if (name == "")
            {
                name = path;
            }
            
            if (!name.EndsWith("lib"))
            {
                name = name + "lib";
            }

            Project proj = NewProject(name, ConfigurationType.StaticLibrary);
            AddDefaultFileTypes(proj, path);

            proj.LayoutParent = filter;

            proj.AddBuildProperty("*", "ClCompile", "CompileAsWinRT", "False");

            return proj;
        }

        private Project GenerateServerLib(SolutionFilter filter)
        {
            Project proj = GenerateDefaultStaticLib(filter, "server");
            proj.ExecuteForBuildItems("sv_rankings.c", delegate(BuildItem item)
            {
                item.ExcludedFromBuild.Set("*", true);
            });
            return proj;
        }

        private Project GenerateQCommonLib(SolutionFilter filter)
        {
            Project proj = GenerateDefaultStaticLib(filter, "qcommon");

            proj.ExecuteForBuildItems("vm_ppc*.c", delegate(BuildItem item)
            {
                item.ExcludedFromBuild.Set("*", true);
            });

            foreach (BuildConfiguration bc in proj.BuildConfigurations)
            {
                if (bc.Architecture != Architecture.x86)
                {
                    proj.ExecuteForBuildItems("vm_x86*.c", (BuildItem item) =>
                    {
                        item.ExcludedFromBuild.Set(bc.Name, true);
                    });
                }
            }

            return proj;
        }

        private Project NewQabiProject(string projectName, SolutionFilter filter)
        {
            string projectFN = projectName + ".vcxproj";
            string projectPath = _outputPath + projectFN;
            string guidPath = projectFN + ".guids";

            Project proj = new Project(
                _config.VisualStudioVersion,
                Platform.WindowsDesktop,
                ConfigurationType.NMake,
                _sln.NewGuid(projectPath),
                projectName,
                projectPath);

            try { proj.LoadGuids(guidPath); }
            catch (Exception) { }

            proj.BuildConfigurations = Project.CreateBuildConfigurationTable(
                proj.ConfigurationType,
                Platform.WindowsDesktop,
                new Architecture[] { Architecture.x86 },
                _config.Configurations);

            proj.OutDir = QabiOutputsPath;
            proj.IntermediateDir = QabiOutputsPath;

            // Add to source control
            if (_config.BindToSCC)
            {
                proj.GlobalProperties.Add("SccProjectName", "SAK");
                proj.GlobalProperties.Add("SccAuxPath", "SAK");
                proj.GlobalProperties.Add("SccLocalPath", "SAK");
                proj.GlobalProperties.Add("SccProvider", "SAK");
            }

            proj.GlobalProperties.Add("Keyword", "MakeFileProj");

            proj.LayoutParent = filter;
            return proj;
        }

        private Project GenerateQabiGenProject(string vmname, string vmvarname, SolutionFilter filter)
        {
            string projectName = vmname + "_qabi";
            Project proj = NewQabiProject(projectName, filter);

            string inputFile = RelativeCodePath + "qabi\\" + vmname + ".qabi";
            string format = "call " + QabiGenScript + " {0} \"$(ProjectDir)" + inputFile + "\" \"" + QabiOutputsPath + "\"";
            if (vmvarname != null && vmvarname != string.Empty) 
                format += " /vm " + vmvarname;

            string outputs = string.Empty;// string.Format("{0}.jumpdefs.h;{0}.jumptable.h;{0}.public.h;{0}.shared.h;{0}.vmcalls.h", vmname);
            
            if (_sln.VisualStudioVersion != VisualStudioVersion.VS2015)
            {
                proj.AddProjectProperty("*", "NMakeBuildCommandLine", string.Format(format, "build"));
                proj.AddProjectProperty("*", "NMakeCleanCommandLine", string.Format(format, "clean"));
                proj.AddProjectProperty("*", "NMakeReBuildCommandLine", string.Format(format, "rebuild"));
                proj.AddProjectProperty("*", "NMakePreprocessorDefinitions", "");
                proj.AddProjectProperty("*", "NMakeOutput", outputs);
            }
            else
            {
                proj.AddConfigurationProperty("*", "NMakeBuildCommandLine", string.Format(format, "build"));
                proj.AddConfigurationProperty("*", "NMakeCleanCommandLine", string.Format(format, "clean"));
                proj.AddConfigurationProperty("*", "NMakeReBuildCommandLine", string.Format(format, "rebuild"));
                proj.AddConfigurationProperty("*", "NMakePreprocessorDefinitions", "");
                proj.AddConfigurationProperty("*", "NMakeOutput", outputs);
            }

            proj.RootLayoutItems.Add(new BuildItem("Text", inputFile));

            if (_config.Platform == Quake3Platforms.XboxOneERA)
            {
                proj.GlobalProperties.Add("ApplicationEnvironment", "");
            }

            return proj;
        }

        private Project GenerateQabiGenEngineProject(SolutionFilter filter)
        {
            string projectName = "engine_qabi";
            Project proj = NewQabiProject(projectName, filter);

            string[] sharedFiles = 
            {
                "common",
                "client"
            };
            string[] inputFiles =
            {
                "cgame",
                "game",
                "ui" 
            };
            string[] outputFiles =
            {
                "jumpdefs",
                "jumptable",
                "public",
                "shared",
                "syscalls"
            };

            string format = string.Empty;
            string outputs = string.Empty;
            foreach (string i in inputFiles)
            {
                format += "call " + QabiGenScript + " {0} \"$(ProjectDir)" + RelativeCodePath + "qabi\\engine." + i + ".qabi\" \"" + QabiOutputsPath + "\"" + Environment.NewLine;
                foreach (string o in outputFiles)
                {
                    //outputs += string.Format("engine.{0}.{1}.h;", i, o);
                }
            };

            proj.AddConfigurationProperty("*", "NMakeBuildCommandLine", string.Format(format, "build"));
            proj.AddConfigurationProperty("*", "NMakeCleanCommandLine", string.Format(format, "clean"));
            proj.AddConfigurationProperty("*", "NMakeReBuildCommandLine", string.Format(format, "rebuild"));
            proj.AddConfigurationProperty("*", "NMakePreprocessorDefinitions", "");
            proj.AddConfigurationProperty("*", "NMakeOutput", outputs);

            foreach (string i in sharedFiles)
            {
                proj.RootLayoutItems.Add(new BuildItem("Text", string.Format("{0}qabi\\engine.{1}.qabi", RelativeCodePath, i)));
            }
            foreach (string i in inputFiles)
            {
                proj.RootLayoutItems.Add(new BuildItem("Text", string.Format("{0}qabi\\engine.{1}.qabi", RelativeCodePath, i)));
            }

            if (_config.Platform == Quake3Platforms.XboxOneERA)
                proj.GlobalProperties.Add("ApplicationEnvironment", "");

            return proj;
        }

        private Project GenerateQSharedLib(SolutionFilter filter)
        {
            Project proj = NewProject("qshared", ConfigurationType.StaticLibrary);
            proj.LayoutParent = filter;

            // Create a filter for the game files
            string[] sources = new string[]
            {
                RelativeCodePath + "game\\bg_lib.c",
                RelativeCodePath + "game\\bg_misc.c",
                RelativeCodePath + "game\\bg_pmove.c",
                RelativeCodePath + "game\\bg_slidemove.c",
                RelativeCodePath + "game\\q_math.c",
                RelativeCodePath + "game\\q_shared.c",
                RelativeCodePath + "ui\\ui_shared.c"
            };

            string[] headers = new string[]
            {
                RelativeCodePath + "game\\q_shared.h",
                RelativeCodePath + "game\\bg_public.h",
                RelativeCodePath + "ui\\ui_public.h",
                RelativeCodePath + "ui\\ui_shared.h"
            };

            FilterItem srcfilter = NewFilter(proj, "Source Files");
            foreach (string f in sources)
            {
                BuildItem item = new BuildItem("ClCompile", f);
                srcfilter.AddItem(item);
            }
            FilterItem hfilter = NewFilter(proj, "Header Files");
            foreach (string f in headers)
            {
                BuildItem item = new BuildItem("ClInclude", f);
                hfilter.AddItem(item);
            }

            ExcludeFiles(proj, new string[] 
            {
                "bg_lib.c"
            });

            return proj;
        }

        private void ExcludeFiles(Project proj, IEnumerable<string> files, string configs = "*")
        {
            StringBuilder r = new StringBuilder();
            foreach (string file in files)
            {
                r.Append("|");
                r.Append("(");
                r.Append(file.Replace(".", "\\."));
                r.Append(")");
            }

            string filterString = r.ToString();
            if (filterString.Length == 0)
                return;

            Regex regex = new Regex(filterString.Substring(1));

            proj.ExecuteForBuildItems(regex, delegate(BuildItem item)
            {
                item.ExcludedFromBuild.Set(configs, true);
            });
        }

        private Project GenerateJpegLib(SolutionFilter filter)
        {
            Project proj = GenerateDefaultStaticLib(filter, "jpeg-6");

            List<string> noInclude = new List<string>()
            {
                "jcapistd.c",
                "jdmerge.c",
                "jdphuff.c",
                "jfdctfst.c",
                "jfdctint.c",
                "jidctfst.c",
                "jidctint.c",
                "jidctred.c",
                "jload.c",
                "jmemansi.c",
                "jmemdos.c",
                "jmemname.c",
                "jpegtran.c",
                "jquant1.c",
                "jquant2.c"
            };

            ExcludeFiles(proj, noInclude);

            return proj;
        }

        private Project GenerateRendererLib(SolutionFilter filter)
        {
            Project proj = NewProject("renderer", ConfigurationType.StaticLibrary);
            AddSourcesOfTypeWithFilters(proj, null, "renderer", "tr_*.c;tr_*.cpp", "ClCompile");
            AddSourcesOfTypeWithFilters(proj, null, "renderer", "tr_*.h", "ClInclude");

            proj.LayoutParent = filter;

            return proj;
        }

        private Project GenerateQd3dLib(SolutionFilter filter)
        {
            Project proj = GenerateDefaultStaticLib(filter, "qd3d");

            if (_config.Target != Quake3Target.XboxOne)
            {
                proj.ExecuteForBuildItems("qd3d_11x.cpp", delegate(BuildItem item)
                {
                    item.ExcludedFromBuild.Set("*", true);
                });
            }
            return proj;
        }

        private Project GenerateD3D11Lib(SolutionFilter filter)
        {
            Project proj = GenerateDefaultStaticLib(filter, "d3d11");
            if (_config.Target == Quake3Target.XboxOne)
            {
                proj.ExecuteForBuildItems("*win*", delegate(BuildItem item)
                {
                    item.ExcludedFromBuild.Set("*", true);
                });
            }
            else 
            {
                proj.ExecuteForBuildItems("*xbo*", delegate(BuildItem item)
                {
                    item.ExcludedFromBuild.Set("*", true);
                });
            }

            proj.AddBuildProperty("*|ARM", "ClCompile", "PreprocessorDefinitions", "Q_CPU_DLIGHT");

            return proj;
        }

        private Project GenerateD3DLib(SolutionFilter filter, string d3dVersion, bool pch)
        {
            Project proj = GenerateDefaultStaticLib(filter, d3dVersion);
            if (_config.Target == Quake3Target.XboxOne)
            {
                proj.ExecuteForBuildItems("*win*", delegate(BuildItem item)
                {
                    item.ExcludedFromBuild.Set("*", true);
                });
            }
            else
            {
                proj.ExecuteForBuildItems("*xbo*", delegate(BuildItem item)
                {
                    item.ExcludedFromBuild.Set("*", true);
                });
            }

            if (pch)
            {
                proj.AddBuildProperty("*", "ClCompile", "PrecompiledHeader", "Use");
                proj.AddBuildProperty("*", "ClCompile", "PrecompiledHeaderFile", "pch.h");
                proj.AddBuildProperty("*", "ClCompile", "AdditionalIncludeDirectories", RelativeCodePath + d3dVersion);

                proj.ExecuteForBuildItems("pch.cpp", (BuildItem item) =>
                {
                    item.SetOverride("*", "PrecompiledHeader", "Create");
                });
            }

            return proj;
        }

        private Project GenerateSplinesLib(SolutionFilter filter)
        {
            Project proj = GenerateDefaultStaticLib(filter, "splines");
            proj.ExecuteForBuildItems("q_shared.cpp", delegate(BuildItem item)
            {
                item.ExcludedFromBuild.Set("*", true);
            });
            return proj;
        }

        private Project GenerateDefaultVM(
            SolutionFilter filter, 
            Project[] qshared,
            Game game,
            string path, 
            string projectName,
            string targetName)
        {
            Project proj = NewProject(projectName, ConfigurationType.DynamicLibrary);
            AddDefaultFileTypes(proj, path);

            proj.AddBuildProperty("*", "Link", "ModuleDefinitionFile", RelativeCodePath + path + "\\" + path + ".def");
            proj.AddBuildProperty("*", "ClCompile", "CompileAsWinRT", "False");

            if (_config.Platform.IsWinRT)
            {
                string linkStr = string.Empty;
                foreach (Project dep in qshared)
                {
                    if (dep.ConfigurationType == ConfigurationType.StaticLibrary)
                    {
                        linkStr += string.Format("{0}.lib;", dep.ProjectName);
                    }
                    proj.ProjectDependencies.Add(dep);
                }
                proj.AddBuildProperty("*", "Link", "AdditionalDependencies", linkStr);
                proj.AddBuildProperty("*", "Link", "AdditionalLibraryDirectories", @"$(SolutionDir)..\build\lib\$(SolutionName)\$(Platform)\$(Configuration)\");
            }
            else
            {
                foreach (Project dep in qshared)
                {
                    if (dep.ConfigurationType == ConfigurationType.StaticLibrary)
                        proj.ProjectReferences.Add(dep);
                    else 
                        proj.ProjectDependencies.Add(dep);
                }
            }

            proj.LayoutParent = filter;
            proj.OutDir += game.ToString() + "\\";

            if (_config.Target == Quake3Target.XboxOne)
                proj.TargetName = targetName + "xbo";
            else
                proj.TargetName = targetName + "$(PlatformShortName)";

            return proj;
        }

        private Project GenerateCgameVM(SolutionFilter filter, Project[] qshared, Game gameName, string projectName)
        {
            Project proj = GenerateDefaultVM(filter, qshared, gameName, "cgame", projectName, "cgame");

            ExcludeFiles(proj, new string[] 
            {
                "bc_lib.c",
                "cg_particles.c"
            });

            if (gameName == Game.missionpack)
            {
                proj.AddBuildProperty("*", "ClCompile", "PreprocessorDefinitions", "MISSIONPACK");
            }
            else
            {
                ExcludeFiles(proj, new string[] 
                { 
                    "cg_newdraw.c"
                });
            }

            return proj;
        }

        private Project GenerateGameVM(SolutionFilter filter, Project[] qshared, Game gameName, string projectName)
        {
            Project proj = GenerateDefaultVM(filter, qshared, gameName, "game", projectName, "qagame");

            ExcludeFiles(proj, new string[] 
            {
                "bg_lib.c",
                "g_rankings.c"
            });

            if (gameName == Game.missionpack)
            {
                proj.AddBuildProperty("*", "ClCompile", "PreprocessorDefinitions", "MISSIONPACK");
            }

            return proj;
        }

        private Project GenerateUIVM(SolutionFilter filter, Project[] qshared)
        {
            Project proj = GenerateDefaultVM(filter, qshared, Game.missionpack, "ui", "ui", "ui");

            ExcludeFiles(proj, new string[] 
            {
                "ui_util.c"
            });

            proj.AddBuildProperty("*", "ClCompile", "PreprocessorDefinitions", "MISSIONPACK");

            return proj;
        }

        private Project GenerateQ3UIVM(SolutionFilter filter, Project[] qshared)
        {
            Project proj = GenerateDefaultVM(filter, qshared, Game.baseq3, "q3_ui", "q3_ui", "ui");

            // Create a filter for the game files
            string[] vmcommon = new string[]
            {
                RelativeCodePath + "ui\\ui_syscalls.c"
            };

            FilterItem subfilter = NewFilter(proj, "VM Shared");
            foreach (string f in vmcommon)
            {
                BuildItem item = new BuildItem("ClCompile", f);
                subfilter.AddItem(item);
            }

            ExcludeFiles(proj, new string[] 
            {
                "ui_rankings.c",
                "ui_rankstatus.c",
                "ui_specifyleague.c",
                "ui_login.c",
                "ui_signup.c"
            });

            proj.AddBuildProperty("*", "Link", "ModuleDefinitionFile", RelativeCodePath + "q3_ui\\ui.def");
            return proj;
        }

        private Project FindProject(string name)
        {
            foreach (Project proj in _sln.Projects)
            {
                if (proj.ProjectName == name)
                    return proj;
            }
            throw new KeyNotFoundException();
        }

        private Project GenerateDesktopApp()
        {
            Project proj = NewProject("quake3", ConfigurationType.Application);

            // 
            // Add the solution items
            //
            AddDefaultFileTypes(proj, "win32");

            proj.AddBuildProperty("*", "PostBuildEvent", "Command",
                RelativeCodePath + "deploydx12sdkbits.bat \"$(OutDir)\"");

            return proj;
        }

        public void CopyFolder(string source, string destination)
        {
            string xcopyPath = Environment.GetEnvironmentVariable("WINDIR") + @"\System32\xcopy.exe";
            ProcessStartInfo info = new ProcessStartInfo(xcopyPath);
            info.UseShellExecute = false;
            info.RedirectStandardOutput = true;
            info.Arguments = string.Format("\"{0}\" \"{1}\" /S /I /Y /D", source, destination);

            Process process = Process.Start(info);
            process.WaitForExit();
            string result = process.StandardOutput.ReadToEnd();

            if (process.ExitCode != 0)
            {
                // Or your own custom exception, or just return false if you prefer.
                throw new InvalidOperationException(string.Format("Failed to copy {0} to {1}: {2}", source, destination, result));
            }
        }

        private Project GenerateWinStoreApp()
        {
            // 
            // Add the application files
            //
            Project proj = NewProject("quake3", ConfigurationType.Application);

            AddDefaultFileTypes(proj, "winrt");
            AddDefaultFileTypes(proj, "win8");

            proj.PackageCertificateKeyFile = RelativeCodePath + "win8\\Quake3_TemporaryKey.pfx";
            
            // Copy the assets folder
            CopyFolder(RelativeCodePath + "win8\\Assets", "Assets");

            // Mark all the images as deployment content
            string assetsPath = RelativeCodePath + "win8\\";
            proj.ExecuteForBuildItems("Assets\\*", (BuildItem item) =>
                {
                    item.Name = item.Name.Substring(assetsPath.Length);
                    item.SetOverride("*", "DeploymentContent", "True");
                });

            // Finally, set the post build event
            proj.AddBuildProperty("*", "PostBuildEvent", "Command",
                    RelativeCodePath + "win8\\AppxrecipeBlender.exe " +
                    "/mapfile \"" + RelativeCodePath + "win8\\packagelist.txt\" " +
                    "/recipefile \"$(OutDir)" + proj.ProjectName + ".build.appxrecipe\" " +
                    "/outfile \"$(OutDir)" + proj.ProjectName + ".build.appxrecipe\" " +
                    "/D CONFIGURATION \"$(Configuration)\" " +
                    "/D PLATFORM \"$(Platform)\" " +
                    "/D CODEDIR \"$(ProjectDir)" + RelativeCodePath + ".\" " + 
                    "/D OUTDIR \"$(OutDir).\"");
            return proj;
        }

        private Project GenerateUniversalApp(string path)
        {
            // 
            // Add the application files
            //
            Project proj = NewProject("quake3", ConfigurationType.Application);

            AddDefaultFileTypes(proj, "winrt");
            AddDefaultFileTypes(proj, path);

            proj.PackageCertificateKeyFile = RelativeCodePath + path + "\\Quake3_TemporaryKey.pfx";
            
            // Copy the assets folder
            CopyFolder(RelativeCodePath + path + "\\Assets", "Assets");

            // Mark all the images as deployment content
            string assetsPath = RelativeCodePath + path + "\\";
            proj.ExecuteForBuildItems("Assets\\*", (BuildItem item) =>
                {
                    item.Name = item.Name.Substring(assetsPath.Length);
                    item.SetOverride("*", "DeploymentContent", "True");
                });

            // Finally, set the post build event
            proj.AddBuildProperty("*", "PostBuildEvent", "Command",
                    RelativeCodePath + path + "\\AppxrecipeBlender.exe " +
                    "/mapfile \"" + RelativeCodePath + path + "\\packagelist.txt\" " +
                    "/recipefile \"$(OutDir)" + proj.ProjectName + ".build.appxrecipe\" " +
                    "/outfile \"$(OutDir)" + proj.ProjectName + ".build.appxrecipe\" " +
                    "/D CONFIGURATION \"$(Configuration)\" " +
                    "/D PLATFORM \"$(Platform)\" " +
                    "/D CODEDIR \"$(ProjectDir)" + RelativeCodePath + ".\" " + 
                    "/D OUTDIR \"$(OutDir).\"");
            return proj;
        }

        private Project GenerateXboApp()
        {
            Project proj = NewProject("quake3", ConfigurationType.Application);

            // 
            // Add the solution items
            //
            AddDefaultFileTypes(proj, "winrt");
            AddDefaultFileTypes(proj, "xbo");

            proj.AddProjectProperty("*", "PullMappingFile", proj.OutDir + "MappingFile.xml");

            // Finally, set the post build event
            //RegexReplace.exe
            // /in $(ProjectDir)MappingFile.xml 
            // /out $(OutDir)MappingFile.xml 
            // /D [SolutionDir] "$(SolutionDir)." 
            // /D [ProjectDir] "$(ProjectDir)." 
            // /D [OutDir] "$(OutDir)."
            proj.AddBuildProperty("*", "PostBuildEvent", "Command",
                    RelativeCodePath + "xbo\\RegexReplace.exe " +
                    "/in \"" + RelativeCodePath + "xbo\\MappingFile.xml\" " +
                    "/out \"$(OutDir)MappingFile.xml\" " +
                    "/D [CodeDir] \"$(SolutionDir)..\\code\" " +
                    "/D [OutDir] \"$(OutDir).\" ");

            // Copy the assets folder
            CopyFolder(RelativeCodePath + "xbo\\Assets", "Assets");

            // Mark all the images as deployment content
            string assetsPath = RelativeCodePath + "xbo\\";
            proj.ExecuteForBuildItems("Assets\\*", (BuildItem item) =>
            {
                item.Name = item.Name.Substring(assetsPath.Length);
                item.SetOverride("*", "DeploymentContent", "True");
            });
            
            proj.ExecuteForBuildItems("XMemCpy\\*", (BuildItem item) =>
            {
                item.ExcludedFromBuild.Set("*", true);
                item.SetOverride("*", "DeploymentContent", "false");
            });

            return proj;
        }

        public SolutionFilter GenerateContentFilter(SolutionFilter filter, IEnumerable<string> paths, string types, bool recurse)
        {
            foreach (string path in paths)
            {
                var files = FindSources(path, types, recurse);
                foreach (var f in files)
                {
                    filter.Items.Add(new SolutionItem(f));
                }
            }

            return filter;
        }

        public SolutionFilter GenerateContentFilter(SolutionFilter filter, string path, string types, bool recurse)
        {
            return GenerateContentFilter(filter, new string[] { path }, types, recurse);
        }

        static void Win10ConvertReferencesToDependencies(Project parentProj)
        {
            string linkerString = string.Empty;
            foreach (Project refProj in parentProj.ProjectReferences)
            {
                linkerString += string.Format("{0}.lib;", refProj.ProjectName);
                parentProj.ProjectDependencies.Add(refProj);
            }
            parentProj.AddBuildProperty("*", "Link", "AdditionalDependencies", linkerString);
            parentProj.AddBuildProperty("*", "Link", "AdditionalLibraryDirectories", @"$(SolutionDir)..\build\lib\$(SolutionName)\$(Platform)\$(Configuration)\");
            parentProj.ProjectReferences.Clear();
        }

        private void GenerateProjects()
        {
            //
            // Shared libs
            //
            SolutionFilter staticLibFilter = NewSlnFilter("Shared");
            _sln.Filters.Add(staticLibFilter);

            SolutionFilter qabi_filter = NewSlnFilter("QABI");

            Project qshared = GenerateQSharedLib(staticLibFilter);
            Project cgame_qabi = GenerateQabiGenProject("cgame", "cgvm", qabi_filter);
            Project game_qabi = GenerateQabiGenProject("game", "gvm", qabi_filter);
            Project ui_qabi = GenerateQabiGenProject("ui", "uivm", qabi_filter);
            Project engine_qabi = GenerateQabiGenEngineProject(qabi_filter);

            Project clientProj = GenerateDefaultStaticLib(staticLibFilter, "client");
            Project serverProj = GenerateServerLib(staticLibFilter);

            List<Project> qabiLibs = new List<Project>()
            {
                cgame_qabi,
                game_qabi,
                ui_qabi,
                engine_qabi
            };

            List<Project> serverLibs = new List<Project>()
            {
                qshared,
                GenerateDefaultStaticLib(staticLibFilter, "botlib"),
                serverProj,
                GenerateJpegLib(staticLibFilter),
                GenerateSplinesLib(staticLibFilter)
            };

            List<Project> clientLibs = new List<Project>()
            {
                clientProj,
                GenerateQCommonLib(staticLibFilter),
                GenerateDefaultStaticLib(staticLibFilter, "gamethread"),
                GenerateDefaultStaticLib(staticLibFilter, "xaudio"),
                GenerateDefaultStaticLib(staticLibFilter, "win", "winshared")
            };

            List<Project> renderLibs = new List<Project>()
            {
                GenerateRendererLib(staticLibFilter),
                GenerateD3DLib(staticLibFilter, "d3d", false),
            };

            switch (_config.DirectX)
            {
                case DirectXLevel.DX11Only:
                    renderLibs.Add(GenerateD3DLib(staticLibFilter, "d3d11", true));
                    break;
                case DirectXLevel.DX12Only:
                    renderLibs.Add(GenerateD3DLib(staticLibFilter, "d3d12", true));
                    break;
                case DirectXLevel.Both:
                    renderLibs.Add(GenerateD3DLib(staticLibFilter, "d3d11", true));
                    renderLibs.Add(GenerateD3DLib(staticLibFilter, "d3d12", true));
                    break;
            }

            if (_config.WantAccountLib)
            {
                clientLibs.Add(GenerateDefaultStaticLib(staticLibFilter, "account"));
            }

            if (_config.WantXInputLib)
            {
                clientLibs.Add(GenerateDefaultStaticLib(staticLibFilter, "xinput"));
            }

            // Executable (must be the first project in the solution!)
            Project exeProj = null;
            switch (_config.Target)
            {
                case Quake3Target.Desktop:
                    exeProj = GenerateDesktopApp();
                    break;
                case Quake3Target.WinStore:
                    exeProj = GenerateWinStoreApp();
                    break;
                case Quake3Target.Win10UWP:
                    exeProj = GenerateUniversalApp("uwp");
                    break;
                case Quake3Target.HoloLens:
                    exeProj = GenerateUniversalApp("hololens");
                    break;
                case Quake3Target.XboxOne:
                    exeProj = GenerateXboApp();
                    break;
                default:
                    throw new NotImplementedException();
            }

            _sln.Projects.Add(exeProj);

            // Set up project dependencies and add them to the SLN
            foreach (Project proj in qabiLibs)
            {
                _sln.Projects.Add(proj);
                exeProj.ProjectDependencies.Add(proj);
            }

            foreach (Project proj in serverLibs)
            {
                proj.ProjectDependencies.Add(engine_qabi);
                proj.ProjectDependencies.Add(game_qabi);
                _sln.Projects.Add(proj);
                exeProj.ProjectReferences.Add(proj);
            }

            foreach (Project proj in clientLibs)
            {
                proj.ProjectDependencies.Add(engine_qabi);
                proj.ProjectDependencies.Add(cgame_qabi);
                proj.ProjectDependencies.Add(ui_qabi);
                _sln.Projects.Add(proj);
                exeProj.ProjectReferences.Add(proj);
            }

            foreach (Project proj in renderLibs)
            {
                proj.ProjectDependencies.Add(engine_qabi);
                proj.ProjectDependencies.Add(cgame_qabi);
                proj.ProjectDependencies.Add(ui_qabi);
                _sln.Projects.Add(proj);
                exeProj.ProjectReferences.Add(proj);
            }

            // In WinRT, static libs are fucked up, so convert to dependencies with linkage
            if (_config.Platform.IsWinRT)
            {
                Win10ConvertReferencesToDependencies(exeProj);
            }
            
            // Set MISSIONPACK on static libs and exe
            foreach (Project proj in serverLibs)
            {
                proj.AddBuildProperty("*", "ClCompile", "PreprocessorDefinitions", "MISSIONPACK");
            }
            foreach (Project proj in clientLibs)
            {
                proj.AddBuildProperty("*", "ClCompile", "PreprocessorDefinitions", "MISSIONPACK");
            }
            exeProj.AddBuildProperty("*", "ClCompile", "PreprocessorDefinitions", "MISSIONPACK");

            // Shaders
            Project hlsl = GenerateShaderProject(staticLibFilter, "hlsl", "shaders");
            _sln.Projects.Add(hlsl);
            exeProj.ProjectDependencies.Add(hlsl);

            // VMs
            SolutionFilter vmFilter = NewSlnFilter("VMs");
            _sln.Filters.Add(vmFilter);
            vmFilter.Subfilters.Add(qabi_filter);
            SolutionFilter q3vmFilter = NewSlnFilter("Quake 3");
            vmFilter.Subfilters.Add(q3vmFilter);
            SolutionFilter tavmFilter = NewSlnFilter("Team Arena");
            vmFilter.Subfilters.Add(tavmFilter);

            _sln.Projects.Add(GenerateGameVM(q3vmFilter, new Project[] { qshared, engine_qabi, game_qabi }, Game.baseq3, "game"));
            _sln.Projects.Add(GenerateCgameVM(q3vmFilter, new Project[] { qshared, engine_qabi, cgame_qabi }, Game.baseq3, "cgame"));
            _sln.Projects.Add(GenerateQ3UIVM(q3vmFilter, new Project[] { qshared, engine_qabi, ui_qabi }));

            exeProj.ProjectDependencies.Add(FindProject("game"));
            exeProj.ProjectDependencies.Add(FindProject("cgame"));
            exeProj.ProjectDependencies.Add(FindProject("q3_ui"));

            if (_config.WantSeparateGameDLLs)
            {
                _sln.Projects.Add(GenerateGameVM(tavmFilter, new Project[] { qshared, engine_qabi, game_qabi }, Game.missionpack, "tagame"));
                _sln.Projects.Add(GenerateCgameVM(tavmFilter, new Project[] { qshared, engine_qabi, cgame_qabi }, Game.missionpack, "tacgame"));
                _sln.Projects.Add(GenerateUIVM(tavmFilter, new Project[] { qshared, engine_qabi, ui_qabi }));

                exeProj.ProjectDependencies.Add(FindProject("tagame"));
                exeProj.ProjectDependencies.Add(FindProject("tacgame"));
                exeProj.ProjectDependencies.Add(FindProject("ui"));
            }

            foreach (Project proj in _sln.Projects)
            {
                proj.PruneEmptyFilters();
            }
        }

        private SolutionFilter NewSlnFilter(string name)
        {
            return new SolutionFilter(name, _sln.NewGuid(name));
        }

        public Solution Generate()
        {
            Directory.CreateDirectory(_outputPath);

            try { _sln.LoadGuids(_sln.FilePath + ".guids"); } catch (Exception) { }

            //
            // Content
            //
            SolutionFilter configsAndMaps = NewSlnFilter("Content");
            _sln.Filters.Add(configsAndMaps);
            {
                configsAndMaps.Subfilters.Add(GenerateContentFilter(
                    NewSlnFilter("Q3"), "..\\baseq3", "*.cfg;*.txt", true));
                configsAndMaps.Subfilters.Add(GenerateContentFilter(
                    NewSlnFilter("TA"), "..\\missionpack", "*.cfg;*.txt", true));
                configsAndMaps.Subfilters.Add(GenerateContentFilter(
                    NewSlnFilter("Menus"), "..\\missionpack\\ui", "*.*", true));
                configsAndMaps.Subfilters.Add(GenerateContentFilter(
                    NewSlnFilter("Maps"), new string[] { "..\\baseq3", "..\\missionpack" }, "*.map", true));
            }

            //
            // Temporarily push the project dir
            //
            string envCurDir = Environment.CurrentDirectory;
            Environment.CurrentDirectory = _outputPath;

            try
            {
                GenerateProjects();
            }
            catch (Exception ex)
            {
                throw ex;
            }
            finally
            {
                // Restore the current directory
                Environment.CurrentDirectory = envCurDir;
            }

            //
            // Write the SLN Guids
            //
            _sln.SaveGuids(_sln.FilePath + ".guids");
            foreach (var proj in _sln.Projects)
            {
                proj.SaveGuids(proj.FilePath + ".guids");
            }

            return _sln;
        }
    }
}
