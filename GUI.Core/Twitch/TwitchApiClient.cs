using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch
{
    public sealed class TwitchTokenValidation
    {
        public string ClientId { get; set; }
        public string Login { get; set; }
        public string UserId { get; set; }
        public string[] Scopes { get; set; } = new string[0];
        public int ExpiresInSeconds { get; set; }
    }

    public sealed class TwitchApiClient(HttpClient httpClient, TwitchOptions options, ITwitchClock clock = null)
    {
        private readonly HttpClient _httpClient = httpClient ?? throw new ArgumentNullException(nameof(httpClient));
        private readonly TwitchOptions _options = options ?? throw new ArgumentNullException(nameof(options));
        private readonly ITwitchClock _clock = clock ?? SystemTwitchClock.Instance;

        public async Task<TwitchTokenValidation> ValidateTokenAsync(string accessToken, CancellationToken cancellationToken)
        {
            if (string.IsNullOrWhiteSpace(accessToken))
                return null;

            using var request = new HttpRequestMessage(HttpMethod.Get, _options.TokenValidationEndpoint);
            request.Headers.Authorization = new AuthenticationHeaderValue("OAuth", accessToken);
            using HttpResponseMessage response = await _httpClient.SendAsync(request, cancellationToken).ConfigureAwait(false);
            if (response.StatusCode == HttpStatusCode.Unauthorized)
            {
                return null;
            }

            string json = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            EnsureSuccess(response, "Token validation failed.");
            JObject body = JObject.Parse(json);

            return new TwitchTokenValidation
            {
                ClientId = (string)body["client_id"] ?? string.Empty,
                Login = (string)body["login"] ?? string.Empty,
                UserId = (string)body["user_id"] ?? string.Empty,
                Scopes = ReadScopes(body["scopes"]),
                ExpiresInSeconds = (int?)body["expires_in"] ?? 0
            };
        }

        public async Task<TwitchIdentity> GetCurrentUserAsync(string accessToken, CancellationToken cancellationToken)
        {
            using var request = CreateHelixRequest(HttpMethod.Get, _options.UsersEndpoint, accessToken);
            using HttpResponseMessage response = await _httpClient.SendAsync(request, cancellationToken).ConfigureAwait(false);
            string json = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            EnsureSuccess(response, "Unable to obtain the authorized Twitch user.");
            
            JObject body = JObject.Parse(json);
            if (body["data"]?.FirstOrDefault() is not JObject user)
            {
                throw new TwitchApiException("Twitch did not return an authorized user.", response.StatusCode);
            }

            return new TwitchIdentity
            {
                UserId = (string)user["id"] ?? string.Empty,
                Login = (string)user["login"] ?? string.Empty,
                DisplayName = (string)user["display_name"] ?? (string)user["login"] ?? string.Empty
            };
        }

        public async Task<TwitchTokenSet> RefreshTokenAsync(string refreshToken, CancellationToken cancellationToken)
        {
            if (string.IsNullOrWhiteSpace(refreshToken))
                throw new TwitchApiException("Twitch authorization cannot be refreshed.", HttpStatusCode.Unauthorized);

            var fields = new Dictionary<string, string>
            {
                ["client_id"] = _options.ClientId,
                ["grant_type"] = "refresh_token",
                ["refresh_token"] = refreshToken
            };

            using var content = new FormUrlEncodedContent(fields);
            using HttpResponseMessage response = await _httpClient.PostAsync(_options.TokenEndpoint, content, cancellationToken).ConfigureAwait(false);
            string json = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            EnsureSuccess(response, "Twitch authorization could not be refreshed.");

            return ParseTokenSet(json);
        }

        public async Task CreateEventSubSubscriptionsAsync(
            string accessToken,
            string broadcasterUserId,
            string sessionId,
            CancellationToken cancellationToken)
        {
            if (string.IsNullOrWhiteSpace(broadcasterUserId))
                throw new ArgumentException("A broadcaster user ID is required.", nameof(broadcasterUserId));
            if (string.IsNullOrWhiteSpace(sessionId))
                throw new ArgumentException("An EventSub session ID is required.", nameof(sessionId));

            string[] subscriptionTypes =
            {
                "channel.cheer",
                "channel.channel_points_custom_reward_redemption.add",
                "channel.subscribe",
                "channel.subscription.message"
            };

            Task[] requests = subscriptionTypes
                .Select(type => CreateEventSubSubscriptionAsync(type, accessToken, broadcasterUserId, sessionId, cancellationToken))
                .ToArray();
            await Task.WhenAll(requests).ConfigureAwait(false);
        }

        internal static TwitchTokenSet ParseTokenSet(string json)
        {
            JObject body = JObject.Parse(json);
            string accessToken = (string)body["access_token"];
            string refreshToken = (string)body["refresh_token"];
            if (string.IsNullOrWhiteSpace(accessToken) || string.IsNullOrWhiteSpace(refreshToken))
                throw new TwitchApiException("Twitch returned an incomplete authorization response.", HttpStatusCode.BadGateway);

            int expiresIn = (int?)body["expires_in"] ?? 0;
            return new TwitchTokenSet
            {
                AccessToken = accessToken,
                RefreshToken = refreshToken,
                ExpiresAtUtc = expiresIn > 0 ? DateTimeOffset.UtcNow.AddSeconds(expiresIn) : (DateTimeOffset?)null,
                Scopes = ReadScopes(body["scope"])
            };
        }

        private async Task CreateEventSubSubscriptionAsync(
            string type,
            string accessToken,
            string broadcasterUserId,
            string sessionId,
            CancellationToken cancellationToken)
        {
            var body = new JObject
            {
                ["type"] = type,
                ["version"] = "1",
                ["condition"] = new JObject { ["broadcaster_user_id"] = broadcasterUserId },
                ["transport"] = new JObject
                {
                    ["method"] = "websocket",
                    ["session_id"] = sessionId
                }
            };

            using var request = CreateHelixRequest(HttpMethod.Post, _options.EventSubSubscriptionsEndpoint, accessToken);
            request.Content = new StringContent(body.ToString(), Encoding.UTF8, "application/json");

            using HttpResponseMessage response = await _httpClient.SendAsync(request, cancellationToken).ConfigureAwait(false);
            await response.Content.ReadAsStringAsync().ConfigureAwait(false);

            EnsureSuccess(response, $"Unable to subscribe to {type} events.");
        }

        private HttpRequestMessage CreateHelixRequest(HttpMethod method, Uri uri, string accessToken)
        {
            var request = new HttpRequestMessage(method, uri);
            request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", accessToken);
            request.Headers.Add("Client-Id", _options.ClientId);
            return request;
        }

        private static string[] ReadScopes(JToken token)
        {
            if (token == null || token.Type == JTokenType.Null)
                return new string[0];
            if (token.Type == JTokenType.Array)
                return token.Values<string>().Where(scope => !string.IsNullOrWhiteSpace(scope)).ToArray();

            string value = (string)token;
            return string.IsNullOrWhiteSpace(value)
                ? new string[0]
                : value.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
        }

        private static void EnsureSuccess(HttpResponseMessage response, string message)
        {
            if (!response.IsSuccessStatusCode)
                throw new TwitchApiException(message, response.StatusCode);
        }
    }
}
