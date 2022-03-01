using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace ProjectGen
{
    class SolutionExporter
    {
        public string TargetDirectory = string.Empty;

        private static bool BadString(string str)
        {
            return str == null || str == string.Empty;
        }
        
        private static string PrepareFilePath(string basepath, string filepath)
        {
            if (filepath == null || filepath == string.Empty)
            {
                throw new InvalidOperationException("FilePath cannot be null.");
            }

            if (basepath == null || basepath == string.Empty)
            {
                basepath = Environment.CurrentDirectory;
            }

            if (!Path.IsPathRooted(filepath))
            {
                filepath = Path.GetFullPath(
                    Path.Combine(basepath, filepath));
            }

            basepath = Path.GetDirectoryName(filepath);
            if (!Directory.Exists(basepath))
            {
                try
                {
                    Directory.CreateDirectory(basepath);
                }
                catch (Exception ex)
                {
                    throw new DirectoryNotFoundException(basepath + " not found.", ex);
                }
            }

            return filepath;
        }

        //-----------------------------------------------------------------
        //
        // --- EXPORT BUILD ITEMS ---
        //
        //-----------------------------------------------------------------
        static readonly string[] ExportTypes = new string[]
        {
            "ClCompile",
            "ClInclude",
            "Image",
            "AppxManifest",
            "Text",
            "None",
            "FxCompile"
        };

        //-----------------------------------------------------------------
        //
        // --- EXPORT PROJ ---
        //
        //-----------------------------------------------------------------
        private void ExportProjInternal(Project proj, XmlWriter xml)
        {
            bool winRT = proj.IsWinRTProject && proj.ConfigurationType != ConfigurationType.NMake;

            //<?xml version="1.0" encoding="utf-8"?>
            //<Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
            xml.WriteLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            xml.WriteLine("<Project DefaultTargets=\"Build\" ToolsVersion=\""
                + proj.VisualStudioVersion.ToolsVersion
                + "\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");
            xml.IncreaseIndent();

            //
            // Project configurations
            //
            xml.WriteLine("<ItemGroup Label=\"ProjectConfigurations\">");
            xml.IncreaseIndent();
            foreach (BuildConfiguration bc in proj.BuildConfigurations)
            {
                //    <ProjectConfiguration Include="Debug|Win32">
                //      <Configuration>Debug</Configuration>
                //      <Platform>Win32</Platform>
                //    </ProjectConfiguration>
                xml.WriteLine(string.Format("<ProjectConfiguration Include=\"{0}\">", bc.Name));
                xml.IncreaseIndent();
                xml.WriteLine(string.Format("<Configuration>{0}</Configuration>",
                    bc.Configuration.Name));
                xml.WriteLine(string.Format("<Platform>{0}</Platform>",
                    bc.Architecture.Name));
                xml.DecreaseIndent();
                xml.WriteLine("</ProjectConfiguration>");
            }
            xml.DecreaseIndent();
            xml.WriteLine("</ItemGroup>");

            xml.WriteLine("<PropertyGroup Label=\"Globals\">");
            xml.IncreaseIndent();

            // Project GUID
            xml.WriteLine(string.Format("<ProjectGuid>{0}</ProjectGuid>",
                Utils.GuidString(proj.ProjectGuid)));
            xml.WriteLine("<DefaultLanguage>" + proj.DefaultLanguage + "</DefaultLanguage>");

            if (proj.VisualStudioVersion.WindowsTargetPlatformVersion != null)
            {
                xml.WriteLine("<WindowsTargetPlatformVersion>" + proj.VisualStudioVersion.WindowsTargetPlatformVersion + "</WindowsTargetPlatformVersion>");
            }

            // Windows RT stuff...
            if (winRT)
            {
                xml.WriteLine("<RootNamespace>" + proj.RootNamespace + "</RootNamespace>");
                xml.WriteLine("<MinimumVisualStudioVersion>" + proj.VisualStudioVersion.VersionNumber + "</MinimumVisualStudioVersion>");
            }

            if (proj.PlatformTarget.GlobalProperties != null)
            {
                foreach (var prop in proj.PlatformTarget.GlobalProperties)
                {
                    if (proj.GlobalProperties != null && !proj.GlobalProperties.Contains(prop.Key))
                    {
                        xml.EmitTag(prop.Key, prop.Value.ToString());
                    }
                }
            }

            if (proj.GlobalProperties != null)
            {
                foreach (var prop in proj.GlobalProperties)
                {
                    xml.EmitTag(prop.Key, prop.Value.ToString());
                }
            }

            xml.DecreaseIndent();
            xml.WriteLine("</PropertyGroup>");

            // Package Certificate Key file for WinRT stuff
            if (proj.PlatformTarget.RequiresPFX)
            {
                xml.WriteLine("<PropertyGroup>");
                xml.IncreaseIndent();
                xml.WriteLine("<PackageCertificateKeyFile>" + proj.PackageCertificateKeyFile + "</PackageCertificateKeyFile>");
                xml.WriteLine("<AppxAutoIncrementPackageRevision>True</AppxAutoIncrementPackageRevision>");
                xml.WriteLine("<AppxBundlePlatforms>x86|x64|arm</AppxBundlePlatforms>");
                xml.DecreaseIndent();
                xml.WriteLine("</PropertyGroup>");
            }

            // PropertyGroup "Configuration"
            foreach (BuildConfiguration bc in proj.BuildConfigurations)
            {
                xml.WriteLine(string.Format("<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{0}'\" Label=\"Configuration\">", bc.Name));
                xml.IncreaseIndent();
                xml.WriteLine("<ConfigurationType>" + Utils.ConfigurationTypeStr(proj.ConfigurationType) + "</ConfigurationType>");
                xml.WriteLine("<PlatformToolset>" + proj.PlatformToolset + "</PlatformToolset>");

                foreach (var prop in bc.ConfigurationProperties)
                {
                    xml.EmitTag(prop.Key, prop.Value.ToString());
                }

                xml.DecreaseIndent();
                xml.WriteLine("</PropertyGroup>");
            }

            if (proj.ConfigurationType != ConfigurationType.NMake || (proj.ConfigurationType == ConfigurationType.NMake && proj.VisualStudioVersion.WantNmakeCppImports))
            {
                xml.WriteLine("<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />");
                xml.WriteLine("<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />");
                xml.WriteLine("<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />");
            }

            xml.WriteLine("<PropertyGroup>");
            xml.IncreaseIndent();
            xml.WriteLine("<_ProjectFileVersion>" + proj.VisualStudioVersion.ProjectFileVersion + "</_ProjectFileVersion>");
            xml.DecreaseIndent();
            xml.WriteLine("</PropertyGroup>");

            foreach (BuildConfiguration bc in proj.BuildConfigurations)
            {
                xml.WriteLine(string.Format("<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{0}'\">", bc.Name));
                xml.IncreaseIndent();

                foreach (var prop in bc.ProjectProperties)
                {
                    xml.EmitTag(prop.Key, prop.Value.ToString());
                }

                xml.WriteLine("<TargetName>" + proj.TargetName + "</TargetName>");
                xml.WriteLine("<OutDir>" + proj.OutDir + "</OutDir>");
                xml.WriteLine("<IntDir>" + proj.IntermediateDir + "</IntDir>");

                xml.DecreaseIndent();
                xml.WriteLine("</PropertyGroup>");
            }

            if (proj.ConfigurationType != ConfigurationType.NMake)
            {
                foreach (BuildConfiguration bc in proj.BuildConfigurations)
                {
                    xml.WriteLine(string.Format("<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='{0}'\">", bc.Name));
                    xml.IncreaseIndent();

                    foreach (CompilationStage stage in bc.CompilationStages)
                    {
                        xml.OpenTag(stage.Name);
                        foreach (var propertyKey in stage.Properties)
                        {
                            string value = propertyKey.Value.ToString(propertyKey.Key);
                            if (value != string.Empty)
                            {
                                xml.EmitTag(propertyKey.Key, value);
                            }
                        }
                        xml.CloseTag(stage.Name);
                    }

                    xml.DecreaseIndent();
                    xml.WriteLine("</ItemDefinitionGroup>");
                }
            }

            // Per-resource item groups
            foreach (string itemType in ExportTypes)
            {
                List<ProjectItem> items = proj.GatherProjectItemsOfType(itemType);
                if (items.Count > 0)
                {
                    xml.WriteLine("<ItemGroup>");
                    xml.IncreaseIndent();
                    foreach (ProjectItem item in items)
                    {
                        item.EmitProjectXml(xml, proj);
                    }
                    xml.DecreaseIndent();
                    xml.WriteLine("</ItemGroup>");
                }
            }

            // Project reference xml
            if (proj.ProjectReferences != null && proj.ProjectReferences.Count > 0)
            {
                xml.WriteLine("<ItemGroup>");
                xml.IncreaseIndent();
                foreach (Project dep in proj.ProjectReferences)
                {
                    // TODO: HACK: This should be the relative path from project to project
                    xml.WriteLine("<ProjectReference Include=\"" + Path.GetFileName(dep.FilePath) + "\">");
                    xml.IncreaseIndent();
                    xml.WriteLine("<Project>" + Utils.GuidString(dep.ProjectGuid) + "</Project>");

                    if (dep.ConfigurationType == ConfigurationType.StaticLibrary && winRT)
                    {
                        // Static libraries have to be treated differently.
                        xml.EmitTag("ReferenceOutputAssembly", "false");
                        xml.EmitTag("LinkLibraryDependencies", "true");
                    }

                    xml.DecreaseIndent();
                    xml.WriteLine("</ProjectReference>");
                }
                xml.DecreaseIndent();
                xml.WriteLine("</ItemGroup>");
            }
            
            xml.WriteLine("<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
            xml.WriteLine("<ImportGroup Label=\"ExtensionTargets\" />");

            xml.DecreaseIndent();
            xml.WriteLine("</Project>");
        }

        private void ExportTypeFiltersInternal(Project proj, XmlWriter xml, string itemType)
        {
            List<ProjectItem> items = proj.GatherProjectItemsOfType(itemType);
            if (items.Count > 0)
            {
                xml.WriteLine("<ItemGroup>");
                xml.IncreaseIndent();
                foreach (ProjectItem item in items)
                {
                    item.EmitFilterXml(xml);
                }
                xml.DecreaseIndent();
                xml.WriteLine("</ItemGroup>");
            }
        }

        private void ExportProjFiltersInternal(Project proj, XmlWriter xml)
        {
            xml.WriteLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            xml.WriteLine(string.Format(
                "<Project ToolsVersion=\"{0}\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">",
                proj.VisualStudioVersion.ToolsVersion));
            xml.IncreaseIndent();

            ExportTypeFiltersInternal(proj, xml, "Filter");

            // Iterate through each type of build item and emit the XML
            foreach (string itemType in ExportTypes)
            {
                ExportTypeFiltersInternal(proj, xml, itemType);
            }

            xml.DecreaseIndent();
            xml.WriteLine("</Project>");
        }

        public void Export(Project proj)
        {
            if (BadString(proj.FilePath))
                throw new InvalidDataException("FilePath is null.");
            if (BadString(proj.RootNamespace))
                throw new InvalidDataException("RootNamespace is null.");
            if (BadString(proj.TargetName))
                throw new InvalidDataException("TargetName is null.");
            if (BadString(proj.OutDir))
                throw new InvalidDataException("OutDir is null.");
            if (BadString(proj.IntermediateDir))
                throw new InvalidDataException("IntermediateDir is null.");
            if (proj.ConfigurationType == ConfigurationType.Application && proj.PlatformTarget.RequiresPFX)
            {
                if (BadString(proj.PackageCertificateKeyFile))
                    throw new InvalidDataException("PackageCertificateKeyFile is null.");
            }
            if (proj.BuildConfigurations == null || proj.BuildConfigurations.Count == 0)
                throw new InvalidDataException("There must be at least one build configuration.");

            string filepath = PrepareFilePath(TargetDirectory, proj.FilePath);

            using (XmlWriter xml = new XmlWriter(filepath))
            {
                ExportProjInternal(proj, xml);
            }

            using (XmlWriter xml = new XmlWriter(filepath + ".filters"))
            {
                ExportProjFiltersInternal(proj, xml);
            }
        }

        //-----------------------------------------------------------------
        //
        // --- EXPORT SLN ---
        //
        //-----------------------------------------------------------------
        public const string GlobalFilterGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";

        private void ExportFilterRecursive(SolutionFilter filter, XmlWriter xml)
        {
            xml.WriteLine(string.Format(
                "Project(\"{0}\") = \"{1}\", \"{2}\", \"{3}\"",
                GlobalFilterGuid, // item type
                filter.Name,
                filter.Name,
                Utils.GuidString(filter.Guid)));
            xml.IncreaseIndent();

            // Write solution items
            if (filter.Items != null && filter.Items.Count > 0)
            {
                xml.WriteLine("ProjectSection(SolutionItems) = preProject");
                xml.IncreaseIndent();
                foreach (var item in filter.Items)
                {
                    xml.WriteLine(string.Format("{0} = {0}", item.Name));
                }
                xml.DecreaseIndent();
                xml.WriteLine("EndProjectSection");
            }

            xml.DecreaseIndent();
            xml.WriteLine("EndProject");

            // Write subfilters
            if (filter.Subfilters != null)
            {
                foreach (var subfilter in filter.Subfilters)
                {
                    ExportFilterRecursive(subfilter, xml);
                }
            }
        }

        private void ExportFilterHierarchyRecursive(SolutionFilter filter, XmlWriter xml)
        {
            if (filter.Subfilters != null && filter.Subfilters.Count > 0)
            {
                foreach (var subfilter in filter.Subfilters)
                {
                    ExportFilterHierarchyRecursive(subfilter, xml);

                    xml.WriteLine(string.Format("{0} = {1}",
                        Utils.GuidString(subfilter.Guid),
                        Utils.GuidString(filter.Guid)));
                }
            }
        }

        public const string GlobalProjectGuid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";

        private void ExportSlnInternal(Solution sln, XmlWriter xml)
        {
            // Microsoft Visual Studio Solution File, Format Version 12.00
            // # Visual Studio 2013
            xml.WriteLine("Microsoft Visual Studio Solution File, Format Version " + sln.VisualStudioVersion.SolutionFileVersion);
            xml.WriteLine("# " + sln.VisualStudioVersion.LongName);
            xml.WriteLine("VisualStudioVersion = " + sln.VisualStudioVersion.LongVersionNumber);
            xml.WriteLine("MinimumVisualStudioVersion = " + sln.VisualStudioVersion.BackCompatVersion);
            
            foreach (SolutionFilter filter in sln.Filters)
            {
                ExportFilterRecursive(filter, xml);
            }

            foreach (Project proj in sln.Projects)
            {
                xml.WriteLine(string.Format(
                    "Project(\"{0}\") = \"{1}\", \"{2}\", \"{3}\"",
                    GlobalProjectGuid, // item type
                    proj.ProjectName,
                    proj.FilePath,
                    Utils.GuidString(proj.ProjectGuid)));

                xml.IncreaseIndent();
                if ((proj.ProjectReferences != null && proj.ProjectReferences.Count > 0) ||
                    (proj.ProjectDependencies != null && proj.ProjectDependencies.Count > 0))
                {
                    xml.WriteLine("ProjectSection(ProjectDependencies) = postProject");
                    xml.IncreaseIndent();
                    if (proj.ProjectDependencies != null)
                    {
                        foreach (Project dep in proj.ProjectDependencies)
                        {
                            xml.WriteLine(string.Format("{0} = {0}",
                                Utils.GuidString(dep.ProjectGuid)));
                        }
                    }
                    if (proj.ProjectReferences != null)
                    {
                        foreach (Project dep in proj.ProjectReferences)
                        {
                            xml.WriteLine(string.Format("{0} = {0}",
                                Utils.GuidString(dep.ProjectGuid)));
                        }
                    }
                    xml.DecreaseIndent();
                    xml.WriteLine("EndProjectSection");
                }
                xml.DecreaseIndent();

                xml.WriteLine("EndProject");
            }

            xml.WriteLine("Global");
            xml.IncreaseIndent();

            xml.WriteLine("GlobalSection(SolutionConfigurationPlatforms) = preSolution");
            xml.IncreaseIndent();
            var slnConfigs = new Dictionary<string, BuildConfiguration>();
            foreach (Project proj in sln.Projects)
            {
                foreach (BuildConfiguration bc in proj.BuildConfigurations)
                {
                    if (!slnConfigs.ContainsKey(bc.Name) && proj.ConfigurationType != ConfigurationType.NMake)
                        slnConfigs.Add(bc.Name, bc);
                }
            }
            foreach (var cfg in slnConfigs)
            {
                xml.WriteLine(string.Format("{0} = {0}", cfg.Key));
            }
            xml.DecreaseIndent();
	        xml.WriteLine("EndGlobalSection");

            xml.WriteLine("GlobalSection(ProjectConfigurationPlatforms) = postSolution");
            xml.IncreaseIndent();
            string[] projectLines = new string[] { "ActiveCfg", "Build.0", "Deploy.0" };
            foreach (Project proj in sln.Projects)
            {
                if (proj.ConfigurationType == ConfigurationType.NMake)
                {
                    foreach (var slnConfig in slnConfigs)
                    {
                        foreach (BuildConfiguration bc in proj.BuildConfigurations)
                        {
                            if (bc.Configuration.Name == slnConfig.Value.Configuration.Name)
                            {
                                foreach (string line in projectLines)
                                {
                                    xml.WriteLine(string.Format("{0}.{1}.{2} = {3}",
                                        Utils.GuidString(proj.ProjectGuid),
                                        slnConfig.Key,
                                        line,
                                        bc.Name));
                                }
                            }
                        }
                    }
                }
                else
                {
                    foreach (BuildConfiguration bc in proj.BuildConfigurations)
                    {
                        foreach (string line in projectLines)
                        {
                            xml.WriteLine(string.Format("{0}.{1}.{2} = {1}",
                                Utils.GuidString(proj.ProjectGuid),
                                bc.Name,
                                line));
                        }
                    }
                }
            }
            xml.DecreaseIndent();
            xml.WriteLine("EndGlobalSection");

            xml.WriteLine("GlobalSection(SolutionProperties) = preSolution");
            xml.IncreaseIndent();
            xml.WriteLine("HideSolutionNode = FALSE");
            xml.DecreaseIndent();
            xml.WriteLine("EndGlobalSection");

            xml.WriteLine("GlobalSection(NestedProjects) = preSolution");
            xml.IncreaseIndent();
            foreach (SolutionFilter filter in sln.Filters)
            {
                ExportFilterHierarchyRecursive(filter, xml);
            }
            foreach (Project proj in sln.Projects)
            {
                if (proj.LayoutParent != null)
                {
                    xml.WriteLine(string.Format("{0} = {1}",
                        Utils.GuidString(proj.ProjectGuid),
                        Utils.GuidString(proj.LayoutParent.Guid)));
                }
            }

            xml.DecreaseIndent();
            xml.WriteLine("EndGlobalSection");

            xml.DecreaseIndent();
            xml.WriteLine("EndGlobal");
        }

        public void Export(Solution sln, bool exportProjectsToo)
        {
            if (BadString(sln.FilePath))
                throw new InvalidDataException("FilePath is null.");
            if (BadString(sln.Name))
                throw new InvalidDataException("Solution doesn't have a name.");
            if (sln.VisualStudioVersion == null)
                throw new NullReferenceException("VisualStudioVersion cannot be null.");
            
            string filepath = PrepareFilePath(TargetDirectory, sln.FilePath);
            XmlWriter xml = new XmlWriter(filepath);

            using (xml)
            {
                ExportSlnInternal(sln, xml);
            }

            //-----------------------------------------------------------------
            //
            // Export the projects as well if necessary
            //
            //-----------------------------------------------------------------
            if (exportProjectsToo)
            {
                foreach (Project proj in sln.Projects)
                    Export(proj);
            }
       }
    }
}
