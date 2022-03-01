using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectGen
{
    class XmlWriter : System.IO.StreamWriter
    {
        private string _indentStr = string.Empty;
        private uint _indentLevel;

        public uint IndentLevel
        {
            get { return _indentLevel; }
            set { _indentLevel = value; SetIndentLevel(value); }
        }

        public void IncreaseIndent() { IndentLevel++; }
        public void DecreaseIndent() { if (IndentLevel > 0) IndentLevel--; }

        private void SetIndentLevel(uint i)
        {
            _indentStr = string.Empty;
            while (i-- > 0)
                _indentStr += "\t";
        }

        public XmlWriter(string filename)
            : base(filename)
        {
        }

        public void OpenTag(string tag)
        {
            WriteLine(string.Format("<{0}>", tag));
            IncreaseIndent();
        }

        public void CloseTag(string tag)
        {
            DecreaseIndent();
            WriteLine(string.Format("</{0}>", tag));
        }

        public override void WriteLine(string value)
        {
            base.WriteLine(_indentStr + value.Trim());
        }

        public void EmitTag<T>(string tag, T content)
        {
            WriteLine(string.Format("<{0}>{1}</{0}>", tag, content));
        }

        public void EmitTag<T>(string tag, Nullable<T> content) where T : struct
        {
            if (content.HasValue)
            {
                WriteLine(string.Format("<{0}>{1}</{0}>", tag, content.Value));
            }
        }

        public void EmitTag(string tag, string content)
        {
            if (content != null && content != string.Empty)
            {
                WriteLine(string.Format("<{0}>{1}</{0}>", tag, content));
            }
        }

        public void EmitTag(string tag, object content)
        {
            if (content != null)
            {
                WriteLine(string.Format("<{0}>{1}</{0}>", tag, content));
            }
        }

        public void EmitTag<T>(string tag, T content, T def = default(T))
        {
            if (!content.Equals(def))
            {
                WriteLine(string.Format("<{0}>{1}</{0}>", tag, content));
            }
        }

        public void EmitTag<T>(string tag, List<T> list, char delim)
        {
            if (list.Count > 0)
            {
                WriteLine(string.Format("<{0}>{1}</{0}>", tag, Utils.JoinToString(list, delim)));
            }
        }
    }
}
