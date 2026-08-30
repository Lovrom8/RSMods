using Newtonsoft.Json;
using RSMods.Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.Http.Headers;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;

namespace RSMods.Util
{
    public static class GithubUpdater
    {
        private static readonly HttpClient _httpClient = new();
        private static GitHubRelease _latestRelease;

        public static async Task CallGithubAPI()
        {
            try
            {
                const string latestRelease_API = "https://api.github.com/repos/Lovrom8/RSMods/releases/latest";

                if (!_httpClient.DefaultRequestHeaders.UserAgent.Any())
                {
                    _httpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("RSMods", Application.ProductVersion));
                }

                var response = await _httpClient.GetAsync(latestRelease_API);

                if (!response.IsSuccessStatusCode)
                {
                    _latestRelease = null;
                    return;
                }

                string jsonResponse = await response.Content.ReadAsStringAsync();

                _latestRelease = JsonConvert.DeserializeObject<GitHubRelease>(jsonResponse);
            }
            catch
            {
                _latestRelease = null;
            }
        }

        public static bool IsUpdateAvailable()
        {
            if (_latestRelease == null) return false;

            return _latestRelease.Version != Application.ProductVersion;
        }

        public static string GetPatchNotes() => _latestRelease?.PatchNotes ?? string.Empty;

        public static async Task DownloadAndRunInstaller()
        {
            if (_latestRelease == null || _latestRelease.Assets.Count == 0) return;

            var asset = _latestRelease.Assets[0];
            string downloadUrl = asset.DownloadUrl;
            string fileName = asset.Name;

            try
            {
                byte[] fileBytes = await _httpClient.GetByteArrayAsync(downloadUrl);
                File.WriteAllBytes(fileName, fileBytes);

                Process.Start(fileName);
                Application.Exit();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Download failed: {ex.Message}");
            }
        }
    }
}
