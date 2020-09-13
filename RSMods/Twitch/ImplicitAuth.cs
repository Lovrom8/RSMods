using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading.Tasks;

namespace RSMods.Twitch
{
    class ImplicitAuth
    {
        public ImplicitAuth()
        {
            RunServer();
        }

        private static string twitchClientId = "8uq8vhme1zmy4gsjnzuq5qaca9i804"; // ClientID for our application
        private static string twitchRedirectUri = "http://localhost:1069/twitch_login";
        private static string twitchScope = "chat:read bits:read channel:read:hype_train channel:read:subscriptions channel:read:redemptions";
        private static string responseForwardContent = "<HTML><BODY> <script> location.href = window.location.href.replace(\"#\", \"/\");  </script></BODY></HTML>";
        private static string responseFinalPageContent = "<HTML><BODY><h1>thanks for the token xd</h1><br><h6><i>This response was totally not suggested by Koko</i></h6></BODY></HTML>";
        private static string url = $"https://id.twitch.tv/oauth2/authorize?response_type=token&client_id={twitchClientId}&redirect_uri=http://localhost:1069/twitch_login&scope={twitchScope}";

        public void MakeAuthRequest()
        {
            Process.Start(url); // WebBrowser control is unreliable, so we will use a regular browser to handle auth stuff for us 
        }

        public async void RunServer()
        {
            var ttask = Task.Run(() => DoWork());
            await ttask;
        }

        public void DoWork()
        {
            //Start the webserver to listen for the Twitch auth callback
            //Make sure the URL ends in a /

            try
            {
                WebServer ws = new WebServer(SendResponse, twitchRedirectUri + "/");
                ws.Run();
            }
            catch (Exception ex)
            {

                throw ex;
            }
        }

        public string SendResponse(HttpListenerRequest request)
        {
            // WARNING: dumb dumb solution, which just happens to work

            // Unforunately, unlike the OAuth client credentials flow which requires secret keys (i.e. something you wouldn't like to use in an open-source app) & returns access_token directly in the URL,
            // implicit OAuth keeps it in document.location.hash, which can't be accesed from server side (i.e. from HTTP request), so we do some crappy shizzle to read the value from server side
            // in more precise words, we redirect to a slightly modified URL from window.location.href, that actually forces a new HTTP request, and then we actually grab the access token

            string currUrl = request.Url.ToString();
            if (!currUrl.Contains("access_token"))
                return responseForwardContent;
            else
            {
                TwitchSettings.AccessToken = currUrl.Split(new string[] { "access_token=", "&" }, StringSplitOptions.None)[1];
                TwitchSettings.Authorized = true;

                Task.Run(async () => await RequestChannelID()).Wait();

                TwitchSettings.SaveSettings();

                return responseFinalPageContent;
            }
        }

        public async Task RequestChannelID() // Grabs the Channel/User ID for this auth token ( i.e. for the user that authorized the bot )
        {
            var userAPIRequest = new HttpRequestMessage
            {
                RequestUri = new Uri("https://api.twitch.tv/helix/users"),
                Method = HttpMethod.Get,
                Headers =
                {
                    { "Client-ID", twitchClientId },
                    { "Authorization", $"Bearer {TwitchSettings.AccessToken}"}
                }
            };

            var response = await TwitchSettings.client.SendAsync(userAPIRequest);
            var responseDataJson = await response.Content.ReadAsStringAsync();

            var userResponse = JsonConvert.DeserializeObject<TwitchUserResponse>(responseDataJson);

            TwitchSettings.ChannelID = userResponse.data[0].id;
            TwitchSettings.Username = userResponse.data[0].login;
        }
    }
}
