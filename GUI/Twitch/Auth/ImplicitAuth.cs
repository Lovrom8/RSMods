using Newtonsoft.Json;
using RSMods.Util;
using System;
using System.Diagnostics;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;

namespace RSMods.Twitch
{
    class ImplicitAuth
    {
        public ImplicitAuth()
        {
            RunServer();
        }

        private static readonly string twitchClientId = "8uq8vhme1zmy4gsjnzuq5qaca9i804"; // ClientID for our application
        private static readonly string twitchRedirectUri = "http://localhost:1069/twitch_login";
        private static readonly string twitchScope = "chat:read%20bits:read%20channel:read:hype_train%20channel_subscriptions%20channel:read:redemptions"; // When you send spaces to a command prompt without qoutation marks, stuff can go wrong very quickly
        private static readonly string responseForwardContent = "<HTML><BODY> <script> location.href = window.location.href.replace(\"#\", \"/\");  </script></BODY></HTML>";
        private static readonly string responseFinalPageContent = "<HTML><BODY><h1>thanks for the token xd</h1><br><h6><i>This response was totally not suggested by Koko</i></h6></BODY></HTML>";
        private static readonly string responseToldToFOff = "<HTML><BODY><h1>You decided to say no, so we say no back</h1><br><h6>We didn't think you'd press \"Cancel\" since why would you press the button, and not trust us?</h6></BODY></HTML>";
        private static readonly string url = $"https://id.twitch.tv/oauth2/authorize?response_type=token&client_id={twitchClientId}&redirect_uri=http://localhost:1069/twitch_login&scope={twitchScope}";
        private WebServer webServer = null;

        public void MakeAuthRequest(bool startMinimized = false)
        {
            string browser = GenUtil.GetDefaultBrowser(url);

            ProcessStartInfo startInfo = new ProcessStartInfo(browser);

            if (startMinimized)
                startInfo.WindowStyle = ProcessWindowStyle.Minimized;

            if(!browser.Contains("edge")) // Edge is procotol activated, not a regular exe
                startInfo.Arguments = url;

            Process.Start(startInfo);

            // WebBrowser control is unreliable, so we will use a regular browser to handle auth stuff for us
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
                if (webServer != null)
                { // To avoid the user from spamming the button and crashing the program.
                    webServer.Stop();
                    webServer = null;
                }
                webServer = new WebServer(SendResponse, twitchRedirectUri + "/");
                webServer.Run();
            }
            catch (Exception ex)
            {
                if ((HttpListenerException)ex != ex)
                    throw ex;
            }
        }

        public string SendResponse(HttpListenerRequest request)
        {
            // WARNING: dumb dumb solution, which just happens to work

            // Unforunately, unlike the OAuth client credentials flow which requires secret keys (i.e. something you wouldn't like to use in an open-source app) & returns access_token directly in the URL,
            // implicit OAuth keeps it in document.location.hash, which can't be accesed from server side (i.e. from HTTP request), so we do some crappy shizzle to read that value, but from server side!
            // In more precise words, we redirect to a slightly modified URL from window.location.href, and that actually forces a new HTTP request and in the end we actually grab the access token

            string currUrl = request.Url.ToString();
            if (!currUrl.Contains("access_token"))
            {
                if (currUrl.Contains("denied"))
                    return responseToldToFOff;
                else
                    return responseForwardContent;
            }
            else
            {
                TwitchSettings.Get.AccessToken = currUrl.Split(new string[] { "access_token=", "&" }, StringSplitOptions.None)[1];
                TwitchSettings.Get.Authorized = true;
                TwitchSettings.Get.Reauthorized = true;

                Task.Run(async () => await RequestChannelID()).Wait();
                TwitchSettings.Get.SaveSettings();

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
                    { "Authorization", $"Bearer {TwitchSettings.Get.AccessToken}"}
                }
            };

            var response = await TwitchSettings.client.SendAsync(userAPIRequest);
            var responseDataJson = await response.Content.ReadAsStringAsync();

            var userResponse = JsonConvert.DeserializeObject<TwitchUserResponse>(responseDataJson);

            TwitchSettings.Get.ChannelID = userResponse.data[0].id;
            TwitchSettings.Get.Username = userResponse.data[0].login;
        }
    }
}
