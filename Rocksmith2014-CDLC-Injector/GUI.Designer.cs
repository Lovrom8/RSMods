namespace Rocksmith2014_CDLC_Injector
{
    partial class GUI
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(GUI));
            this.UseCDLCButton = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // UseCDLCButton
            // 
            this.UseCDLCButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 64F, System.Drawing.FontStyle.Bold);
            this.UseCDLCButton.Location = new System.Drawing.Point(12, 71);
            this.UseCDLCButton.Name = "UseCDLCButton";
            this.UseCDLCButton.Size = new System.Drawing.Size(911, 443);
            this.UseCDLCButton.TabIndex = 1;
            this.UseCDLCButton.Text = "Click To Enable CDLC";
            this.UseCDLCButton.UseVisualStyleBackColor = true;
            this.UseCDLCButton.Click += new System.EventHandler(this.UseCDLCButton_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(206, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(518, 40);
            this.label1.TabIndex = 2;
            this.label1.Text = "Note: You only need to open this, and press this button once.\r\nOnce it\'s done, yo" +
    "u can delete the program from your computer.";
            // 
            // GUI
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(935, 526);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.UseCDLCButton);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "GUI";
            this.Text = "Rocksmith CDLC Enabler";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Button UseCDLCButton;
        private System.Windows.Forms.Label label1;
    }
}

