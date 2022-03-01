using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Text.RegularExpressions;
using System.Reflection;


namespace QAbiGen
{
    class Program
    {
        static string GetEnumeration(string f)
        {
            StringBuilder s = new StringBuilder();
            char p = '\0';
            for (int i = 0; i < f.Length; ++i)
            {
                char c = f[i];
                if (!char.IsLetterOrDigit(c))
                    c = '_';

                bool barrier = char.IsUpper(c) && !char.IsUpper(p);
                if (i > 0 && barrier && p != '_')
                {
                    s.Append('_');
                }
                s.Append(char.ToUpper(c));
                p = c;
            }
            return s.ToString();
        }

        class Config
        {
            public string InputFile { get; private set; }
            public string OutputPath = null;
            public List<string> SearchPaths = new List<string>();
            public string VM = string.Empty;
            public bool IsVM = false;
            public bool Force = false;
            public bool Debug = false;

            public Config()
            {
                InputFile = null;
            }

            public Config(string[] args)
            {
                InputFile = null;
                Parse(args);
            }

            public void Parse(string[] args)
            {
                for (int i = 0; i < args.Length; ++i)
                {
                    string arg = args[i].Trim();
                    if (arg.Length < 2 || (arg[0] != '-' && arg[0] != '/'))
                    {
                        // Must be the input filename
                        if (InputFile != null)
                        {
                            throw new InvalidDataException("Filename already specified: " + InputFile);
                        }
                        InputFile = arg;
                        continue;
                    }

                    arg = arg.Substring(1);

                    if (arg.ToLower() == "f")
                    {
                        Force = true;
                        continue;
                    }

                    if (arg == "d")
                    {
                        Debug = true;
                        continue;
                    }

                    if (i == args.Length - 1)
                    {
                        throw new InvalidDataException(arg + " expects an argument.");
                    }

                    string value = args[++i].Trim();

                    switch (arg)
                    {
                        case "I":
                            SearchPaths.Add(value);
                            break;
                        case "o":
                            OutputPath = value;
                            break;
                        case "vm":
                            VM = value;
                            IsVM = true;
                            break;
                        default:
                            throw new InvalidDataException("Unknown switch: " + arg);
                    }
                }
            }

            public string FindFileInSearchPaths(string file, string relativeTo = null)
            {
                // Directory of current file
                if (relativeTo != null && relativeTo != string.Empty)
                {
                    relativeTo = Path.GetDirectoryName(Path.GetFullPath(relativeTo));
                    string path = Path.Combine(relativeTo, file);
                    if (File.Exists(path))
                        return path;
                }
                
                // Current search directory
                if (File.Exists(file)) return file;

                foreach (string searchPath in SearchPaths)
                {
                    string path = Path.Combine(searchPath, file);
                    if (File.Exists(path))
                        return path;
                }

                throw new FileNotFoundException("Could not find " + file, file);
            }
        }

        class ParseException : Exception
        {
            public ParseException(string filename, int line, string msg)
                : base(string.Format("{0}({1}): error: {2}", filename, line, msg))
            {

            }
        }

        static string GetIncludeFile(string directive)
        {
            Regex r = new Regex("#include *\"(.*?)\"", RegexOptions.Singleline);
            Match m = r.Match(directive);

            if (m.Groups.Count != 2)
            {
                throw new Exception();
            }

            return m.Groups[1].Value;
        }

        public enum DataType
        {
            Void,
            Int,
            IntPtr,
            Float,
            //VarArg, // Not supported by Q3.
            Pointer,
            Array
        };

        public struct OutputFilenames
        {
            public string Shared;
            public string Publics;
            public string JumpDefs;
            public string JumpTable;
            public string Calls; // Either VM or syscalls
        }

        public struct ReturnValue
        {
            public string DefinitionString;
            public DataType UnderlyingType;
        };

        public struct ArgumentValue
        {
            public string DefinitionString;
            public string TypeString;
            public string NameString;
            public string LiteralString;
            public DataType UnderlyingType;
        };

        class ABI
        {
            public ReturnValue ReturnType;
            public string InternalName = string.Empty;
            public string ExternalName = string.Empty;
            public string Enumeration = string.Empty;
            public List<ArgumentValue> Arguments = new List<ArgumentValue>();
            public bool HasArguments = false;
        }

        class ParseState
        {
            public string VM;
            public string Module = string.Empty;
            public HashSet<string> FluffWords = new HashSet<string>();
            public Dictionary<string, DataType> DataTypeWords = new Dictionary<string,DataType>();
            public List<ABI> ABIs = new List<ABI>();
            public HashSet<string> ABINames = new HashSet<string>();

