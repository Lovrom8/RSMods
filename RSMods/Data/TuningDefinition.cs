using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RSMods.Data
{
    public class TuningDefinitionInfo
    {
        public string UIName;
        public Dictionary<string, int> Strings;
    }

    public class TuningDefinitionList : Dictionary<string, TuningDefinitionInfo> { };
}
