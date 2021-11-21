using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace RSMods_WPF.Pages.ModPages
{
    public abstract class ModPage : Page
    {
        abstract public void LoadSettings();
    }
}