            public ParseState()
            {
                DataTypeWords["int"] = DataType.Int;
                DataTypeWords["intptr"] = DataType.IntPtr;
                DataTypeWords["float"] = DataType.Float;
                DataTypeWords["pointer"] = DataType.Pointer;
                DataTypeWords["array"] = DataType.Array;
            }

            public int ComputeChecksum()
            {
                StringBuilder s = new StringBuilder();

                foreach (ABI abi in ABIs)
                {
                    s.Append((int)abi.ReturnType.UnderlyingType);
                    s.Append(abi.ExternalName);
                    foreach (ArgumentValue arg in abi.Arguments)
                    {
                        s.Append((int)arg.UnderlyingType);
                    }
                }

                s.Append(ABIs.Count);
                return s.ToString().GetHashCode();
            }
        }

        class TokenReader
        {
            public string Filename { get; private set; }
            public string Content { get; private set; }
            public int Line { get; private set; }
            public int Cursor { get { return _cursor; } }
            public int Length { get { return Content.Length; } }
            public int Remaining { get { return Content.Length - _cursor; } }

            private int _cursor = 0;

            public TokenReader(string contentName, string content)
            {
                Filename = contentName;
                Content = content;
                Line = 1;
                skipWhitespace();
            }

            public bool EndOfStream
            {
                get { return _cursor >= Content.Length; }
            }




            private void throwEOS()
            {
                throw new ParseException(Filename, Line, "Unexpected end-of-stream");
            }

            private bool canPeek(int offset)
            {
                return _cursor + offset < Content.Length;
            }

            private char peek(int offset = 0)
            {
                if (_cursor + offset < 0)
                    throwEOS();
                if (_cursor + offset >= Content.Length)
                    throwEOS();
                    
                return Content[_cursor + offset];
            }

            private char next()
            {
                if (EndOfStream)
                    throwEOS();
                if (Content[_cursor] == '\n')
                    Line++;
                return Content[_cursor++];
            }

            private static bool isWhitespace(char c, bool includeNewLines = false)
            {
                if (!includeNewLines && c == '\n') return false;
                return char.IsWhiteSpace(c);
            }

            private void skipWhitespace(bool includingNewLines = true)
            {
                while (!EndOfStream && isWhitespace(peek(), includingNewLines)) { next(); }
            }



            public void SkipWhitespace(bool includingNewLines = true)
            {
                skipWhitespace(includingNewLines);
            }

            public string ReadToNext(char c, bool includeEnd = true, bool consumeEnd = true)
            {
                StringBuilder s = new StringBuilder();
                while (!EndOfStream && peek() != c)
                {
                    s.Append(next());
                }

                if (!EndOfStream && includeEnd)
                {
                    s.Append(c);
                }

                if (!EndOfStream && (consumeEnd || includeEnd))
                {
                    next();
                }

                skipWhitespace(); 
                return s.ToString();
            }

            public void SkipToNext(char c, bool inclusive)
            {
                while (!EndOfStream && peek() != c) next();
                if (inclusive && !EndOfStream) next();

                skipWhitespace();
            }

            public char PeekChar(int offset = 0)
            {
                if (!canPeek(offset))
                    throwEOS();

                return peek(offset);
            }

            public char ReadChar(bool skipNewLines = false)
            {
                if (EndOfStream)
                    throwEOS();

                char c = next();
                skipWhitespace(skipNewLines);
                return c;
            }

            public void Expect(char c, bool skipNewLines = true)
            {
                if (ReadChar(skipNewLines) != c)
                {
                    throw new ParseException(Filename, Line, "expected '" + c + "'");
                }
            }

            public string PeekToken()
            {
                // If we're at the end of the stream then exit
                if (EndOfStream)
                    throwEOS();

                StringBuilder s = new StringBuilder();

                // Read the token
                for (int cur = 0; canPeek(cur); ++cur)
                {
                    char read = peek(cur);
                    if (isWhitespace(read))
                        break;

                    if (!char.IsLetterOrDigit(read) && read != '_')
                    {
                        if (s.Length > 0)
                            break;

                        // Symbols get read one at a time.
                        s.Append(read);
                        break;
                    }

                    s.Append(read);
                }

                // Return the token
                return s.ToString();
            }

