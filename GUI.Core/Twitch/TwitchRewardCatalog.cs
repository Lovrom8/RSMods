using System.Collections.Generic;

namespace RSMods.Twitch
{
    public static class TwitchRewardCatalog
    {
        public static List<TwitchReward> CreateDefaults()
        {
            return
            [
                new TwitchReward("Rainbow Strings", "Your strings will continuously shift colors like a rainbow", "rainbowstrings"),
                new TwitchReward("Rainbow Notes", "The notes on screen will continously shift colors like a rainbow", "rainbownotes"),
                new TwitchReward("Remove Notes", "Make noteheads not show at all", "removenotes"),
                new TwitchReward("Transparent Notes", "Make noteheads transparent", "transparentnotes"),
                new TwitchReward("Solid color notes", "Make all notes have the same color", "solidnotes"),
                new TwitchReward("Drunk Mode", "Have the background freak out because you're drunk", "drunkmode"),
                new TwitchReward("F Your FC", "Makes your current note streak go to 0", "fyourfc"),
                new TwitchReward("Shuffle your tones", "Switch your tone every few seconds", "shuffletones"),
                new TwitchReward("Change tone - slot 1", "Switch your tone to slot 1", "changetoneslot1"),
                new TwitchReward("Change tone - slot 2", "Switch your tone to slot 2", "changetoneslot2"),
                new TwitchReward("Change tone - slot 3", "Switch your tone to slot 3", "changetoneslot3"),
                new TwitchReward("Change tone - slot 4", "Switch your tone to slot 4", "changetoneslot4"),
                new TwitchReward("Remove Instrument", "Removes your instrument (strings, fretbars and headstock)", "removeinstrument"),
                new TwitchReward("Invert Strings", "Inverts your guitar or bass strings but keeps the notes in the same location", "invertedstrings"),
                new TwitchReward("Kill guitar volume", "Kills volume of your guitar", "killguitarvolume"),
                new TwitchReward("Kill music volume", "Kills volume of the music", "killmusicvolume"),
                new TwitchReward("Big note heads", "Makes all note heads big", "bignoteheads"),
                new TwitchReward("Small note heads", "Makes all note heads tiny", "smallnoteheads"),
                new TwitchReward("Half scroll speed", "Makes scroll speed half (ramp-up effect - not instant!)", "halfscrollspeed"),
                new TwitchReward("Double scroll speed", "Makes scroll speed double (ramp-up effect)", "doublescrollspeed"),
                new TwitchReward("Triple scroll speed", "Makes scroll speed triple (ramp-up effect)", "triplescrollspeed"),
                new TwitchReward("Zoom camera in", "Zooms in the main camera", "zoomin"),
                new TwitchReward("TurboSpeed", "EXPERIMENTAL: turn up song speed, a lot.", "turbospeed")
            ];
        }
    }
}
