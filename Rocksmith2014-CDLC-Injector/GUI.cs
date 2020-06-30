using System;
using System.Windows.Forms;

namespace Rocksmith2014_CDLC_Injector
{
    public partial class GUI : Form
    {
        public GUI()
        {
            InitializeComponent();
        }

        private void UseCDLCButton_Click(object sender, EventArgs e)
        {
            DLLStuff.InjectDLL(Worker.WhereIsRocksmith());   
        }
    }
}
