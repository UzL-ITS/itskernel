using System;
using System.Drawing;
using System.IO;
using System.Linq;

namespace dumpconverter
{
    internal class Program
    {
        private const int WIDTH = 512;

        private static void Main(string[] args)
        {
            // Read dump
            Dump dump = new Dump("R:\\itskernel\\out\\dump.bin");

            //foreach(var entry in dump.MemoryMap)
            //    Console.WriteLine("- " + entry.AddressStart.ToString("X16") + "    " + entry.AddressEnd.ToString("X16") + "    " + entry.Type.ToString());
            //Console.WriteLine();

            //foreach(var frame in dump.Frames)
            //    Console.WriteLine(frame.Address.ToString("X16") + "    " + frame.Flags.ToString());



            // Delete old dumps
            DirectoryInfo dumpsDir = new DirectoryInfo("R:\\dumps");
            if(!dumpsDir.Exists)
                dumpsDir.Create();
            foreach(var file in dumpsDir.GetFiles("*.png"))
                file.Delete();

            foreach(var mapEntry in dump.MemoryMap)
            {
                // Skip uninteresting entries
                if(mapEntry.Type != Dump.MemoryMapEntryTypes.Available || mapEntry.AddressStart >= mapEntry.AddressEnd)
                    continue;

                // Prepare image
                string imageName = $"{mapEntry.AddressStart.ToString("X16")}-{mapEntry.AddressEnd.ToString("X16")}-{mapEntry.Type.ToString()}.png";
                int imageFrameCount = (int)((mapEntry.AddressEnd - mapEntry.AddressStart) / 4096);
                Bitmap image = new Bitmap(WIDTH, (imageFrameCount + WIDTH - 1) / WIDTH);
                using(Graphics g = Graphics.FromImage(image))
                    g.Clear(Color.Black);

                foreach(var frame in dump.Frames.Where(f => mapEntry.AddressStart <= f.Address && f.Address <= mapEntry.AddressEnd))
                {
                    // Draw frame
                    int pos = (int)((frame.Address - mapEntry.AddressStart) / 4096);
                    int row = pos / WIDTH;
                    int col = pos % WIDTH;
                    if((frame.Flags & Dump.DumpEntryFlags.StackPage) == Dump.DumpEntryFlags.StackPage)
                        image.SetPixel(col, row, Color.Blue);
                    else if((frame.Flags & Dump.DumpEntryFlags.Reserved) == Dump.DumpEntryFlags.Reserved)
                        image.SetPixel(col, row, Color.Cyan);
                    else if((frame.Flags & Dump.DumpEntryFlags.Auxiliary) == Dump.DumpEntryFlags.Auxiliary)
                        image.SetPixel(col, row, Color.Green);
                    else if((frame.Flags & Dump.DumpEntryFlags.Free) == Dump.DumpEntryFlags.Free)
                    {
                        if((frame.Flags & Dump.DumpEntryFlags.Size2M) == Dump.DumpEntryFlags.Size2M)
                        {
                            int cnt = (2 * 1024 * 1024) / 4096;
                            for(int i = 0; i < cnt; ++i)
                            {
                                row = (pos + i) / WIDTH;
                                col = (pos + i) % WIDTH;
                                image.SetPixel(col, row, Color.Orange);
                            }
                        }
                        else if((frame.Flags & Dump.DumpEntryFlags.Size1G) == Dump.DumpEntryFlags.Size1G)
                        {
                            int cnt = (1 * 1024 * 1024 * 1024) / 4096;
                            for(int i = 0; i < cnt; ++i)
                            {
                                row = (pos + i) / WIDTH;
                                col = (pos + i) % WIDTH;
                                image.SetPixel(col, row, Color.Yellow);
                            }
                        }
                        else
                        {
                            image.SetPixel(col, row, Color.Red);
                        }
                    }
                }

                // Done
                image.Save("R:\\dumps\\" + imageName, System.Drawing.Imaging.ImageFormat.Png);
            }

            // Print legend
            Console.WriteLine($"Number of 4K pages per line: {WIDTH} ({Math.Round(4096.0 * WIDTH / (1024 * 1024), 1)}M)");
            Console.WriteLine($"Colors:");
            Console.WriteLine($"    Black     Used");
            Console.WriteLine($"    Blue      Stack Page");
            Console.WriteLine($"    Cyan      Reserved by PMM");
            Console.WriteLine($"    Green     Auxiliary for PMM");
            Console.WriteLine($"    Red       Free (4K)");
            Console.WriteLine($"    Orange    Free (2M)");
            Console.WriteLine($"    Yellow    Free (1G)");
            Console.Read();
        }
    }
}
