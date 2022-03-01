using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace ProjectGen
{
    class CompilationStageCollection : List<CompilationStage>
    {
        public CompilationStageCollection()
            : base()
        {

        }

        public void SetProperty(string stageName, string propertyName, string value)
        {
            foreach (CompilationStage stage in this)
            {
                if (stageName == stage.Name)
                {
                    stage.Properties.Add(propertyName, value);
                }
            }
        }

        public bool ContainsStage(string name)
        {
            foreach (CompilationStage stage in this)
            {
                if (stage.Name == name)
                    return true;
            }
            return false;
        }

        public CompilationStage FindStage(string name)
        {
            foreach (CompilationStage stage in this)
            {
                if (stage.Name == name)
                    return stage;
            }
            throw new KeyNotFoundException("Couldn't find compilation stage '" + name + "'");
        }

        public void Merge(CompilationStageCollection other)
        {
            if (other == null)
                return;

            foreach (CompilationStage otherStage in other)
            {
                // Create it or get it
                CompilationStage thisStage;
                if (ContainsStage(otherStage.Name))
                {
                    thisStage = FindStage(otherStage.Name);
                }
                else
                {
                    thisStage = new CompilationStage(otherStage.Name);
                    Add(thisStage);
                }
                
                thisStage.Properties.Add(otherStage.Properties);                
            }
        }

        public static CompilationStageCollection Clone(CompilationStageCollection a)
        {
            CompilationStageCollection b = new CompilationStageCollection();
            b.Merge(a);
            return b;
        }
    }    

    class CompilationStage
    {
        public readonly string Name;
        public PropertyCollection Properties;

        public CompilationStage(string name)
            : base()
        {
            Name = name;
            Properties = new PropertyCollection();
        }
    }

    class ClCompile : CompilationStage
    {
        public const string Identifier = "ClCompile";

        public ClCompile(PropertyCollection props)
            : base(Identifier)
        {
            Properties.SetPrependProperty("AdditionalUsingDirectories", true);
            Properties.SetPrependProperty("AdditionalIncludeDirectories", true);
            Properties.SetPrependProperty("AdditionalOptions", true, ' ');
            Properties.SetPrependProperty("PreprocessorDefinitions", true);
            Properties.SetPrependProperty("TreatSpecificWarningsAsErrors", true);
            Properties.SetPrependProperty("UndefinePreprocessorDefinitions", true);
            Properties.SetPrependProperty("DisableSpecificWarnings", true);
            Properties.Add(props);
        }
    }

    class Link : CompilationStage
    {
        public const string Identifier = "Link";

        public Link(PropertyCollection props)
            : base(Identifier)
        {
            Properties.SetPrependProperty("AdditionalDependencies", true);
            Properties.SetPrependProperty("AdditionalLibraryDirectories", true);
            Properties.SetPrependProperty("IgnoreSpecificDefaultLibraries", true, ' ');
            Properties.SetPrependProperty("ForceSymbolReferences", true);
            Properties.SetPrependProperty("DelayLoadDLLs", true);
            Properties.SetPrependProperty("AdditionalManifestDependencies", true);
            Properties.SetPrependProperty("AdditionalOptions", true);
            Properties.Add(props);
        }
    }

    class Lib : CompilationStage
    {
        public const string Identifier = "Lib";

        public Lib(PropertyCollection props)
            : base(Identifier)
        {
            Properties.SetPrependProperty("AdditionalDependencies", true);
            Properties.SetPrependProperty("AdditionalLibraryDirectories", true);
            Properties.SetPrependProperty("AdditionalOptions", true);
            Properties.SetPrependProperty("IgnoreSpecificDefaultLibraries", true, ' ');
            Properties.SetPrependProperty("RemoveObjects", true);
            Properties.SetPrependProperty("ForceSymbolReferences", true);
            Properties.Add(props);
        }
    }

    class ResourceCompile : CompilationStage
    {
        public const string Identifier = "ResourceCompile";

        public ResourceCompile(PropertyCollection props)
            : base(Identifier)
        {
            Properties.SetPrependProperty("PreprocessorDefinitions", true);
            Properties.Add(props);
        }
    }

    class FxCompile : CompilationStage
    {
        public const string Identifier = "FxCompile";

        public FxCompile(PropertyCollection props)
            : base(Identifier)
        {
            Properties.SetPrependProperty("AdditionalIncludeDirectories", true);
            Properties.SetPrependProperty("AdditionalOptions", true, ' ');
            Properties.SetPrependProperty("PreprocessorDefinitions", true);
            Properties.Add(props);
        }
    }
}
