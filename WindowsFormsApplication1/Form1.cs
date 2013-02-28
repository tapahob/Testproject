using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Threading;
using Wrapper;


namespace WindowsFormsApplication1
{
    public partial class Form1 : Form
    {
        private Thread oThread   = null;
        private bool alive       = true;
        private TEEngine wrapper = null;

        public Form1()
        {
            InitializeComponent();
        }

        private void panel1_Click(object sender, EventArgs e)
        {
            var args = (MouseEventArgs)e;
            if (wrapper != null)
                wrapper.Shoot(args.X, args.Y);
        }

        public void Render()
        {
            while (alive)
            {
                wrapper.Render();
                Invalidate();
                Thread.Sleep(1);
            }
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            alive = false;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            if (wrapper == null)
                wrapper = new TEEngine(panel1.Handle, panel1.Width, panel1.Height);

            if (oThread == null && wrapper != null)
            {
                oThread = new Thread(new ThreadStart(Render));
                oThread.Start();
            }
        }
    }
}
