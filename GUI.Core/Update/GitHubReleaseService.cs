using System;
using System.Net.Http;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace RSMods.Core.Update
{
    /// <summary>Pulls a comparable version out of a GitHub release tag/name.</summary>
    public static class ReleaseVersioning
    {
        // First dotted run in a tag like "RSModsInstaller-v1.2.8.4_OnCommit" -> "1.2.8.4".
        private static readonly Regex VersionPattern = new(@"\d+(?:\.\d+){1,3}", RegexOptions.Compiled);

        public static bool TryExtractVersion(string text, out Version version)
        {
            version = null;
            if (string.IsNullOrWhiteSpace(text))
                return false;

            Match match = VersionPattern.Match(text);
            return match.Success && Version.TryParse(match.Value, out version);
        }
    }

    /// <summary>The latest published release as shown on the Home tab.</summary>
    public sealed class ReleaseInfo
    {
        /// <summary>Release title, falling back to the tag when GitHub leaves the name blank.</summary>
        public string Title { get; set; } = string.Empty;
        public string TagName { get; set; } = string.Empty;
        public DateTimeOffset? PublishedAt { get; set; }
        /// <summary>Release body, in GitHub-flavoured Markdown.</summary>
        public string Notes { get; set; } = string.Empty;
        /// <summary>Human-facing release page (not the API URL).</summary>
        public string HtmlUrl { get; set; } = string.Empty;
    }

    public interface IGitHubReleaseService
    {
        Task<ReleaseInfo> GetLatestReleaseAsync(CancellationToken cancellationToken = default);
    }

    /// <summary>Reads the newest RSMods release from the public GitHub API. No auth: subject to GitHub's
    /// unauthenticated rate limit, which is ample for a once-per-launch check.</summary>
    public sealed class GitHubReleaseService : IGitHubReleaseService
    {
        // "latest" excludes drafts and pre-releases, matching what users actually download.
        private const string LatestReleaseEndpoint = "https://api.github.com/repos/Lovrom8/RSMods/releases/latest";

        private readonly HttpClient _httpClient;

        public GitHubReleaseService(HttpClient httpClient) =>
            _httpClient = httpClient ?? throw new ArgumentNullException(nameof(httpClient));

        public async Task<ReleaseInfo> GetLatestReleaseAsync(CancellationToken cancellationToken = default)
        {
            using var request = new HttpRequestMessage(HttpMethod.Get, LatestReleaseEndpoint);
            // GitHub rejects requests without a User-Agent; the Accept header pins the v3 JSON schema.
            request.Headers.UserAgent.ParseAdd("RSMods-Configurator");
            request.Headers.Accept.ParseAdd("application/vnd.github+json");

            using HttpResponseMessage response =
                await _httpClient.SendAsync(request, cancellationToken).ConfigureAwait(false);
            response.EnsureSuccessStatusCode();

            string json = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            JObject body = JObject.Parse(json);

            string tag = (string)body["tag_name"] ?? string.Empty;
            string name = (string)body["name"];

            return new ReleaseInfo
            {
                TagName = tag,
                Title = string.IsNullOrWhiteSpace(name) ? tag : name,
                Notes = (string)body["body"] ?? string.Empty,
                HtmlUrl = (string)body["html_url"] ?? string.Empty,
                PublishedAt = (DateTimeOffset?)body["published_at"],
            };
        }
    }
}
