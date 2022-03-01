using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace ProjectGen
{
    static class Utils
    {
        public static string GuidString(Guid guid)
        {
            return string.Format("{{{0}}}", guid.ToString()).ToUpper();
        }

        public static string JoinToString<_ItemType, _DelimType>(IEnumerable<_ItemType> items, _DelimType delim)
        {
            HashSet<_ItemType> unique_items = new HashSet<_ItemType>();
            foreach (_ItemType i in items)
            {
                if (!unique_items.Contains(i))
                    unique_items.Add(i);
            }

            StringBuilder s = new StringBuilder();
            bool needDelim = false;
            foreach (_ItemType i in unique_items)
            {
                if (needDelim)
                    s.Append(delim.ToString());
                s.Append(i.ToString());
                needDelim = true;
            }
            return s.ToString();
        }

        public static string ConfigurationTypeStr(ConfigurationType type)
        {
            switch (type)
            {
                case ConfigurationType.Application:
                    return "Application";
                case ConfigurationType.DynamicLibrary:
                    return "DynamicLibrary";
                case ConfigurationType.StaticLibrary:
                    return "StaticLibrary";
                case ConfigurationType.NMake:
                    return "Makefile";
                default:
                    throw new Exception("Unknown configuration type: " + type.ToString());
            }
        }

        public static Regex WildcardToRegex(string wc)
        {
            wc = wc.Replace("\\", "\\\\");
            wc = wc.Replace("/", "\\/");
            wc = wc.Replace("{", "\\{");
            wc = wc.Replace("}", "\\}");
            wc = wc.Replace("$", "\\$");
            wc = wc.Replace(".", "\\.");
            wc = wc.Replace("^", "\\^");
            wc = wc.Replace("[", "\\[");
            wc = wc.Replace("]", "\\]");
            wc = wc.Replace("(", "\\(");
            wc = wc.Replace(")", "\\)");
            wc = wc.Replace("|", "\\|");
            wc = wc.Replace("?", ".");
            wc = wc.Replace("*", ".*");
            return new Regex(wc, RegexOptions.IgnoreCase);
        }
    }
}
