using System.Collections.Generic;

namespace RSMods.SetAndForget.Models
{
    public class TuningDefinitionInfo
    {
        public string UIName;
        public Dictionary<string, int> Strings;
    }

    public class TuningDefinitionList : Dictionary<string, TuningDefinitionInfo> { };
}
