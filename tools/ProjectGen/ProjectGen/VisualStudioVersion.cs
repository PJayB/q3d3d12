using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class VisualStudioVersion
    {
        public string Name;
        public string LongName;
        public string VersionNumber;
        public string LongVersionNumber;
        public string SolutionFileVersion;
        public string ProjectFileVersion;
        public string ToolsVersion;
        public string BackCompatVersion;
        public string WindowsTargetPlatformVersion;
        public bool WantNmakeCppImports;

        public static readonly VisualStudioVersion VS2012 = new VisualStudioVersion()
        {
            Name = "VS2012",
            LongName = "Visual Studio 2012",
            SolutionFileVersion = "12.00",
            ProjectFileVersion = "11.0.60610.1",
            VersionNumber = "11.0",
            LongVersionNumber = "11.0.50727.1",
            BackCompatVersion = "10.0.40219.1",
            ToolsVersion = "4.0",
            WantNmakeCppImports = false
        };

        public static readonly VisualStudioVersion VS2013 = new VisualStudioVersion()
        {
            Name = "VS2013",
            LongName = "Visual Studio 2013",
            SolutionFileVersion = "12.00",
            ProjectFileVersion = "11.0.60610.1",
            VersionNumber = "12.0",
            LongVersionNumber = "12.0.30501.0",
            BackCompatVersion = "10.0.40219.1",
            ToolsVersion = "12.0",
            WantNmakeCppImports = true
        };

        public static readonly VisualStudioVersion VS2015 = new VisualStudioVersion()
        {
            Name = "VS2015",
            LongName = "Visual Studio 2015",
            SolutionFileVersion = "12.00",
            ProjectFileVersion = "11.0.60610.1",
            VersionNumber = "14.0",
            LongVersionNumber = "14.0.22310.1",
            BackCompatVersion = "10.0.40219.1",
            ToolsVersion = "14.0",
            WantNmakeCppImports = true,
            WindowsTargetPlatformVersion = "10.0.14393.0"
        };
    }
}
