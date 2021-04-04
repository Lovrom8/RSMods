using System;
using System.Windows.Forms;
using System.Security.Principal;

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

                bool isAdmin = new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);

                if (!isAdmin)
                    Application.Run(new MainForm());
                else
                    Application.Run(new AdminForm());
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message +  " " + ex.ToString(), "Error");
            }
        }
    }
}
