using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace dumpconverter
{
    class Program
    {
        const byte DUMP_FREE = 0x01;
        const byte DUMP_STACK_PAGE = 0x02;
        const byte DUMP_SIZE_2M = 0x04;
        const byte DUMP_SIZE_1G = 0x08;
        const byte DUMP_RESERVED = 0x10;
        const byte DUMP_AUX = 0x20;

        const int WIDTH = 512;

        static void Main(string[] args)
        {
            DirectoryInfo dumpsDir = new DirectoryInfo("R:\\dumps");
            foreach(var file in dumpsDir.GetFiles("*.png"))
                file.Delete();

            DirectoryInfo dir = new DirectoryInfo("R:\\");
            foreach(var file in dir.GetFiles("*.bin"))
            {
                byte[] dumpFile = File.ReadAllBytes(file.FullName);

                ulong pageCount = BitConverter.ToUInt64(dumpFile, 0);

                Bitmap bmp = new Bitmap(WIDTH, (int)((pageCount + WIDTH - 1) / WIDTH));
                using(Graphics g = Graphics.FromImage(bmp))
                    g.Clear(Color.Black);

                for(int i = 0; i < (int)pageCount; ++i)
                {
                    int row = i / WIDTH;
                    int col = i % WIDTH;

                    byte data = dumpFile[8 + i];

                    if((data & DUMP_STACK_PAGE) != 0)
                        bmp.SetPixel(col, row, Color.Blue);
                    else if((data & DUMP_FREE) != 0)
                    {
                        if((data & DUMP_SIZE_1G) != 0)
                            bmp.SetPixel(col, row, Color.Yellow);
                        else if((data & DUMP_SIZE_2M) != 0)
                            bmp.SetPixel(col, row, Color.Orange);
                        else
                            bmp.SetPixel(col, row, Color.Red);
                    }
                    else if((data & DUMP_RESERVED) != 0)
                        bmp.SetPixel(col, row, Color.Cyan);
                    else if((data & DUMP_AUX) != 0)
                        bmp.SetPixel(col, row, Color.Green);
                }

                bmp.Save("R:\\dumps\\" + Path.GetFileNameWithoutExtension(file.Name) + ".png", System.Drawing.Imaging.ImageFormat.Png);
            }
        }
    }
}
