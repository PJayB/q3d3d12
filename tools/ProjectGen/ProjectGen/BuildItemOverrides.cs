using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace ProjectGen
{
    class BuildItemOverrides
    {
        struct Override
        {
            public string ConfigurationPattern;
            public string PropertyName;
            public string Value;
        };

        List<Override> _overrides = new List<Override>();
                
        public BuildItemOverrides()
        {

        }

        public void Add(string configurationPattern, string property, string value)
        {
            _overrides.Add(new Override()
            {
                ConfigurationPattern = configurationPattern,
                PropertyName = property,
                Value = value
            });
        }

        public void Add<T>(ConfigurationSpecific<T> item, string propertyName)
        {
            foreach (var kvp in item.Values)
            {
                Add(kvp.Key, propertyName, kvp.Value.ToString());
            }
        }

        public static BuildItemOverrides Clone(BuildItemOverrides original)
        {
            BuildItemOverrides output = new BuildItemOverrides();
            foreach (var i in original._overrides)
            {
                output._overrides.Add(i);
            }
            return output;
        }

        public PropertyCollection FlattenForConfiguration(string configuration)
        {
            PropertyCollection props = new PropertyCollection();

            foreach (var ov in _overrides)
            {
                Regex regex = Utils.WildcardToRegex(ov.ConfigurationPattern);
                if (regex.IsMatch(configuration))
                {
                    props.Add(ov.PropertyName, ov.Value);
                }
            }

            return props;
        }
    }

    class ConfigurationSpecific<T>
    {
        private Dictionary<string, T> _values = new Dictionary<string, T>();

        public ConfigurationSpecific()
        {
        }

        public IReadOnlyDictionary<string, T> Values { get { return _values; } }

        public bool Contains(string buildConfiguration)
        {
            return _values.ContainsKey(buildConfiguration);
        }

        public T Get(string buildConfiguration)
        {
            return _values[buildConfiguration];
        }

        public void Set(string buildConfiguration, T value)
        {
            _values[buildConfiguration] = value;
        }

        public bool IsEmpty
        {
            get { return _values.Count == 0; }
        }
    }
}
