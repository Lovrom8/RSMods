using System.Collections.Generic;

namespace RSMods.Data
{
    public class TuningDefinitionInfo
    {
        public string UIName;
        public Dictionary<string, int> Strings;
    }

    public class TuningDefinitionList : Dictionary<string, TuningDefinitionInfo> { };
}
