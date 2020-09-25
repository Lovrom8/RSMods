using System;
using System.Collections.Generic;
using CrowdControl.Common;
using CrowdControl.Games.Packs;
using ConnectorType = CrowdControl.Common.ConnectorType;

public class Rocksmith2014 : SimpleTCPPack
{
    public override string Host => "127.0.0.1";

    public override ushort Port => 45659;

    public Rocksmith2014(IPlayer player, Func<CrowdControlBlock, bool> responseHandler, Action<object> statusUpdateHandler) : base(player, responseHandler, statusUpdateHandler) { }

    public override Game Game => new Game(1234, "Rocksmith 2014", "rocksmith2014", "PC", ConnectorType.SimpleTCPConnector);

    public override List<Effect> Effects => new List<Effect>
    {
        new Effect("Rainbow strings (60 seconds)", "rainbow60"),
    };
}