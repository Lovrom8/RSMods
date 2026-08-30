using System;
using System.Net;

namespace RSMods.Twitch
{
    public sealed class TwitchApiException(string message, HttpStatusCode statusCode) : Exception(message)
    {
        public HttpStatusCode StatusCode { get; } = statusCode;
    }
}
