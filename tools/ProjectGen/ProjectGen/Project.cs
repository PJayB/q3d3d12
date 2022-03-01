using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class Project
    {
        public string ProjectName;
        public string FilePath;

        public readonly VisualStudioVersion VisualStudioVersion;
        public readonly Platform PlatformTarget;
        public readonly string PlatformToolset;

        public SolutionFilter LayoutParent = null;


        //  <PropertyGroup Label="Globals">
        //    <ProjectGuid>{44760134-0930-4F5F-A11B-7A0C85504CAE}</ProjectGuid>
        public readonly Guid ProjectGuid;
        public PropertyCollection GlobalProperties = new PropertyCollection();

        // Note that the following do NOT appear in non-WinStore apps:
        //    <RootNamespace>DirectXApp1</RootNamespace>
        //    <DefaultLanguage>en-US</DefaultLanguage>
        //    <MinimumVisualStudioVersion>12.0</MinimumVisualStudioVersion>
        //    <AppContainerApplication>true</AppContainerApplication>
        //    <ApplicationType>Windows Store</ApplicationType>
        //    <ApplicationTypeRevision>8.1</ApplicationTypeRevision>
        public readonly bool IsWinRTProject = false;
        public string RootNamespace;
        public string DefaultLanguage = "en-US";
        // public readonly string MinimumVisualStudioVersion; // e.g. 12.0 for 2013
        // public readonly bool AppContainerApplication; // e.g. true for WinRT apps
        // public readonly string ApplicationType; // e.g. Windows Store
        // public readonly string ApplicationTypeRevision; // e.g. 8.1 for Windows Store
        //  </PropertyGroup>

        //  <PropertyGroup>
        //    <PackageCertificateKeyFile>TemporaryKey.pfx</PackageCertificateKeyFile>
        public string PackageCertificateKeyFile = string.Empty;
        //  </PropertyGroup>

        // PropertyGroup conditional on configuration, Label="Configuration"
        //  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM'" Label="Configuration">
        //    <ConfigurationType>Application</ConfigurationType>
        //    <PlatformToolset>v120</PlatformToolset>
        //    <UseOfMfc>false</UseOfMfc>
        //    <UseDebugLibraries>true</UseDebugLibraries>
        public ConfigurationType ConfigurationType = ConfigurationType.Application;
        //public readonly string PlatformToolset; // e.g. v120
        public bool UseOfMfc = false;
        // public bool UseDebugLibraries; // Based off configuration
        //      (see SpecificBuildConfiguration)
        //  </PropertyGroup>

        // Must always do:
        //  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
        //  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
        // The rest seem to be predicated on platform but are all identical, so simplifying we get:
        //  <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />

        //  <PropertyGroup>
        //    <_ProjectFileVersion>11.0.60610.1</_ProjectFileVersion>
        // public readonly string ProjectFileVersion;
        //  </PropertyGroup>

        // PropertyGroup conditional on configuration, e.g.
        //  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM'">
        public string TargetName;
        public string OutDir;
        public string IntermediateDir;
        // public bool LinkIncremental; // default false in release, true in debug
        //   (see SpecificBuildConfiguration)
        //  </PropertyGroup>

        // All of these are predicated off configuration, e.g.
        //   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
        public List<BuildConfiguration> BuildConfigurations = new List<BuildConfiguration>();
        //   </ItemDefinitionGroup>

        // All these are grouped by type in <ItemGroup> tags
        public List<ProjectItem> RootLayoutItems = new List<ProjectItem>();

        // These emit like:
        //  <ItemGroup>
        //    <ProjectReference Include="project.vcxproj">
        //      <Project>{26165c8d-14ea-4bcc-8e90-ad5a5935bafe}</Project>
        //    </ProjectReference>
        public List<Project> ProjectDependencies = new List<Project>();
        public List<Project> ProjectReferences = new List<Project>();
        //  </ItemGroup>

        //   <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
        //   <ImportGroup Label="ExtensionTargets">
        //   </ImportGroup>
        // </Project>

        private GuidGenerator _guidGen = new GuidGenerator();

        public Project(
            VisualStudioVersion vsVersion,
            Platform platform,
            ConfigurationType type,
            Guid guid,
            string name = "", 
            string filepath = "")
        {
            if (!platform.Toolsets.ContainsKey(vsVersion))
                throw new InvalidOperationException(platform.Name + " projects are not compatible with " + vsVersion.LongName);

            // Copy some of the solution properties
            VisualStudioVersion = vsVersion;
            ProjectName = name;
            FilePath = filepath;
            PlatformTarget = platform;
            IsWinRTProject = platform.IsWinRT;
            ConfigurationType = type;
            
            PlatformToolset = PlatformTarget.Toolsets[VisualStudioVersion];

            ProjectGuid = guid;

            RootNamespace = name;
            TargetName = name;
            OutDir = @"$(SolutionDir)Build\$(SolutionName)\$(Platform)\$(Configuration)\";
            IntermediateDir = @"$(SolutionDir)Build\Intermediate\$(SolutionName)\$(Platform)\$(ProjectName)\$(Configuration)\";
        }

        public void AddBuildProperty(string configurationPattern, string stage, string propertyName, string value)
        {
            System.Text.RegularExpressions.Regex regex = Utils.WildcardToRegex(configurationPattern);
            foreach (BuildConfiguration bc in BuildConfigurations)
            {
                if (regex.IsMatch(bc.Name))
                {
                    bc.AddBuildProperty(stage, propertyName, value);
                }
            }
        }

        public void AddProjectProperty(string configurationPattern, string propertyName, string value)
        {
            System.Text.RegularExpressions.Regex regex = Utils.WildcardToRegex(configurationPattern);
            foreach (BuildConfiguration bc in BuildConfigurations)
            {
                if (regex.IsMatch(bc.Name))
                {
                    bc.AddProjectProperty(propertyName, value);
                }
            }
        }

        public void AddConfigurationProperty(string configurationPattern, string propertyName, string value)
        {
            System.Text.RegularExpressions.Regex regex = Utils.WildcardToRegex(configurationPattern);
            foreach (BuildConfiguration bc in BuildConfigurations)
            {
                if (regex.IsMatch(bc.Name))
                {
                    bc.AddConfigurationProperty(propertyName, value);
                }
            }
        }

        public static List<BuildConfiguration> CreateBuildConfigurationTable(
            ConfigurationType type,
            Platform platform,
            IEnumerable<Architecture> archs,
            IEnumerable<Configuration> configs)
        {
            List<BuildConfiguration> bcs = new List<BuildConfiguration>();
            foreach (Architecture arch in archs)
                foreach (Configuration config in configs)
                    bcs.Add(new BuildConfiguration(type, platform, arch, config));
            return bcs;
        }

        private void GatherFilterItemsRecursive(string type, List<ProjectItem> list, FilterItem filter)
        {
            foreach (ProjectItem item in filter.SubItems)
            {
                GatherProjectItemsRecursive(type, list, item);
            }
        }

        private void GatherProjectItemsRecursive(string type, List<ProjectItem> list, ProjectItem item)
        {
            if (item.Is(type))
            {
                list.Add(item);
            }

            // Recurse down if it's a filter
            if (item is FilterItem)
            {
                GatherFilterItemsRecursive(type, list, item as FilterItem);
            }
        }

        public List<ProjectItem> GatherProjectItemsOfType(string type)
        {
            List<ProjectItem> list = new List<ProjectItem>();
            foreach (ProjectItem item in RootLayoutItems)
            {
                GatherProjectItemsRecursive(type, list, item);
            }
            return list;
        }

        public static void PruneEmptyFilters(FilterItem root)
        {
            for (int i = 0; i < root.SubItems.Count; )
            {
                FilterItem filter = root.SubItems[i++] as FilterItem;
                if (filter != null)
                {
                    if (filter.SubItems.Count != 0)
                    {
                        PruneEmptyFilters(filter);
                    }
                    else
                    {
                        --i;
                        root.RemoveItem(filter);
                    }
                }
            }
        }

        public void PruneEmptyFilters()
        {
            for (int i = 0; i < RootLayoutItems.Count; )
            {
                FilterItem filter = RootLayoutItems[i++] as FilterItem;
                if (filter != null)
                {
                    if (filter.SubItems.Count != 0)
                    {
                        PruneEmptyFilters(filter);
                    }
                    else
                    {
                        RootLayoutItems.RemoveAt(--i);
                    }
                }
            }
        }

        private void ExecuteForBuildItems(ProjectItem root, System.Text.RegularExpressions.Regex regex, Action<BuildItem> action)
        {
            BuildItem bi = root as BuildItem;
            if (bi != null && regex.IsMatch(root.Name))
            {
                action(bi);
            }

            FilterItem fi = root as FilterItem;
            if (fi != null)
            {
                foreach (var i in fi.SubItems)
                {
                    ExecuteForBuildItems(i, regex, action);
                }
            }
        }

        public void ExecuteForBuildItems(string wildcard, Action<BuildItem> action)
        {
            System.Text.RegularExpressions.Regex regex = Utils.WildcardToRegex(wildcard);

            foreach (var i in RootLayoutItems)
                ExecuteForBuildItems(i, regex, action);
        }

        public void ExecuteForBuildItems(System.Text.RegularExpressions.Regex regex, Action<BuildItem> action)
        {
            foreach (var i in RootLayoutItems)
                ExecuteForBuildItems(i, regex, action);
        }

        public void LoadGuids(string guidFile)
        {
            _guidGen.Load(guidFile);
        }

        public void SaveGuids(string guidFile)
        {
            _guidGen.Write(guidFile);
        }

        public Guid NewGuid(string name)
        {
            return _guidGen.Next(name);
        }
    }
}
