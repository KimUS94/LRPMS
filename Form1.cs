using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;
using System.Diagnostics;
using System.Threading;
using System.Data.SqlClient;

namespace LRPMS_Server2
{
    public partial class Form1 : Form
    {
        static SerialPort serial = null;
        //static string constr = @"Data Source=DESKTOP-DNOPPED;Initial Catalog=hyangmi;Persist Security Info=True;User ID=sa;Password=123";
        
        public Form1()
        {
            InitializeComponent();

            cbox_brate.Items.Add("2400");
            cbox_brate.Items.Add("9600");
            cbox_brate.Items.Add("19200");
            cbox_brate.Items.Add("38000");

            //TextAdd(textBox1, "86\n");
            //TextAdd(textBox1, "87\n");
            //TextAdd(textBox1, "97\n");
            //TextAdd(textBox1, "83\n");
            //TextAdd(textBox1, "2016.06.15\n");
            //TextAdd(textBox1, "18:04:37\n");
            //TextAdd(textBox1, "18:04:52\n");
            //TextAdd(textBox1, "18:05:11\n");
            //TextAdd(textBox1, "18:05:21\n");
            //TextAdd(textBox1, "18:05:32\n");
            //TextAdd(textBox1, "18:05:47\n");
            //TextAdd(textBox1, "36°47' 54.4N, 127°4' 23.4E");
        }

        private void btn_findport_Click(object sender, EventArgs e)
        {
            serial = new SerialPort();
            cbox_port.Items.Clear();

            foreach (string a in SerialPort.GetPortNames())
            {
                cbox_port.Items.Add(a);
            }
        }

        private void btn_connect_Click(object sender, EventArgs e)
        {
            if (serial == null)
            {
                serial = new SerialPort();
                serial.PortName = cbox_port.Text;
                serial.BaudRate = Convert.ToInt32(cbox_brate.Text);

                try
                {
                    serial.Open();
                    serial.DataReceived += Serial_DataReceived;
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex.Message);
                }

                if (serial.IsOpen)
                {
                    this.Text = "연결됨";
                    btn_connect.Text = "연결 끊기";

                    tbox_log.Clear();
                    tbox_log.Text += serial.PortName + "Connect";
                }
            }
            else
            {
                serial.Close();
                tbox_log.Text += serial.PortName + "DisConnect";
                this.Text = "종료됨";
                btn_connect.Text = "연결 하기";
                serial = null;
            }
        }

        string all = string.Empty;
        string x = string.Empty;
        string y = string.Empty;
        string z = string.Empty;
        string h = string.Empty;
        string t = string.Empty;
        string d = string.Empty;
        string l = string.Empty;
        int mindex, xindex, yindex, zindex, hindex, tindex, dindex, aindex, oindex;
        string mvalue, xvalue, yvalue, zvalue, hvalue, tvalue, dvalue, avalue, ovalue;
        string U_flag = "0";
        int U_flag_Count=0;
        int x_absolute;
        int count = 0;
         