            public string ReadToken(bool skipNewLines = true)
            {
                // If we're at the end of the stream then exit
                if (EndOfStream)
                    throwEOS();

                StringBuilder s = new StringBuilder();

                // Read the token
                while (!EndOfStream && !isWhitespace(peek()))
                {
                    char read = peek();
                    if (!char.IsLetterOrDigit(read) && read != '_')
                    {
                        if (s.Length > 0)
                            break;

                        // Symbols get read one at a time.
                        s.Append(next());
                        break;
                    }

                    s.Append(next());
                }

                skipWhitespace(skipNewLines);

                // Return the token
                return s.ToString();
            }

            public void Expect(string s, bool skipNewLines = true)
            {
                if (ReadToken(skipNewLines) != s)
                {
                    throw new ParseException(Filename, Line, "expected '" + s + "'");
                }
            }
        };

        class Parser
        {
            public string Filename;
            public ParseState State;
            public Config Cfg;
            private TokenReader tokens;
            private HashSet<string> included = new HashSet<string>();

            void Throw(string msg)
            {
                throw new ParseException(Filename, tokens.Line, msg);
            }

            static string FirstWord(string s)
            {
                int i;
                for (i = 0; i < s.Length && char.IsLetter(s[i]); ++i) { }
                return s.Substring(0, i);
            }

            void CheckIdentifier(string s)
            {
                if (s.Length == 0) Throw("expected identifier");
                if (!char.IsLetter(s[0]) && s[0] != '_' && s[0] != '*') Throw("expected identifier");

                for (int i = 1; i < s.Length; ++i)
                {
                    if (!char.IsLetterOrDigit(s[i]) && s[i] != '_' && s[i] != '*')
                        Throw("expected identifier");
                }
            }

            void CheckLiteral(string s)
            {
                if (s.Length == 0) Throw("expected literal");
                if (!char.IsLetterOrDigit(s[0]) && s[0] != '.' && s[0] != '_') Throw("expected literal");

                for (int i = 1; i < s.Length; ++i)
                {
                    if (!char.IsLetterOrDigit(s[i]) && s[i] != '_' && s[i] != '.')
                        Throw("expected literal");
                }
            }

