using Newtonsoft.Json;

namespace RSMods.Data
{
    public class GitHubAsset
    {
        [JsonProperty("name")]
        public string Name { get; set; }

        [JsonProperty("browser_download_url")]
        public string DownloadUrl { get; set; }
    }
}
