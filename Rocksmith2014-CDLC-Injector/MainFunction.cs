using System;
using System.Windows.Forms;

namespace Rocksmith2014_CDLC_Injector
{
    static class MainFunction
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new GUI());
        }
    }
}
