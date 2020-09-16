using System;
using System.Windows.Forms;
using System.IO;
using System.Linq;
using System.Drawing;
using RSMods.Data;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json;
using System.Text.RegularExpressions;
using RSMods.Util;
using System.Reflection;
using SevenZip;
using RocksmithToolkitLib.DLCPackage;
using System.Collections.Generic;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using System.Management;
using RocksmithToolkitLib.Extensions;
using RSMods.Twitch;
using System.Threading;


namespace RSMods
{
    class Dictionaries
    {
        public static Dictionary<Control, string> TooltipDictionary = new Dictionary<Control, string>() { }; // I really wish I could make this full of stuff, but `this.` and `MainForm.` don't work, so I need to call a different function `FillToolTipDictionary()` do it for me. :(

        public static Dictionary<string, string> GuitarSpeakKeyPressDictionary = new Dictionary<string, string>()
        {
            {"Delete", ReadSettings.GuitarSpeakDeleteIdentifier },
            {"Space", ReadSettings.GuitarSpeakSpaceIdentifier },
            {"Enter", ReadSettings.GuitarSpeakEnterIdentifier },
            {"Tab", ReadSettings.GuitarSpeakTabIdentifier },
            {"Page Up", ReadSettings.GuitarSpeakPGUPIdentifier },
            {"Page Down", ReadSettings.GuitarSpeakPGDNIdentifier },
            {"Up Arrow", ReadSettings.GuitarSpeakUPIdentifier },
            {"Down Arrow", ReadSettings.GuitarSpeakDNIdentifier },
            {"Escape", ReadSettings.GuitarSpeakESCIdentifier },
            {"Open Bracket", ReadSettings.GuitarSpeakOBracketIdentifier},
            {"Close Bracket", ReadSettings.GuitarSpeakCBracketIdentifier},
            {"Tilde / Tilda", ReadSettings.GuitarSpeakTildeaIdentifier},
            {"Forward Slash", ReadSettings.GuitarSpeakForSlashIdentifier},
            {"Close Guitar Speak", ReadSettings.GuitarSpeakCloseIdentifier }
        };

    //    public static Dictionary<string, string> GuitarSpeakPresetDictionary = new Dictionary<string, string>)()
    //    {
    //        {"Delete: ", GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDeleteIdentifier)},

    //    }
    ///* "Delete: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDeleteIdentifier)),
    //            "Space: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakSpaceIdentifier)),
    //            "Enter: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakEnterIdentifier)),
    //            "Tab: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTabIdentifier)),
    //            "Page Up: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGUPIdentifier)),
    //            "Page Down: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGDNIdentifier)),
    //            "Up Arrow: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakUPIdentifier)),
    //            "Down Arrow: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDNIdentifier)),
    //            "Escape: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakESCIdentifier)),
    //            "Open Bracket: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakOBracketIdentifier)),
    //            "Close Bracket: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCBracketIdentifier)),
    //            "Tilde / Tilda: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTildeaIdentifier)),
    //            "Forward Slash: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakForSlashIdentifier)),
    //            "Close Guitar Speak: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCloseIdentifier)) */
    }
}
