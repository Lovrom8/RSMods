using System;

namespace RSMods_WPF
{
    public class Utils
    {
        public static int? ConvertStringToInt(string input, int? @default)
        {
            try
            {
                return Convert.ToInt32(input);
            }
            catch
            {
                return @default;
            }
        }
    }
}
