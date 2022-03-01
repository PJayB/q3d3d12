using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace ProjectGen
{
    class GuidGenerator
    {
        private Queue<Tuple<string, Guid>> _guids = new Queue<Tuple<string, Guid>>();
        private List<Tuple<string, Guid>> _used = new List<Tuple<string, Guid>>();

        //private const string Filename = "guids.txt";

        public GuidGenerator()
        {
        }

        public GuidGenerator(string filename)
        {
            Load(filename);
        }

        public void Load(string filename)
        {
            using (StreamReader r = new StreamReader(filename))
            {
                while (!r.EndOfStream)
                {
                    string[] v = r.ReadLine().Split(new char[] { '=' });
                    Debug.Assert(v.Length == 2);

                    Guid g;
                    if (Guid.TryParse(v[1], out g))
                    {
                        _guids.Enqueue(new Tuple<string, Guid>(v[0].Trim(), g));
                    }
                }
            }
        }

        private Guid GenerateNext(string name)
        {
            if (_guids.Count > 0)
            {
                var g = _guids.Peek();
                if (g.Item1 == name)
                {
                    return _guids.Dequeue().Item2;
                }
            }

            return Guid.NewGuid();
        }

        public Guid Next(string name)
        {
            Guid next = GenerateNext(name);
            _used.Add(new Tuple<string, Guid>(name, next));
            return next;
        }

        public void Write(string filename)
        {
            using (StreamWriter w = new StreamWriter(filename))
            {
                foreach (var g in _used)
                {
                    w.WriteLine(
                        string.Format("{0} = {1}",
                            g.Item1,
                            g.Item2.ToString()));
                }
                foreach (var g in _guids)
                {
                    w.WriteLine(
                        string.Format("{0} = {1}",
                            g.Item1,
                            g.Item2.ToString()));
                }
            }
        }
    }
}
