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
            this.UseCDLCButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // UseCDLCButton
            // 
            this.UseCDLCButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 64F, System.Drawing.FontStyle.Bold);
            this.UseCDLCButton.Location = new System.Drawing.Point(24, 30);
            this.UseCDLCButton.Name = "UseCDLCButton";
            this.UseCDLCButton.Size = new System.Drawing.Size(746, 394);
            this.UseCDLCButton.TabIndex = 1;
            this.UseCDLCButton.Text = "Click To Enable CDLC";
            this.UseCDLCButton.UseVisualStyleBackColor = true;
            this.UseCDLCButton.Click += new System.EventHandler(this.UseCDLCButton_Click);
            // 
            // GUI
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.UseCDLCButton);
            this.Name = "GUI";
            this.Text = "Rocksmith CDLC Enabler";
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.Button UseCDLCButton;
    }
}

