using Newtonsoft.Json;
using System.Collections.Generic;

namespace RSMods.Data
{
    public class GitHubRelease
    {
        [JsonProperty("name")]
        public string Name { get; set; }

        [JsonProperty("body")]
        public string PatchNotes { get; set; }

        [JsonProperty("assets")]
        public List<GitHubAsset> Assets { get; set; }

        public string Version => Name?.Replace("RSModsInstaller-v", "") ?? string.Empty;
    }
}
