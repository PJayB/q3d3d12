using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen.Tests
{
    static class RandomSolution
    {
        static readonly Dictionary<string, string> ExtensionToFileTypeMapping = new Dictionary<string, string>()
        {
            { ".cpp", "ClCompile" },
            { ".h", "ClInclude" },
            { ".ico", "Image" },
            { ".bmp", "Image" },
            { ".appxmanifest", "AppxManifest" },
            { ".txt", "Text" },
            { ".pfx", "None" },
            { ".hlsl", "FxCompile" }
        };

        static readonly string[] Extensions = ExtensionToFileTypeMapping.Keys.ToArray();

        static string GenerateRandomName(Random rng)
        {
            const string Selection = "abcdefghijklmnopqrstuvwxyz0123456789";

            StringBuilder s = new StringBuilder();
            for (int i = 0; i < 8; ++i)
                s.Append(Selection[rng.Next(0, Selection.Length - 1)]);

            return s.ToString();
        }

        static HlslShaderType GenerateRandomHlslType(Random rng)
        {
            HlslShaderType[] types = new HlslShaderType[]
            {
                HlslShaderType.Vertex,
                HlslShaderType.Pixel,
                HlslShaderType.Compute
            };

            return types[rng.Next(0, types.Length - 1)];
        }

        static BuildItem GeneratePCH(Project proj)
        {
            BuildItem item = new BuildItem("ClCompile", "pch.cpp");
            item.SetOverride("*", "PrecompiledHeader", "Create");
            return item;
        }

        static BuildItem GenerateBuildItem(FilterItem parent, Random rng, string type, string name)
        {
            BuildItem item = new BuildItem(type, name);

            parent.AddItem(item);

            if (type == "FxCompile")
            {
                item.SetOverride("*", "ShaderType", GenerateRandomHlslType(rng).ToString());
            }

            return item;
        }

        static string GenerateRandomFileName(Random rng)
        {
            string extension = Extensions[rng.Next(0, Extensions.Length - 1)];
            return GenerateRandomName(rng) + extension;
        }

        static ProjectItem GenerateRandomFile(FilterItem parent, Random rng)
        {
            string name = GenerateRandomFileName(rng);
            string type = ExtensionToFileTypeMapping[System.IO.Path.GetExtension(name)];

            BuildItem item = GenerateBuildItem(parent, rng, type, name);

            if (rng.NextDouble() < 0.2)
            {
                item.ExcludedFromBuild.Set("Debug|x64", true);
                item.ExcludedFromBuild.Set("Profile|x64", true);
                item.ExcludedFromBuild.Set("Release|x64", true);
                item.ExcludedFromBuild.Set("Debug|Win32", false);
                item.ExcludedFromBuild.Set("Profile|Win32", false);
                item.ExcludedFromBuild.Set("Release|Win32", false);
                item.ExcludedFromBuild.Set("Debug|ARM", false);
                item.ExcludedFromBuild.Set("Profile|ARM", false);
                item.ExcludedFromBuild.Set("Release|ARM", false);
            }

            return item;
        }

        static ProjectItem GenerateRandomFilter(FilterItem parent, Random rng, int depth)
        {
            FilterItem filter = new FilterItem(GenerateRandomName(rng), Guid.NewGuid());


            filter.Parent = parent;

            int fileCount = rng.Next(2, 8);
            while (fileCount-- > 0)
            {
                filter.AddItem(GenerateRandomItem(filter, rng, depth + 1));
            }
            return filter;
        }

        static ProjectItem GenerateRandomItem(FilterItem parent, Random rng, int depth)
        {
            if (rng.NextDouble() < 0.3 && depth < 4)
            {
                return GenerateRandomFilter(parent, rng, depth);
            }
            else
            {
                return GenerateRandomFile(parent, rng);
            }
        }

        static List<ProjectItem> GenerateRandomItems(Project proj)
        {
            Random rng = new Random();
            List<ProjectItem> items = new List<ProjectItem>();

            items.Add(GeneratePCH(proj));

            int fileCount = rng.Next(16, 24);
            while (fileCount-- > 0)
            {
                items.Add(GenerateRandomItem(null, rng, 0));
            }

            return items;
        }

        static void GenerateSolutionItems(Random rng, SolutionFilter parent, int depth)
        {
            int subfilters = depth < 3 ? rng.Next(3, 8) : 0;
            for (int i = 0; i < subfilters; ++i)
            {
                SolutionFilter subFilter = new SolutionFilter(GenerateRandomName(rng), Guid.NewGuid());
                parent.Subfilters.Add(subFilter);
                GenerateSolutionItems(rng, subFilter, depth + 1);
            }

            int items = rng.Next(3, 8);
            for (int i = 0; i < items; ++i)
            {
                parent.Items.Add(new SolutionItem(GenerateRandomFileName(rng)));
            }
        }

        static void GenerateSolutionItems(Solution sln)
        {
            Random rng = new Random();
            int filters = rng.Next(3, 6);
            for (int i = 0; i < filters; ++i)
            {
                SolutionFilter filter = new SolutionFilter(GenerateRandomName(rng), Guid.NewGuid());
                sln.Filters.Add(filter);
                GenerateSolutionItems(rng, filter, 0);
            }
        }

        public static Solution Generate(VisualStudioVersion vsver)
        {
            Random rng = new Random();
            string slnName = GenerateRandomName(rng);

            Solution sln = new Solution(vsver, slnName, slnName + vsver.Name + ".sln");

            GenerateSolutionItems(sln);

            int projectCount = rng.Next(4, 8);
            for (int i = 0; i < projectCount; ++i)
            {
                string projName = GenerateRandomName(rng);

                Project proj = new Project(
                    sln.VisualStudioVersion,
                    Platform.WindowsDesktop,
                    ConfigurationType.StaticLibrary,
                    Guid.NewGuid(),
                    projName, 
                    projName + vsver.Name + ".vcxproj");
                proj.BuildConfigurations = Project.CreateBuildConfigurationTable(
                    proj.ConfigurationType,
                    proj.PlatformTarget,
                    new Architecture[] {
                        Architecture.x86,
                        Architecture.x64},
                    new Configuration[] { 
                        Configuration.Debug,
                        Configuration.Profile,
                        Configuration.Release });
                proj.RootLayoutItems = GenerateRandomItems(proj);

                if (rng.NextDouble() < 0.4 && sln.Filters.Count > 0)
                {
                    int filterIndex = rng.Next(0, sln.Filters.Count);
                    proj.LayoutParent = sln.Filters[filterIndex];
                }

                // Random dependencies
                if (rng.NextDouble() < 0.4 && sln.Projects.Count > 0)
                {
                    int depIndex = rng.Next(0, sln.Projects.Count);
                    proj.ProjectReferences.Add(sln.Projects[depIndex]);
                }

                sln.Projects.Add(proj);
            }

            return sln;
        }
    }
}
