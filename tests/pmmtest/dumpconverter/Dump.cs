using System;
using System.Collections.Generic;
using System.IO;

namespace dumpconverter
{
    internal class Dump
    {
        public  List<MemoryMapEntry> MemoryMap { get; } = new List<MemoryMapEntry>();
        public List<FrameData> Frames { get; } = new List<FrameData>();

        public Dump(string fileName)
        {
            // Open dump file
            using(var dump = new BinaryReader(File.OpenRead(fileName)))
            {
                // Read memory map
                int memoryMapEntryCount = (int)dump.ReadUInt32();
                for(int i = 0; i < memoryMapEntryCount; ++i)
                {
                    MemoryMapEntry entry = new MemoryMapEntry();
                    entry.Type =(MemoryMapEntryTypes) dump.ReadUInt32();
                    entry.AddressStart = dump.ReadUInt64();
                    entry.AddressEnd = dump.ReadUInt64();
                    MemoryMap.Add(entry);
                }

                // Read frames
                int frameCount = (int)dump.ReadUInt32();
                for(int i =0; i <frameCount; ++i)
                {
                    FrameData frame = new FrameData();
                    ulong data = dump.ReadUInt64();
                    frame.Address = data & ~0xFFFul;
                    frame.Flags = (DumpEntryFlags)(data & 0xFFFul);
                    Frames.Add(frame);
                }
            }
        }

        public class MemoryMapEntry
        {
            public MemoryMapEntryTypes Type { get; set; }
            public ulong AddressStart { get; set; }
            public ulong AddressEnd { get; set; }
        }

        public class FrameData
        {
            public ulong Address { get; set; }
            public DumpEntryFlags Flags { get; set; }
        }

        public enum MemoryMapEntryTypes : uint
        {
            Available = 1,
            Reserved = 2,
            AcpiReclaim = 3,
            AcpiNvs = 4,
            Bad = 5
        }

        [Flags]
        public enum DumpEntryFlags : byte
        {
            Free = 0x01,
            StackPage = 0x02,
            Size4K = 0x00,
            Size2M = 0x04,
            Size1G = 0x08,
            Reserved = 0x10,
            Auxiliary = 0x20
        }
    }
}
