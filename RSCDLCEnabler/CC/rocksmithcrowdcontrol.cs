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
        new Effect("Rainbow strings", "rainbow", new[] { "timer" }),
        new Effect("Drunk mode", "drunk", new[] { "timer" }),
        new Effect("FYOurFC", "fyourfc", new[] { "timer" }),
        new Effect("Solid Notes (random)", "solidrandom", new[] { "timer" }),

        new Effect("Solid Notes (custom)", "solidcustom", new[] { "timer", "color" }),
        new Effect("Solid Notes (custom rgb)", "solidcustomrgb", new[] { "timer", "sliderred", "slidergreen", "sliderblue"}),

        new Effect("Red", "red", ItemKind.Usable, "color"),
        new Effect("Yellow", "yellow", ItemKind.Usable, "color"),
        new Effect("Blue", "blue", ItemKind.Usable, "color"),
        new Effect("Orange", "orange", ItemKind.Usable, "color"),
        new Effect("Green", "green", ItemKind.Usable, "color"),
        new Effect("Purple", "purple", ItemKind.Usable, "color"),

        new Effect("Transparent notes", "transparentnotes", new[] { "timer" }),

        new Effect("Remove notes", "removenotes", new[] { "timer" }),
        new Effect("Remove instrument", "removeinstrument", new[] { "timer" }),

        new Effect("Change tone", "changetone", ItemKind.Folder),
        new Effect("Change tone to slot 1", "changetoneslot1", "changetone"),
        new Effect("Change tone to slot 2", "changetoneslot2", "changetone"),
        new Effect("Change tone to slot 3", "changetoneslot3", "changetone"),
        new Effect("Change tone to slot 4", "changetoneslot4", "changetone"),
        new Effect("Shuffle tones every 2 seconds", "shuffletones", new[] { "timer" }),

        new Effect("Kill guitar volume", "killguitarvolume", new[] { "timer" }),
        new Effect("Kill music volume", "killmusicvolume", new[] { "timer" }),

        new Effect("Big note heads", "bignoteheads", new[] { "timer" }),
        new Effect("Small note heads", "smallnoteheads", new[] { "timer" }),
        new Effect("Inverted strings", "invertedstrings", new[] { "timer" }),

        new Effect("Half song speed", "halfsongspeed", new[] { "timer" }),
        new Effect("Double song speed", "doublesongspeed", new[] { "timer" }),

        new Effect("Zoom camera in", "zoomin", new[] { "timer" }) 
    };

    public override List<ItemType> ItemTypes => new List<ItemType>(new[]
    {
        new ItemType("Color", "color", ItemType.Subtype.ItemList),
        new ItemType("Red", "sliderred", ItemType.Subtype.Slider, "{\"min\":0,\"max\":255}"),
        new ItemType("Green", "slidergreen", ItemType.Subtype.Slider, "{\"min\":0,\"max\":255}"),
        new ItemType("Blue", "sliderblue", ItemType.Subtype.Slider, "{\"min\":0,\"max\":255}"),
        new ItemType("Duration (sec)", "timer", ItemType.Subtype.Slider, "{\"min\":0,\"max\":60000}")
    });
}