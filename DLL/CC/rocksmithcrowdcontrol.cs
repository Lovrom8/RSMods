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

    public override Game Game => new Game(99, "Rocksmith 2014", "Rocksmith2014", "PC", ConnectorType.SimpleTCPConnector);

    public override List<Effect> Effects => new List<Effect>
    {
        new Effect("Rainbow strings (20 seconds)", "rainbowstrings"),
        new Effect("Drunk mode (10 seconds)", "drunkmode"),
        new Effect("FYOurFC (reset combo)", "fyourfc"),

        new Effect("Solid Notes (random, 10 seconds)", "solidrandom"),
        new Effect("Solid Notes (custom, 10 seconds)", "solidcustom", new[] { "color" }),
        new Effect("Rainbow notes (20 seconds)", "rainbownotes"),

        new Effect("Red", "red", ItemKind.Usable, "color"),
        new Effect("Yellow", "yellow", ItemKind.Usable, "color"),
        new Effect("Blue", "blue", ItemKind.Usable, "color"),
        new Effect("Orange", "orange", ItemKind.Usable, "color"),
        new Effect("Green", "green", ItemKind.Usable, "color"),
        new Effect("Purple", "purple", ItemKind.Usable, "color"),

        new Effect("Remove notes (20 seconds)", "removenotes"),
        new Effect("Remove instrument (20 seconds)", "removeinstrument"),

        new Effect("Change tone", "changetone", ItemKind.Folder),
        new Effect("Change tone to slot 1", "changetoneslot1", "changetone"),
        new Effect("Change tone to slot 2", "changetoneslot2", "changetone"),
        new Effect("Change tone to slot 3", "changetoneslot3", "changetone"),
        new Effect("Change tone to slot 4", "changetoneslot4", "changetone"),
        new Effect("Shuffle tones every 2 seconds (20 seconds)", "shuffletones"),

        new Effect("Kill guitar volume (10 seconds)", "killguitarvolume"),
        new Effect("Kill music volume (10 seconds)", "killmusicvolume"),

        new Effect("Big note heads (20 seconds)", "bignoteheads"),
        new Effect("Small note heads (20 seconds)", "smallnoteheads"),
        // Disabled until actual string inverting is figured out (currently only flips strings, not notes) (also crash in dll)
        //new Effect("Inverted strings (20 seconds)", "invertedstrings"),

        new Effect("Half scroll speed (30 seconds)", "halfscrollspeed"),
        new Effect("Double scroll speed (30 seconds)", "doublescrollspeed"),
        new Effect("Triple scroll speed (30 seconds)", "triplescrollspeed"),

        new Effect("Zoom camera in (20 seconds)", "zoomin"),
        new Effect("Zoom camera out (20 seconds)", "zoomout")
    };

    public override List<ItemType> ItemTypes => new List<ItemType>(new[]
    {
        new ItemType("Color", "color", ItemType.Subtype.ItemList)
    });
}
