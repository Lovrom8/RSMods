using System;
using System.Windows.Forms;
using System.Security.Principal;
using RSMods.Core;
using RSMods.WinForms;

namespace RSMods
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            try
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                AppServices.Initialize(new WinFormsDialogService(), new WinFormsAppEnvironment());
                Application.Run(new MainForm());
            }
            catch (Exception ex)
            {
                MessageBox.Show($"{ex.Message} {ex}", "Error");
            }
        }
    }
}