            static string[] SplitOnWhitespace(string p)
            {
                return p.Split(new char[] { ' ', '\t', '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            }

            void DoImport(string filename)
            {
                //if (!args.StartsWith("\"") || !args.EndsWith("\""))
                //    Throw("expected filename string");
                //
                //string filename = args.Substring(1, args.Length - 2);

                filename = Cfg.FindFileInSearchPaths(filename, Filename);

                if (included.Contains(filename))
                    return;

                included.Add(filename);

                Parser p = new Parser();
                p.Parse(filename, Cfg, State);
            }

            void DoTypedef(TokenReader tokens)
            {
                List<string> typeTokens = new List<string>();
                while (tokens.PeekChar() != ';')
                {
                    typeTokens.Add(tokens.ReadToken());
                }
                tokens.Expect(';');

                if (typeTokens.Count < 2)
                    Throw("expected: identifier");

                DataType dt = GetDataTypeFromHintString(typeTokens.ToArray(), true);

                string defType = typeTokens[typeTokens.Count - 1];

                CheckIdentifier(defType);

                State.DataTypeWords[defType] = dt;

#if DEBUG
                Console.WriteLine("Typedef " + defType + " => " + State.DataTypeWords[defType]);
#endif
            }

            bool IsIdentifierChar(char c)
            {
                return char.IsLetterOrDigit(c) || c == '_';
            }

            void GetFunctionParts(TokenReader tokens, out string returnString, out string nameString, out string[] argsStrings)
            {
                // Read identifiers until we hit a bracket
                List<string> prologue = new List<string>();
                while (!tokens.EndOfStream)
                {
                    string t = tokens.ReadToken();
                    if (t == "(")
                        break;

                    CheckIdentifier(t);
                    prologue.Add(t);
                }

                if (prologue.Count < 2)
                    Throw("expected '<return type> <function name>(<arguments>);'");
                
                // The name is the last token
                nameString = prologue[prologue.Count - 1];

                // The return string is everything before that
                string ret = prologue[0];
                for (int i = 1; i < prologue.Count - 1; ++i)
                {
                    ret += ' ' + prologue[i];
                }
                returnString = ret;

                List<string> args = new List<string>();
                List<string> argDefs = new List<string>();
                int bracketStack = 1;
                while (!tokens.EndOfStream && tokens.PeekChar() != ';')
                {
                    string t = tokens.ReadToken();

                    if (t == "(") bracketStack++;
                    if (t == ")") bracketStack--;

                    if ((t == ")" && bracketStack == 0) || t == ",")
                    {
                        // Commit the arguments
                        if (args.Count == 0)
                            Throw("expected arguments");
                        string argType = args[0];
                        for (int i = 1; i < args.Count; ++i)
                        {
                            argType += ' ' + args[i];
                        }

                        argDefs.Add(argType);
                        args.Clear();
                        continue;
                    }

                    args.Add(t);
                }

                tokens.Expect(';');

                argsStrings = argDefs.ToArray();
            }

            DataType GetDataTypeFromHintString(string[] p, bool ignoreLast)
            {
                // If it's got a star in it, it's a pointer, no matter what
                foreach (string s in p)
                {
                    if (s.Contains('*'))
                        return DataType.Pointer;
                }

                if (p.Length == 1 && p[0] == "void")
                    return DataType.Void;

                if (p.Length == 1 && p[0] == "...")
                    Throw("variable arguments (...) are not supported by QABIGEN");
                    // return DataType.VarArg;

                bool couldBeInt = false;
                DataType? type = null;

                int processUpTo = p.Length;

                for (int i = 0; i < processUpTo; ++i)
                {
                    if (p[i] == "[")
                    {
                        return DataType.Array;
                    }
                    if (p[i] == "=")
                    {
                        processUpTo = i;
                        break;
                    }
                }

                if (ignoreLast)
                    processUpTo--;

                if (processUpTo <= 0)
                    Throw("malformed argument definition: " + string.Join(" ", p));

                for (int i = 0; i < processUpTo; ++i)
                {
                    string c = p[i];
                    switch (c)
                    {
                        case "const":
                            break;
                        case "signed":
                        case "unsigned":
                        case "long":
                            couldBeInt = true;
                            break;
                        case "short":
                            Throw("'short' is not suppported.");
                            break;
                        default:
                            if (State.DataTypeWords.ContainsKey(c))
                                type = State.DataTypeWords[c];
                            else
                                Throw("Unknown data type: " + c);
                            break;
                    }
                }

                if (!type.HasValue && couldBeInt)
                    type = DataType.Int;

                if (!type.HasValue)
                    Throw("Unknown data type: " + string.Join(" ", p));

                return type.Value;
            }

            ReturnValue GetReturnValue(string ret)
            {
                // Split the string
                string[] p = SplitOnWhitespace(ret);

                ReturnValue r = new ReturnValue();
                r.DefinitionString = ret;
                r.UnderlyingType = GetDataTypeFromHintString(p, false);
                return r;
            }

            string JoinStrings(string[] p, int count)
            {
                StringBuilder s = new StringBuilder();
                for (int i = 0; i < count; ++i)
                {
                    if (i > 0) s.Append(" ");
                    s.Append(p[i]);
                }
                return s.ToString();
            }

            ArgumentValue GetArgumentValue(string ret)
            {
                string literal = null;

                int eq = ret.IndexOf('=');
                if (eq != -1)
                {
                    literal = ret.Substring(eq + 1).Trim();
                    CheckLiteral(literal);

                    ret = ret.Substring(0, eq);
                }

                // Split the string
                string[] p = SplitOnWhitespace(ret);

                ArgumentValue a = new ArgumentValue();
                a.UnderlyingType = GetDataTypeFromHintString(p, true);

                // Check for array
                int typePos = p.Length - 1;
                if (a.UnderlyingType == DataType.Array)
                {
                    int arrayPos = typePos;
                    while (arrayPos > 0 && p[arrayPos] != "[")
                    {
                        arrayPos--;
                    }
                    if (p[arrayPos] == "[" && arrayPos > 0)
                        typePos = arrayPos-1;
                }

                a.DefinitionString = ret;
                a.TypeString = JoinStrings(p, typePos).Trim();
                a.NameString = p[typePos];
                a.LiteralString = literal;
                return a;
            }

            ABI DoFunctionDeclaration(TokenReader tokens, string alias)
            {
                string name, ret;
                string[] args;
                GetFunctionParts(tokens, out ret, out name, out args);

                CheckIdentifier(name);
                CheckIdentifier(alias);

                ABI abi = new ABI();
                abi.ReturnType = GetReturnValue(ret);
                abi.InternalName = name;
                abi.ExternalName = (alias == null || alias == string.Empty) ? name : alias;
                abi.Enumeration = GetEnumeration("QABI_" + State.Module + '_' + abi.ExternalName);

                foreach (string arg in args)
                {
                    var av = GetArgumentValue(arg);
                    CheckIdentifier(av.NameString);
                    abi.Arguments.Add(av);
                }

                if (abi.Arguments.Count == 0)
                    abi.Arguments.Add(new ArgumentValue()
                    {
                        DefinitionString = "void",
                        TypeString = "void",
                        NameString = string.Empty,
                        UnderlyingType = DataType.Void
                    });

                if (abi.Arguments.Count > 1 || abi.Arguments[0].UnderlyingType != DataType.Void)
                {
                    abi.HasArguments = true;
                }

#if DEBUG
                Console.WriteLine(abi.Enumeration);
                Console.WriteLine(abi.ReturnType.DefinitionString + " => " + abi.ReturnType.UnderlyingType);
                Console.WriteLine(name + "(");
                for (int i = 0; i < abi.Arguments.Count; ++i)
                {
                    ArgumentValue a = abi.Arguments[i];
                    if (i < args.Length-1)
                        Console.WriteLine("    " + a.DefinitionString + " => " + a.UnderlyingType + ",");
                    else
                        Console.WriteLine("    " + a.DefinitionString + " => " + a.UnderlyingType + ");");
                }
#endif

                return abi;
            }

            void DoABI(TokenReader tokens)
            {
                string alias = null;

                if (tokens.PeekChar() == '(')
                {
                    tokens.ReadToken();
                    alias = tokens.ReadToken();
                    tokens.Expect(')');
                    CheckIdentifier(alias);
                }

                // Process the C-style function definition
                ABI abi = DoFunctionDeclaration(tokens, alias);

                State.ABIs.Add(abi);
            }

            void ProcessChunk(TokenReader tokens)
            {                
                // Get the first word
                string action = tokens.ReadToken();
                if (action.Length == 0)
                    return;

                switch (action)
                {
                    //case "import":
                    //    DoImport(parms);
                    //    break;
                    case "typedef":
                        DoTypedef(tokens);
                        break;
                    case "abi":
                        DoABI(tokens);
                        break;
                    default:
                        throw new ParseException(Filename, tokens.Line, "Unknown syntax: " + action);
                }
            }

            public void DoDirective(TokenReader tokens)
            {
                tokens.Expect('#', false);
                tokens.Expect("include", false);
                tokens.Expect('\"', false);
                string file = tokens.ReadToNext('\"', false, false);
                tokens.Expect('\"', false);
                tokens.Expect('\n', false);
                tokens.SkipWhitespace(true);

                DoImport(file);
            }

            public void Parse(string filename, Config cfg, ParseState state)
            {
                Cfg = cfg;
                State = state;
                Filename = filename;
                included.Clear();

                string content = string.Empty;
                using (StreamReader r = new StreamReader(filename))
                {
                    content = r.ReadToEnd();
                }

                tokens = new TokenReader(filename, content);

                while (!tokens.EndOfStream)
                {
                    // If the chunk starts with //, skip to the next newline
                    if (tokens.Remaining > 1 && tokens.PeekChar() == '/' && tokens.PeekChar(1) == '/')
                    {
                        tokens.SkipToNext('\n', true);
                        continue;
                    }

                    // If the chunk starts with #, do a directive
                    if (tokens.PeekChar() == '#')
                    {
                        // Process the directive.
                        DoDirective(tokens);
                        continue;
                    }

                    ProcessChunk(tokens);
                }
            }
        }

        static void HeaderGuard(StreamWriter w, string module, string type)
        {
            string guardMacro = "__" + GetEnumeration("QABI_" + module + '_' + type) + "_H__";
            w.WriteLine("// Generated by QABIGEN. Do not modify!");
            w.WriteLine("#ifndef " + guardMacro);
            w.WriteLine("#define " + guardMacro);
            w.WriteLine();
        }

        static void FooterGuard(StreamWriter w)
        {
            w.WriteLine();
            w.WriteLine("#endif");
        }

        static void GenerateShared(ParseState state, string file)
        {
            using (StreamWriter w = new StreamWriter(file))
            {
                HeaderGuard(w, state.Module, "SHARED");

                string checksum = string.Format("0x{0:X}", state.ComputeChecksum());

                w.WriteLine("#define " + GetEnumeration("_QABI_" + state.Module + "_CHECKSUM") + " " + checksum);
                w.WriteLine();

                w.WriteLine("enum {");
                foreach (ABI abi in state.ABIs)
                {
                    w.WriteLine("    " + abi.Enumeration + ",");
                }
                w.WriteLine("    " + GetEnumeration("_QABI_" + state.Module + "_ABI_COUNT"));
                w.WriteLine("};");

                FooterGuard(w);
            }
        }

        static void GeneratePublics(ParseState state, string file)
        {
            using (StreamWriter w = new StreamWriter(file))
            {
                HeaderGuard(w, state.Module, "PUBLIC");

                foreach (ABI abi in state.ABIs)
                {
                    w.Write("extern ");
                    w.Write(abi.ReturnType.DefinitionString);
                    w.Write(" ");
                    w.Write(abi.ExternalName);
                    w.WriteLine("(");
                    bool exp = false;
                    for (int i = 0; i < abi.Arguments.Count; ++i)
                    {
                        ArgumentValue arg = abi.Arguments[i];
                        if (arg.LiteralString == null)
                        {
                            if (exp) w.WriteLine(",");
                            w.Write("    ");
                            w.Write(arg.DefinitionString);
                            exp = true;
                        }
                    }
                    w.WriteLine(");");
                    w.WriteLine();
                }

                FooterGuard(w);
            }
        }

        static void PrintArgs(ABI abi, StringBuilder body)
        {
            if (abi.Arguments.Count == 1 && abi.Arguments[0].UnderlyingType == DataType.Void)
                return;

            foreach (ArgumentValue arg in abi.Arguments)
            {
                //if (arg.UnderlyingType == DataType.VarArg)
                //    break;
                if (arg.LiteralString == null)
                {
                    body.AppendLine(",");
                    body.Append("        ");
                    body.Append(arg.NameString);
                }
            }
        }

        static char DataTypeCharKey(DataType dt)
        {
            switch (dt)
            {
                case DataType.Float: return 'f';
                case DataType.Int: return 'i';
                case DataType.IntPtr:
                case DataType.Pointer: 
                case DataType.Array: return 'p';
                default: throw new Exception("Unknown data type: " + dt);
            }
        }

        static string DataTypeKeyword(DataType dt)
        {
            switch (dt)
            {
                case DataType.Float: return "float";
                case DataType.Int: return "int";
                case DataType.Pointer:
                case DataType.Array:
                case DataType.IntPtr: return "size_t";
                default: throw new Exception("Unknown data type: " + dt);
            }
        }

        static void ExportFunctionCall(ABI abi, StreamWriter w, string func, string args)
        {
            w.Write(abi.ReturnType.DefinitionString);
            w.Write(" ");
            w.Write(abi.ExternalName);
            w.Write("( ");

            bool exp = false;
            int argCount = 1;
            for (int i = 0; i < abi.Arguments.Count; ++i)
            {
                if (abi.Arguments[i].LiteralString == null)
                {
                    if (exp) w.Write(", ");
                    w.Write(abi.Arguments[i].DefinitionString);
                    exp = true;

                    argCount++;
                }
            }

            w.WriteLine(" ) {");

            if (abi.ReturnType.UnderlyingType != DataType.Void)
                w.WriteLine("    vmArg_t r = {0};");

            w.WriteLine("    vmArg_t args[" + argCount + "];");
            w.WriteLine("    args[0].p = (size_t) " + abi.Enumeration + ";");
            if (abi.HasArguments)
            {
                int k = 1;
                for (int i = 0; i < abi.Arguments.Count; ++i)
                {
                    ArgumentValue arg = abi.Arguments[i];
                    if (arg.LiteralString == null)
                    {
                        if (arg.UnderlyingType == DataType.Pointer || arg.UnderlyingType == DataType.Array)
                            w.WriteLine("    args[" + (k++) + "].p = (size_t) " + arg.NameString + ";");
                        else
                        {
                            w.WriteLine("    args[" + k + "].p = 0;");
                            w.WriteLine("    args[" + (k++) + "]." + DataTypeCharKey(arg.UnderlyingType) + " = " + arg.NameString + ";");
                        }
                    }
                }
            }
            w.Write("    ");

            if (abi.ReturnType.UnderlyingType != DataType.Void)
                w.Write("r.p = ");

            w.Write(func);
            w.Write("( ");
            if (args != null && args != string.Empty)
                w.Write(args + ", ");
            w.WriteLine("args );");

            if (abi.ReturnType.UnderlyingType != DataType.Void)
            {
                w.WriteLine("    return (" + abi.ReturnType.DefinitionString + ") r." + DataTypeCharKey(abi.ReturnType.UnderlyingType) + ';');
            }
            w.WriteLine("}");
            w.WriteLine();
        }

        static void GenerateVMCalls(ParseState state, string file)
        {
            using (StreamWriter w = new StreamWriter(file))
            {
                w.WriteLine("// Generated by QABIGEN. Do not modify!");
                w.WriteLine();
                w.WriteLine("#ifdef " + GetEnumeration(state.Module));
                w.WriteLine("#    error Do not include this inside the VM.");
                w.WriteLine("#endif");
                w.WriteLine();
                w.WriteLine("extern vm_t* cgvm;");
                w.WriteLine();

                foreach (ABI abi in state.ABIs)
                {
                    ExportFunctionCall(abi, w, "VM_CallA", state.VM);
                }
            }
        }

        static void GenerateSysCalls(ParseState state, string file)
        {
            using (StreamWriter w = new StreamWriter(file))
            {
                w.WriteLine("// Generated by QABIGEN. Do not modify!");
                w.WriteLine();
                w.WriteLine("#ifndef " + GetEnumeration(state.Module));
                w.WriteLine("#    error Include this inside the VM.");
                w.WriteLine("#endif");
                w.WriteLine();

                foreach (ABI abi in state.ABIs)
                {
                    ExportFunctionCall(abi, w, "syscallA", null);
                }
            }
        }

        static void GenerateJumpTableFunctions(ParseState state, string file, bool convertPtrs)
        {
            using (StreamWriter w = new StreamWriter(file))
            {
                w.WriteLine("// Generated by QABIGEN. Do not modify!");
                w.WriteLine();
                w.WriteLine("#pragma warning(push)");
                w.WriteLine("#pragma warning(error: 4013)");
                w.WriteLine();

                foreach (ABI abi in state.ABIs)
                {
                    w.Write("static size_t QDECL __QABI_");
                    w.Write(abi.ExternalName);
                    w.WriteLine("( vmArg_t* args ) {");
                    w.WriteLine("    vmArg_t r = { 0 };");
                    w.Write("    ");

                    switch (abi.ReturnType.UnderlyingType)
                    {
                        case DataType.Float:
                        case DataType.Int:
                        case DataType.IntPtr:
                        case DataType.Pointer:
                        case DataType.Array:
                            w.Write("r." + DataTypeCharKey(abi.ReturnType.UnderlyingType) + " = (" + DataTypeKeyword(abi.ReturnType.UnderlyingType) + ") ");
                            break;
                        default:
                            break;
                    }

                    w.Write(abi.InternalName);

                    if (abi.HasArguments)
                    {
                        w.WriteLine("( ");

                        int k = 0;
                        for (int i = 0; i < abi.Arguments.Count; ++i)
                        {
                            ArgumentValue arg = abi.Arguments[i];
                            if (i > 0)
                                w.WriteLine(",");

                            w.Write("        ");

                            if (arg.LiteralString != null)
                            {
                                w.Write(arg.LiteralString);
                                continue;
                            }

                            if (arg.UnderlyingType != DataType.Array)
                            {
                                w.Write("(");
                                w.Write(arg.TypeString);
                                w.Write(") ");
                            }

                            switch (arg.UnderlyingType)
                            {
                                case DataType.Float:
                                case DataType.Int:
                                case DataType.IntPtr:
                                    w.Write("args[" + (k++) + "]." + DataTypeCharKey(arg.UnderlyingType));
                                    break;
                                case DataType.Pointer:
                                case DataType.Array:
                                    if (convertPtrs)
                                        w.Write("VM_ArgPtr( (size_t) ");
                                    w.Write("QABI_PTR(args[" + (k++) + "].p)");
                                    if (convertPtrs)
                                        w.Write(" )");
                                    break;
                                default:
                                    throw new Exception("Unknown data type: " + arg.UnderlyingType);
                            }
                        }

                        w.Write(" )");
                    }
                    else
                    {
                        w.Write("()");
                    }

                    w.WriteLine(";");

                    w.WriteLine("    return r.p;");
                    w.WriteLine("}");

                    w.WriteLine();
                }

                w.WriteLine("#pragma warning(pop)");
                w.WriteLine(); 
            }
        }

        static void GenerateJumpTable(ParseState state, string file)
        {
            using (StreamWriter w = new StreamWriter(file))
            {
                w.WriteLine("// Generated by QABIGEN. Do not modify!");
                w.WriteLine();

                foreach (ABI abi in state.ABIs)
                {
                    w.Write("__QABI_");
                    w.Write(abi.ExternalName);
                    w.WriteLine(",");
                }
            }
        }

        static OutputFilenames GenerateOutputFilenames(string outputPath, string module, bool vm)
        {
            OutputFilenames f = new OutputFilenames();

            f.Shared = Path.Combine(outputPath, module + ".shared.h");
            f.Publics = Path.Combine(outputPath, module + ".public.h");
            f.JumpDefs = Path.Combine(outputPath, module + ".jumpdefs.h");
            f.JumpTable = Path.Combine(outputPath, module + ".jumptable.h");

            if (vm)
            {
                f.Calls = Path.Combine(outputPath, module + ".vmcalls.h");
            }
            else
            {
                f.Calls = Path.Combine(outputPath, module + ".syscalls.h");
            }

            return f;
        }

        public static string AssemblyPath
        {
            get
            {
                string codeBase = Assembly.GetExecutingAssembly().CodeBase;
                UriBuilder uri = new UriBuilder(codeBase);
                string path = Uri.UnescapeDataString(uri.Path);
                return Path.GetFullPath(path);
            }
        }

        static DateTime ScanInputFileWriteTime(Config cfg, string file, string relativeTo)
        {
            file = cfg.FindFileInSearchPaths(file, relativeTo);

            DateTime mt = File.GetLastWriteTimeUtc(file);

            try
            {
                DateTime tdt = File.GetLastWriteTimeUtc(AssemblyPath);
                if (tdt > mt)
                    mt = tdt;
            }
            catch (Exception) { }

            // TODO: cache the contents somewhere so we're not reading the file twice?
            using (StreamReader r = new StreamReader(file))
            {
                while (!r.EndOfStream)
                {
                    string line = r.ReadLine().Trim();
                    if (line.StartsWith("#include"))
                    {
                        try
                        {
                            string inc = GetIncludeFile(line);
                            DateTime imt = ScanInputFileWriteTime(cfg, inc, file);

                            // Keep the newest time
                            if (imt > mt)
                                mt = imt;
                        }
                        catch (Exception)
                        { 
                            // Ignore
                        }
                    }
                }
            }

            return mt;
        }

        static bool FileIsOutOfDate(string file, DateTime wrt)
        {
            if (!File.Exists(file)) return true;
            return File.GetLastWriteTimeUtc(file) < wrt;
        }

        static void MainProtected(Config cfg)
        {
            ParseState state = new ParseState();

            state.Module = Path.GetFileNameWithoutExtension(cfg.InputFile);

            string inputfile = cfg.FindFileInSearchPaths(cfg.InputFile);

            if (cfg.OutputPath == null)
            {
                cfg.OutputPath = Path.GetDirectoryName(inputfile);
                if (!Directory.Exists(cfg.OutputPath))
                {
                    throw new DirectoryNotFoundException("Couldn't find output path: " + cfg.OutputPath);
                }
            }

            OutputFilenames ofs = GenerateOutputFilenames(cfg.OutputPath, state.Module, cfg.IsVM);

            // Check if the outputs are missing or out of date compared to the input
            // TODO: scan the files for dependencies
            DateTime imTime = ScanInputFileWriteTime(cfg, inputfile, null);
            if (!cfg.Force && 
                !FileIsOutOfDate(ofs.Shared, imTime) &&
                !FileIsOutOfDate(ofs.Publics, imTime) &&
                !FileIsOutOfDate(ofs.JumpTable, imTime) &&
                !FileIsOutOfDate(ofs.JumpDefs, imTime) &&
                !FileIsOutOfDate(ofs.Calls, imTime))
            {
                Console.WriteLine("QABI: " + state.Module + " up-to-date.");
                return;
            }

            Parser p = new Parser();
            p.Parse(
                inputfile, 
                cfg, state);

            GenerateShared(state, ofs.Shared);
            GeneratePublics(state, ofs.Publics);
            GenerateJumpTableFunctions(state, ofs.JumpDefs, !cfg.IsVM);
            GenerateJumpTable(state, ofs.JumpTable);

            if (cfg.IsVM)
            {
                state.VM = cfg.VM;
                GenerateVMCalls(state, ofs.Calls);
            }
            else
            {
                state.VM = cfg.VM;
                GenerateSysCalls(state, ofs.Calls);
            }

            Console.WriteLine("QABI: " + cfg.InputFile + " OK.");
        }

        static void Main(string[] args)
        {
            try
            {
                Config config = new Config(args);

                if (config.InputFile == null || config.InputFile.Length == 0)
                    throw new InvalidDataException("Filename required.");

                MainProtected(config);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);

                // Reconstruct the calling arguments
                Console.Write("Arguments:");
                foreach (string arg in args)
                {
                    if (arg.Contains(' '))
                        Console.Write(" \"" + arg + "\"");
                    else
                        Console.Write(" " + arg);
                }
                Console.WriteLine();
                Environment.Exit(1);
            }
        }
    }
}
