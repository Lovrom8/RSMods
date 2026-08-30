using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch
{
    public sealed class TwitchDeviceAuthorization
    {
        internal TwitchDeviceAuthorization(
            string deviceCode,
            string userCode,
            Uri verificationUri,
            DateTimeOffset expiresAtUtc,
            TimeSpan pollingInterval)
        {
            DeviceCode = deviceCode;
            UserCode = userCode;
            VerificationUri = verificationUri;
            ExpiresAtUtc = expiresAtUtc;
            PollingInterval = pollingInterval;
        }

        internal string DeviceCode { get; }
        public string UserCode { get; }
        public Uri VerificationUri { get; }
        public DateTimeOffset ExpiresAtUtc { get; }
        public TimeSpan PollingInterval { get; }
    }

    public sealed class TwitchAuthorizationException(string message) : Exception(message)
    {
    }

    public sealed class TwitchAuthService(HttpClient httpClient, TwitchOptions options, ITwitchClock clock = null)
    {
        private readonly HttpClient _httpClient = httpClient ?? throw new ArgumentNullException(nameof(httpClient));
        private readonly TwitchOptions _options = options ?? throw new ArgumentNullException(nameof(options));
        private readonly ITwitchClock _clock = clock ?? SystemTwitchClock.Instance;

        public async Task<TwitchDeviceAuthorization> StartDeviceAuthorizationAsync(CancellationToken cancellationToken)
        {
            var fields = new Dictionary<string, string>
            {
                ["client_id"] = _options.ClientId,
                ["scopes"] = string.Join(" ", _options.Scopes)
            };

            using var content = new FormUrlEncodedContent(fields);
            using HttpResponseMessage response = await _httpClient.PostAsync(_options.DeviceAuthorizationEndpoint, content, cancellationToken).ConfigureAwait(false);
            string json = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            if (!response.IsSuccessStatusCode)
            {
                throw new TwitchApiException("Unable to start Twitch device authorization.", response.StatusCode);
            }

            JObject body = JObject.Parse(json);
            string deviceCode = (string)body["device_code"];
            string userCode = (string)body["user_code"];
            string verificationUri = (string)body["verification_uri"];
            int expiresIn = (int?)body["expires_in"] ?? 0;
            int interval = (int?)body["interval"] ?? 5;
            if (string.IsNullOrWhiteSpace(deviceCode) || string.IsNullOrWhiteSpace(userCode) ||
                !Uri.TryCreate(verificationUri, UriKind.Absolute, out Uri uri) || expiresIn <= 0)
            {
                throw new TwitchAuthorizationException("Twitch returned an incomplete device authorization response.");
            }

            return new TwitchDeviceAuthorization(
                deviceCode,
                userCode,
                uri,
                _clock.UtcNow.AddSeconds(expiresIn),
                TimeSpan.FromSeconds(Math.Max(1, interval)));
        }

        public async Task<TwitchTokenSet> PollForAuthorizationAsync(
            TwitchDeviceAuthorization authorization,
            CancellationToken cancellationToken)
        {
            if (authorization == null)
                throw new ArgumentNullException(nameof(authorization));

            while (_clock.UtcNow < authorization.ExpiresAtUtc)
            {
                await _clock.DelayAsync(authorization.PollingInterval, cancellationToken).ConfigureAwait(false);
                var fields = new Dictionary<string, string>
                {
                    ["client_id"] = _options.ClientId,
                    ["scopes"] = string.Join(" ", _options.Scopes),
                    ["device_code"] = authorization.DeviceCode,
                    ["grant_type"] = "urn:ietf:params:oauth:grant-type:device_code"
                };

                using (var content = new FormUrlEncodedContent(fields))
                using (HttpResponseMessage response = await _httpClient.PostAsync(_options.TokenEndpoint, content, cancellationToken).ConfigureAwait(false))
                {
                    string json = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
                    if (response.IsSuccessStatusCode)
                    {
                        TwitchTokenSet tokens = TwitchApiClient.ParseTokenSet(json);
                        tokens.ClientId = _options.ClientId;
                        int expiresIn = (int?)JObject.Parse(json)["expires_in"] ?? 0;
                        tokens.ExpiresAtUtc = expiresIn > 0 ? _clock.UtcNow.AddSeconds(expiresIn) : (DateTimeOffset?)null;
                        return tokens;
                    }

                    string error = ReadError(json);
                    if (response.StatusCode == HttpStatusCode.BadRequest &&
                        string.Equals(error, "authorization_pending", StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }

                    throw new TwitchAuthorizationException(
                        string.Equals(error, "invalid device code", StringComparison.OrdinalIgnoreCase)
                            ? "The Twitch device code is no longer valid."
                            : "Twitch authorization was denied or could not be completed.");
                }
            }

            throw new TwitchAuthorizationException("The Twitch device authorization code expired.");
        }

        private static string ReadError(string json)
        {
            try
            {
                return (string)JObject.Parse(json)["message"] ?? string.Empty;
            }
            catch
            {
                return string.Empty;
            }
        }
    }
}
