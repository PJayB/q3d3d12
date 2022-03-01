using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    enum PropertyInheritance
    {
        Inherit,
        Overwrite
    };

    class BuildProperty
    {
        private char _delimiter = ';';
        private bool _prepend = false;
        private HashSet<string> _values = new HashSet<string>();
        PropertyInheritance? _inheritance = null;

        public bool Prepend
        {
            get { return _prepend; }
            set 
            { 
                if (_values.Count > 1)
                    throw new InvalidOperationException("Can't set Prepend if multiple values are store.");
                _prepend = value;
            }
        }

        public char Delimiter
        {
            get { return _delimiter; }
            set { _delimiter = value; }
        }

        public PropertyInheritance Inheritance
        {
            get { return _inheritance.GetValueOrDefault(); }
            set { _inheritance = value; }
        }

        public BuildProperty()
        {
        }

        public BuildProperty(string value)
        {
            _prepend = false;
            _values.Add(value);
        }

        public BuildProperty(string values, char delimiter, PropertyInheritance? inh = null)
        {
            _prepend = true;
            _delimiter = delimiter;
            _inheritance = inh;
            AddMany(values);
        }

        public BuildProperty(IEnumerable<string> values, char delimiter, PropertyInheritance? inh = null)
        {
            _prepend = true;
            _delimiter = delimiter;
            _inheritance = inh;
            foreach (var value in values)
            {
                AddMany(value);
            }
        }

        public void Clear()
        {
            _values.Clear();
        }

        public void Add(string value)
        {
            if (_prepend)
            {
                AddMany(value);
            }
            else
            {
                _values.Clear();
                _values.Add(value);
            }
        }

        private void AddOne(string value)
        {
            if (value != string.Empty && !_values.Contains(value))
                _values.Add(value);
        }

        private void AddMany(string str)
        {
            string[] values = str.Split(new char[] { _delimiter }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var value in values)
            {
                AddOne(value);
            }
        }

        public void Merge(BuildProperty prop)
        {
            if (prop._prepend || _prepend)
            {
                if (!_prepend && _values.Count > 0)
                    throw new InvalidOperationException("Can't assign a prepend property to a non-prepend property.");

                if (!_prepend)
                {
                    _prepend = true;
                    _delimiter = prop._delimiter;
                    _inheritance = prop._inheritance;
                }

                // Inherit the inheritance value
                if (prop._inheritance.HasValue)
                    _inheritance = prop._inheritance.Value;

                // Special case where properties were initialized from a delimited string
                if (!prop._prepend)
                {
                    AddMany(prop._values.First());
                }
                else
                {
                    foreach (var value in prop._values)
                    {
                        if (!_values.Contains(value))
                            AddOne(value);
                    }
                }
            }
            else
            {
                _values.Clear();
                AddOne(prop._values.First());
            }
        }

        public BuildProperty Clone()
        {
            return new BuildProperty(_values, _delimiter);        
        }

        public static BuildProperty Clone(BuildProperty a)
        {
            return new BuildProperty(a._values, a._delimiter);
        }

        public string ToString(char delimiter)
        {
            if (_values.Count == 0)
                return string.Empty;
            else if (!_prepend)
                return _values.First();
            else
            {
                StringBuilder str = new StringBuilder();
                var value = _values.GetEnumerator();

                // Guaranteed to be at least one as we check for 0 at the start of this function
                value.MoveNext();
                str.Append(value.Current);

                while (value.MoveNext())
                {
                    str.Append(delimiter);
                    str.Append(value.Current);
                }
                return str.ToString();
            }
        }

        public string ToString(string inheritFrom, char delimiter = ';')
        {
            string value = ToString(delimiter);
            if (_prepend && Inheritance == PropertyInheritance.Inherit)
            {
                if (value != string.Empty)
                    value += _delimiter;
                value += "%(" + inheritFrom + ")";
            }
            return value;
        }

        public override string ToString()
        {
            return ToString(_delimiter);
        }
    };
    
    class PropertyCollection : 
        IEnumerable<KeyValuePair<string, BuildProperty>>, 
        System.Collections.IEnumerable
    {
        Dictionary<string, BuildProperty> _props = new Dictionary<string, BuildProperty>();

        public PropertyCollection()
        {
        }

        //public IReadOnlyDictionary<string, BuildProperty> Items { get { return _props; } }
        IEnumerator<KeyValuePair<string, BuildProperty>> IEnumerable<KeyValuePair<string, BuildProperty>>.GetEnumerator() { return _props.GetEnumerator(); }
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() { return _props.GetEnumerator(); }

        public BuildProperty Get(string propertyName)
        {
            return _props[propertyName];
        }

        private BuildProperty GetOrCreate(string propertyName)
        {
            if (!_props.ContainsKey(propertyName))
            {
                BuildProperty prop = new BuildProperty();
                _props.Add(propertyName, prop);
                return prop;
            }
            else
            {
                return _props[propertyName];
            }
        }

        private BuildProperty GetOrNull(string propertyName)
        {
            if (_props.ContainsKey(propertyName))
                return _props[propertyName];
            else
                return null;
        }

        public void SetPrependProperty(string propertyName, bool prepend, char delimiter = ';')
        {
            BuildProperty prop = GetOrCreate(propertyName);
            prop.Prepend = prepend;
            prop.Delimiter = delimiter;
        }

        public bool IsPrependProperty(string propertyName)
        {
            BuildProperty prop = GetOrNull(propertyName);
            return (prop != null) ? prop.Prepend : false;
        }

        public bool Contains(string propertyName)
        {
            return _props.ContainsKey(propertyName);
        }

        public void Clear(string propertyName)
        {
            if (propertyName == null || propertyName == string.Empty)
                throw new NullReferenceException();

            if (_props.ContainsKey(propertyName))
            {
                _props[propertyName].Clear();
            }
        }

        public void Add(string propertyName, string nu)
        {
            if (propertyName == null || propertyName == string.Empty)
                throw new NullReferenceException();

            GetOrCreate(propertyName).Add(nu);
        }

        public void Add(string propertyName, BuildProperty prop)
        {
            if (propertyName == null || propertyName == string.Empty)
                throw new NullReferenceException();

            if (!_props.ContainsKey(propertyName))
                _props.Add(propertyName, prop);
            else
                _props[propertyName].Merge(prop);
        }

        public void Add(PropertyCollection overrider)
        {
            if (overrider == null)
                return;

            foreach (var kvp in overrider._props)
            {
                GetOrCreate(kvp.Key).Merge(kvp.Value);
            }
        }

        public string this[string propertyName]
        {
            get { return _props[propertyName].ToString(); }
        }

        public static PropertyCollection Clone(PropertyCollection a)
        {
            PropertyCollection b = new PropertyCollection();
            b.Add(a);
            return b;
        }
    }
}
