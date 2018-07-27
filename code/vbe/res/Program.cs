using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.IO;

namespace test
{
    class Program
    {
        static void Main(string[] args)
        {
            Bitmap b = new Bitmap("font.png");


            //Bitmap b2 = new Bitmap(b.Width, b.Height);

            string defs = "uint8_t font_data[95][13] = \n{\n";

            for(int i = 0; i < 95; ++i)
            {
                defs += "\t{ ";
                bool first = true;
                for(int j = 0; j < 13; ++j)
                {
                    if(first)
                        first = false;
                    else
                        defs += ", ";

                    byte val = 0;
                    for(int p = 0; p < 8; ++p)
                    {
                        Color col = b.GetPixel(i * 9 + p, j);
                        if(col.R == 0 && col.G == 0 && col.B == 0)
                        {
                            //b2.SetPixel(i * 9 + p, j, Color.Black);
                            val |= (byte)(0x80 >> p);
                        }
                    }
                    defs += "0x" + val.ToString("X2");
                }
                defs += " },\n";
            }

            defs += "};\n";

            File.WriteAllText("font.txt", defs);

            //b2.Save("test.bmp");
        }
    }
}
