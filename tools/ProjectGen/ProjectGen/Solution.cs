using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class SolutionFilter
    {
        public readonly Guid Guid;
        public string Name = string.Empty;
        public List<SolutionFilter> Subfilters = new List<SolutionFilter>();
        public List<SolutionItem> Items = new List<SolutionItem>();

        public SolutionFilter(string name, Guid guid)
        {
            Guid = guid;
            Name = name;
        }
    };

    class SolutionItem
    {
        public string Name = string.Empty;

        public SolutionItem(string name)
        {
            Name = name;
        }
    };

    class Solution
    {
        public string Name = string.Empty;
        public string FilePath = string.Empty;
        public VisualStudioVersion VisualStudioVersion;
        public List<Project> Projects = new List<Project>();
        public List<SolutionFilter> Filters = new List<SolutionFilter>();

        private GuidGenerator _guidGen = new GuidGenerator();

        public Solution()
        {
        }

        public Solution(VisualStudioVersion vs, string solutionName = "", string filepath = "")
        {
            VisualStudioVersion = vs;
            Name = solutionName;
            FilePath = filepath;
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
