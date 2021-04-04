
namespace RSMods
{
    partial class AdminForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AdminForm));
            this.checkBox_SpeedUpEnumeration = new System.Windows.Forms.CheckBox();
            this.label_AdminOnly = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // checkBox_SpeedUpEnumeration
            // 
            this.checkBox_SpeedUpEnumeration.AutoSize = true;
            this.checkBox_SpeedUpEnumeration.Location = new System.Drawing.Point(38, 40);
            this.checkBox_SpeedUpEnumeration.Name = "checkBox_SpeedUpEnumeration";
            this.checkBox_SpeedUpEnumeration.Size = new System.Drawing.Size(136, 17);
            this.checkBox_SpeedUpEnumeration.TabIndex = 0;
            this.checkBox_SpeedUpEnumeration.Text = "Speed Up Enumeration";
            this.checkBox_SpeedUpEnumeration.UseVisualStyleBackColor = true;
            this.checkBox_SpeedUpEnumeration.CheckedChanged += new System.EventHandler(this.Save_SpeedUpEnumeration);
            // 
            // label_AdminOnly
            // 
            this.label_AdminOnly.AutoSize = true;
            this.label_AdminOnly.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F);
            this.label_AdminOnly.Location = new System.Drawing.Point(187, 170);
            this.label_AdminOnly.Name = "label_AdminOnly";
            this.label_AdminOnly.Size = new System.Drawing.Size(402, 80);
            this.label_AdminOnly.TabIndex = 1;
            this.label_AdminOnly.Text = "This section is for mods that require admin access.\r\nIf you opened this yourself," +
    " please close it.\r\nYou should only use this page if RSMods sent you here.\r\nPleas" +
    "e close when you\'re done.\r\n";
            // 
            // AdminForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Azure;
            this.ClientSize = new System.Drawing.Size(601, 255);
            this.Controls.Add(this.label_AdminOnly);
            this.Controls.Add(this.checkBox_SpeedUpEnumeration);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "AdminForm";
            this.Text = "RSMods Admin Form";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.CheckBox checkBox_SpeedUpEnumeration;
        private System.Windows.Forms.Label label_AdminOnly;
    }
}