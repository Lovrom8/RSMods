using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Web;
using System.Windows.Forms;

namespace RSMods.Twitch
{
    class PubSubImplicit
    {
        public PubSubImplicit()
        {
            RunServer();
        }

        private static readonly string twitchClientId = "6j445pcbepas4mtlzgfiset3u6symq";
        private static readonly string twitchRedirectUri = "http://localhost:1069/twitch_login";
        private static readonly string twitchScope = "chat:read bits:read channel:read:hype_train channel:read:subscriptions channel:read:redemptions";
        private static readonly string responseForwardContent =  "<HTML><BODY> <script> location.href = window.location.href.replace(\"#\", \"/\");  </script></BODY></HTML>";
        private static readonly string responseFinalPageContent = "<HTML><BODY><h1>thanks for the token xd</h1><br><h6><i>This response was totally not suggested by Koko</i></h6></BODY></HTML>";
        private static readonly string url = String.Format("https://id.twitch.tv/oauth2/authorize?response_type=token&client_id={0}&redirect_uri=http://localhost:1069/twitch_login&scope={1}", twitchClientId, twitchScope);
        private static string accessToken;

        public void MakeRequest()
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
                accessToken = currUrl.Split(new string[] { "access_token=", "&" }, StringSplitOptions.None)[1];

                return responseFinalPageContent;
            }
        }

    }
}
