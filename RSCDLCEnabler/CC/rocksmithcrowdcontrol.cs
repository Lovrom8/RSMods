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
        new Effect("Drunk mode (20 seconds)", "drunk20"),
        new Effect("FYOurFC (5 seconds)", "fyourfc5"),
        new Effect("Solid Notes (random, 10 seconds)", "solidrandom10"),
        new Effect("Solid Notes (custom, 10 seconds)", "solidcustom10"),
        new Effect("Transparent notes (20 seconds)", "transparentnotes20"),
        new Effect("Remove notes (20 seconds)", "removenotes20")
    };
}