        //비동기 스레드 메소드사용.
        private delegate void SetTextDeleg(string data);
        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            if (serial == null) return;
            Thread.Sleep(10);
            string data = serial.ReadExisting();
            //비동기 스레드 메소드사용.
            //ar = this.BeginInvoke(new SetTextDeleg(si_DataReceived), new object[] { data });
            this.Invoke(new SetTextDeleg(si_DataReceived), new object[] { data });
        }

        private void si_DataReceived(string data)
        {
            all += data;
            //SqlConnection scon = new SqlConnection(constr);

            mindex = all.IndexOf('m');
            if (mindex != -1)
            {
                xindex = all.IndexOf('x', mindex);
                if (xindex != -1)
                {
                    mvalue = all.Substring(mindex + 1, xindex - (mindex + 1));
                    TextAdd(tbox_mlog, mvalue + "\n");
                    all = all.Remove(mindex, xindex - mindex);
                }
                if (Convert.ToInt16(xvalue) < 0)
                {
                    x_absolute = -Convert.ToInt16(xvalue);
                }
                else
                {
                    x_absolute = Convert.ToInt16(xvalue);
                }
            }

            xindex = all.IndexOf('x');
            if (xindex != -1)
            {
                yindex = all.IndexOf('y', xindex);
                if (yindex != -1)
                {
                    xvalue = all.Substring(xindex + 1, yindex - (xindex + 1));
                    TextAdd(tbox_xlog, xvalue + "\n");
                    all = all.Remove(xindex, yindex - xindex);
                }
                if (Convert.ToInt16(xvalue) < 0)
                {
                    x_absolute = -Convert.ToInt16(xvalue);
                }
                else
                {
                    x_absolute = Convert.ToInt16(xvalue);
                }
            }

            yindex = all.IndexOf('y');
            if (yindex != -1)
            {
                zindex = all.IndexOf('z', yindex);
                if (zindex != -1)
                {
                    yvalue = all.Substring(yindex + 1, zindex - (yindex + 1));
                    TextAdd(tbox_ylog, yvalue + "\n");
                    all = all.Remove(yindex, zindex - yindex);
                }
            }

            zindex = all.IndexOf('z');
            if (zindex != -1)
            {
                hindex = all.IndexOf('h', zindex);
                if (hindex != -1)
                {
                    zvalue = all.Substring(zindex + 1, hindex - (zindex + 1));
                    TextAdd(tbox_zlog, zvalue + "\n");
                    all = all.Remove(zindex, hindex - zindex);
                }
            }

            hindex = all.IndexOf('h');
            if (hindex != -1)
            {
                tindex = all.IndexOf('t', hindex);
                if (tindex != -1)
                {
                    hvalue = all.Substring(hindex + 1, tindex - (hindex + 1));
                    TextAdd(tbox_hlog, hvalue + "\n");
                    all = all.Remove(hindex, tindex - hindex);
                }
            }

            tindex = all.IndexOf('t');
            if (tindex != -1)
            {
                dindex = all.IndexOf('d', tindex);
                if (dindex != -1)
                {
                    tvalue = all.Substring(tindex + 1, dindex - (tindex + 1));
                    TextAdd(tbox_tlog, tvalue + "\n");
                    all = all.Remove(tindex, dindex - tindex);
                }
            }

            dindex = all.IndexOf('d');
            if (dindex != -1)
            {
                aindex = all.IndexOf('a', dindex);
                if (aindex != -1)
                {
                    dvalue = all.Substring(dindex + 1, aindex - (dindex + 1));
                    TextAdd(tbox_dlog, dvalue + "\n");
                    all = all.Remove(dindex, aindex - dindex);
                }
            }

            aindex = all.IndexOf('a');
            if (aindex != -1)
            {
                oindex = all.IndexOf('o', aindex);
                if (oindex != -1)
                {
                    avalue = all.Substring(aindex + 1, oindex - (aindex + 1));
                    TextAdd(tbox_alog, avalue + "\n");
                    all = all.Remove(aindex, oindex - aindex);
                }
            }



            oindex = all.IndexOf('o');
            if (oindex != -1)
            {
                int last = all.IndexOf('o', oindex + 1);
                if (last != -1)
                {
                    ovalue = all.Substring(oindex + 1, last - (oindex + 1));
                    TextAdd(tbox_olog, ovalue + "\n");
                    all = all.Remove(oindex, last + 1 - oindex);
                }
                count++;
            }

            //scon.Open();
            if (x_absolute < 300 && Convert.ToInt32(hvalue) < 40)
            {
                U_flag_Count++;
                if (U_flag_Count >= 10)
                {
                    U_flag = "1";
                    //응급상황시 바로 현 측정값 바로 디비 저장.
                    //맥박,위치(위,경도),응급상황 1(true)

                    //string cmdstr = string.Format("insert into M{0}_GPS(MID,lat,har,pul,state) Values('{0}','{1}','{2}','{3}','{4}')", mvalue, avalue, ovalue, hvalue,U_flag);
                    //SqlCommand scom = new SqlCommand(cmdstr, scon);

                    //scom.ExecuteNonQuery();
                }
            }
            else 
            {
                U_flag_Count = 0;
                U_flag = "0";
            }

            if (count >= 10)
            {
                //데이터 디비 저장
                //맥박,위치(위,경도),응급상황 0(false)

                //string cmdstr = string.Format("insert into M{0}_GPS(MID,lat,har,pul,state) Values('{0}','{1}','{2}','{3}','{4}')", mvalue, avalue, ovalue, hvalue, U_flag);
                //SqlCommand scom = new SqlCommand(cmdstr, scon);

                //scom.ExecuteNonQuery();
                count = 0;
            }
            //scon.Close();
        }
        delegate void MyDele(Control con, string str);
        void TextAdd(Control con, string str)
        {
            if (con.InvokeRequired)
            {
                con.Invoke(new MyDele(TextAdd), new object[] { con, str });
            }
            else
            {
                (con as TextBox).AppendText(str);
            }
        }
      
    }
}
