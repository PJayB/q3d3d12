using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    abstract class ProjectItem
    {
        public string Name;
        public string ItemType;

        public FilterItem Parent { get; set; }

        public ProjectItem(string itemType, string name)
        {
            Name = name;
            ItemType = itemType;
        }

        public bool Is(string type) { return ItemType.Equals(type); }

        public abstract void EmitProjectXml(XmlWriter xml, Project project);
        public abstract void EmitFilterXml(XmlWriter xml);
    }

    sealed class FilterItem : ProjectItem
    {
        public List<ProjectItem> _subItems = new List<ProjectItem>();


        public readonly Guid Guid;
        public List<string> Extensions = new List<string>();


        public FilterItem(string name, Guid guid)
            : base("Filter", name)
        {
            Guid = guid;
        }

        public IReadOnlyList<ProjectItem> SubItems { get { return _subItems; } }

        public void AddItem(ProjectItem item)
        {
            if (item.Parent != null)
                item.Parent.RemoveItem(item);

            if (!_subItems.Contains(item))
            {
                _subItems.Add(item);
                item.Parent = this;
            }
        }

        public void RemoveItem(ProjectItem item)
        {
            if (_subItems.Contains(item))
            {
                _subItems.Remove(item);
                item.Parent = null;
            }
        }

        public string GetFullPath()
        {
            string fullpath = Name;

            ProjectItem parent = Parent;
            while (parent != null)
            {
                fullpath = parent.Name + '\\' + fullpath;
                parent = parent.Parent;
            }

            return fullpath;
        }

        public override sealed void EmitProjectXml(XmlWriter xml, Project project)
        {
            throw new NotImplementedException();
        }

        public override sealed void EmitFilterXml(XmlWriter xml)
        {
            xml.WriteLine("<Filter Include=\"" + GetFullPath() + "\">");
            xml.IncreaseIndent();
            if (Guid != Guid.Empty)
            {
                xml.WriteLine("<UniqueIdentifier>" + Utils.GuidString(Guid) + "</UniqueIdentifier>");
            }
            if (Extensions != null && Extensions.Count > 0)
            {
                xml.WriteLine("<Extensions>" + Utils.JoinToString(Extensions, ';') + "</Extensions>");
            }
            xml.DecreaseIndent();
            xml.WriteLine("</Filter>");
        }
    }
      
    // Without an overload, these emit like:
    //  <ItemGroup>
    //    <{Type, e.g. Text} Include="somefile.txt" />
    //  </ItemGroup>
    sealed class BuildItem : ProjectItem
    {
        public ConfigurationSpecific<bool> ExcludedFromBuild = new ConfigurationSpecific<bool>();

        private BuildItemOverrides _buildItemOverrides = new BuildItemOverrides();

        public BuildItem(string typeName, string filepath)
            : base(typeName, filepath)
        {
        }

        private void EmitXmlHeader(XmlWriter xml)
        {
            xml.WriteLine(string.Format("<{0} Include=\"{1}\">",
                ItemType,
                Name));
            xml.IncreaseIndent();
        } 

        private void EmitXmlFooter(XmlWriter xml)
        {
            xml.DecreaseIndent();
            xml.WriteLine("</" + ItemType + ">");
        }

        public sealed override void EmitProjectXml(XmlWriter xml, Project project)
        {
            EmitXmlHeader(xml);

            BuildItemOverrides finalProperties = BuildItemOverrides.Clone(_buildItemOverrides);
            finalProperties.Add(ExcludedFromBuild, "ExcludedFromBuild");

            
            
            foreach (var bc in project.BuildConfigurations)
            {
                // Flatten the array for each configuration
                PropertyCollection properties = finalProperties.FlattenForConfiguration(bc.Name);

                foreach (var propertyKVP in properties)
                {
                    xml.WriteLine(string.Format("<{0} Condition=\"'$(Configuration)|$(Platform)'=='{1}'\">{2}</{0}>",
                        propertyKVP.Key, bc.Name, propertyKVP.Value));
                }
            }

            EmitXmlFooter(xml);
        }

        public sealed override void EmitFilterXml(XmlWriter xml)
        {
            EmitXmlHeader(xml);
            if (Parent != null && Parent is FilterItem)
            {
                xml.WriteLine("<Filter>" + (Parent as FilterItem).GetFullPath() + "</Filter>");
            }
            EmitXmlFooter(xml);
        }

        public void SetOverride(string configuration, string property, string value)
        {
            _buildItemOverrides.Add(configuration, property, value);
        }
    }
}
