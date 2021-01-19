using System.Collections.Generic;

namespace RSMods.Twitch.EffectServer
{
    public class Response
    {
        public int id { get; set; }
        public string message { get; set; }
        public int status { get; set; }
    }

    public class Request
    {
        public string code { get; set; }
        public int id { get; set; }
        public int type { get; set; }
        public string viewer { get; set; }
        public List<object> parameters { get; set; }
    }
}
