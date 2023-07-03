using System;
using System.Collections.Generic;
using CrowdControl.Common;
using CrowdControl.Games.Packs;
using ConnectorType = CrowdControl.Common.ConnectorType;

public class Rocksmith2014 : SimpleTCPPack
{
    public override string Host => "127.0.0.1";

    public override ushort Port => 45659;

    public override ISimpleTCPPack.MessageFormat MessageFormat => ISimpleTCPPack.MessageFormat.CrowdControl;

    public Rocksmith2014(UserRecord player, Func<CrowdControlBlock, bool> responseHandler, Action<object> statusUpdateHandler) : base(player, responseHandler, statusUpdateHandler) { }

    public override Game Game => new("Rocksmith 2014", "Rocksmith2014", "PC", ConnectorType.SimpleTCPServerConnector);

    public override EffectList Effects { get; } = new Effect[]
    {
        new Effect("Rainbow Strings", "rainbowstrings")
        {
            Duration = 20,
            IsDurationEditable = false
        },
        new Effect("Drunk Mode", "drunkmode")
        {
            Duration = 10,
            IsDurationEditable = false
        },
        new Effect("Reset Combo", "fyourfc"),

        new Effect("Solid Notes (Random)", "solidrandom")
        {
            Duration = 10,
            IsDurationEditable = false
        },
        new Effect("Solid Notes (Custom)", "solidcustom")
        {
            Parameters = new ParameterDef("Color", "color",
                new Parameter("Red", "red"),
                new Parameter("Yellow", "yellow"),
                new Parameter("Blue", "blue"),
                new Parameter("Orange", "orange"),
                new Parameter("Green", "green"),
                new Parameter("Purple", "purple")
            ),
            Duration = 10,
            IsDurationEditable = false
        },
        //new Effect("Solid Notes (Custom RGB)", "solidcustomrgb")
        //{
        //    Parameters = new ParameterDef("Color", "colorRGB", ParameterBase.ParameterType.HexColor),
        //    Duration = 10,
        //    IsDurationEditable = false
        //},
        new Effect("Rainbow notes", "rainbownotes")
        {
            Duration = 20,
            IsDurationEditable = false
        },

        new Effect("Remove Notes", "removenotes")
        {
            Duration = 20,
            IsDurationEditable = false
        },
        new Effect("Remove Instrument", "removeinstrument")
        {
            Duration = 20,
            IsDurationEditable = false
        },
        
        new Effect("Change Tone to Slot 1", "changetoneslot1") { Category = "Change Tone" },
        new Effect("Change Tone to Slot 2", "changetoneslot2") { Category = "Change Tone" },
        new Effect("Change Tone to Slot 3", "changetoneslot3") { Category = "Change Tone" },
        new Effect("Change Tone to Slot 4", "changetoneslot4") { Category = "Change Tone" },
        new Effect("Shuffle Tones Every 2 Seconds", "shuffletones")
        {
            Duration = 20,
            IsDurationEditable = false
        },

        new Effect("Kill Guitar Volume", "killguitarvolume")
        {
            Duration = 10,
            IsDurationEditable = false
        },
        new Effect("Kill Music Volume", "killmusicvolume")
        {
            Duration = 10,
            IsDurationEditable = false
        },

        new Effect("Big Note Heads", "bignoteheads")
        {
            Duration = 20,
            IsDurationEditable = false
        },
        new Effect("Small Note Heads", "smallnoteheads")
        {
            Duration = 20,
            IsDurationEditable = false
        },
        // Disabled until actual string inverting is figured out (currently only flips strings, not notes) (also crash in dll)
        //new Effect("Inverted Strings", "invertedstrings")
        //{
        //    Duration = 20,
        //    IsDurationEditable = false
        //},
        new Effect("Half Scroll Speed", "halfscrollspeed")
        {
            Duration = 30,
            IsDurationEditable = false
        },
        new Effect("Double Scroll Speed", "doublescrollspeed")
        {
            Duration = 30,
            IsDurationEditable = false
        },
        new Effect("Triple Scroll Speed", "triplescrollspeed")
        {
            Duration = 30,
            IsDurationEditable = false
        },

        new Effect("Zoom Camera In", "zoomin")
        {
            Duration = 20,
            IsDurationEditable = false
        },
        new Effect("Zoom Camera Out", "zoomout")
        {
            Duration = 20,
            IsDurationEditable = false
        },

        new Effect("200% Song Speed", "turbospeed")
        {
            Duration = 10,
            IsDurationEditable = false
        },

        new Effect("Wavy Notes", "wavynotes")
        {
            Duration = 20,
            IsDurationEditable = false
        }
    };
}