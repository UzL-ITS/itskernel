using System;

namespace dumpconverter
{
    internal class Program
    {
        private const byte DUMP_FREE = 0x01;
        private const byte DUMP_STACK_PAGE = 0x02;
        private const byte DUMP_SIZE_2M = 0x04;
        private const byte DUMP_SIZE_1G = 0x08;
        private const byte DUMP_RESERVED = 0x10;
        private const byte DUMP_AUX = 0x20;
        private const int WIDTH = 512;

        private static void Main(string[] args)
        {
            Dump dump = new Dump("R:\\itskernel\\out\\dump.bin");

            foreach(var entry in dump.MemoryMap)
                Console.WriteLine("- " + entry.AddressStart.ToString("X16") + "    " + entry.AddressEnd.ToString("X16") + "    " + entry.Type.ToString());
            Console.WriteLine();

            foreach(var frame in dump.Frames)
                Console.WriteLine(frame.Address.ToString("X16") + "    " + frame.Flags.ToString());


            Console.ReadLine();

            /*

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

            */
        }
    }
}
