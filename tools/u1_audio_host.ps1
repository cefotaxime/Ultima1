$ErrorActionPreference = 'Stop'

$source = @'
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;

public static class U1AudioHost {
    const uint WAVE_MAPPER = 0xFFFFFFFF;
    const uint CALLBACK_EVENT = 0x00050000;
    const uint WHDR_DONE = 0x00000001;
    const int WAIT_OBJECT_0 = 0;
    const int BUFFER_FRAMES = 128;
    const int BUFFER_COUNT = 3;
    const int SAMPLE_RATE = 48000;

    [StructLayout(LayoutKind.Sequential)]
    struct WAVEFORMATEX {
        public ushort wFormatTag;
        public ushort nChannels;
        public uint nSamplesPerSec;
        public uint nAvgBytesPerSec;
        public ushort nBlockAlign;
        public ushort wBitsPerSample;
        public ushort cbSize;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct WAVEHDR {
        public IntPtr lpData;
        public uint dwBufferLength;
        public uint dwBytesRecorded;
        public UIntPtr dwUser;
        public uint dwFlags;
        public uint dwLoops;
        public IntPtr lpNext;
        public UIntPtr reserved;
    }

    [DllImport("winmm.dll")]
    static extern int waveOutOpen(out IntPtr hwo, uint uDeviceID, ref WAVEFORMATEX lpFormat,
                                  IntPtr dwCallback, IntPtr dwInstance, uint fdwOpen);
    [DllImport("winmm.dll")]
    static extern int waveOutPrepareHeader(IntPtr hwo, IntPtr pwh, uint cbwh);
    [DllImport("winmm.dll")]
    static extern int waveOutUnprepareHeader(IntPtr hwo, IntPtr pwh, uint cbwh);
    [DllImport("winmm.dll")]
    static extern int waveOutWrite(IntPtr hwo, IntPtr pwh, uint cbwh);
    [DllImport("winmm.dll")]
    static extern int waveOutReset(IntPtr hwo);
    [DllImport("winmm.dll")]
    static extern int waveOutClose(IntPtr hwo);
    [DllImport("winmm.dll")]
    static extern uint timeBeginPeriod(uint uPeriod);
    [DllImport("winmm.dll")]
    static extern uint timeEndPeriod(uint uPeriod);

    [DllImport("kernel32.dll", SetLastError=true)]
    static extern IntPtr CreateEvent(IntPtr lpEventAttributes, bool bManualReset, bool bInitialState, string lpName);
    [DllImport("kernel32.dll")]
    static extern int WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);
    [DllImport("kernel32.dll")]
    static extern bool ResetEvent(IntPtr hEvent);
    [DllImport("kernel32.dll")]
    static extern bool CloseHandle(IntPtr hObject);

    [DllImport("avrt.dll", CharSet=CharSet.Unicode)]
    static extern IntPtr AvSetMmThreadCharacteristics(string TaskName, out uint TaskIndex);
    [DllImport("avrt.dll")]
    static extern bool AvRevertMmThreadCharacteristics(IntPtr AvrtHandle);

    sealed class Voice {
        public short[] Pcm;
        public long Start;
        public Voice(short[] pcm, long start) { Pcm = pcm; Start = start; }
    }

    static readonly object Gate = new object();
    static readonly List<Voice> Voices = new List<Voice>();
    static short[][] Clips = new short[0][];
    static long ProducedFrame = 0;
    static volatile bool Stop = false;
    static IntPtr Wave = IntPtr.Zero;
    static IntPtr Event = IntPtr.Zero;
    static short[][] Buffers = new short[BUFFER_COUNT][];
    static GCHandle[] BufferPins = new GCHandle[BUFFER_COUNT];
    static IntPtr[] Headers = new IntPtr[BUFFER_COUNT];

    static void Fill(short[] dst, long baseFrame) {
        lock (Gate) {
            for (int i = 0; i < dst.Length; ++i) {
                long pos = baseFrame + i;
                int mixed = 0;
                for (int v = 0; v < Voices.Count; ++v) {
                    Voice voice = Voices[v];
                    long off = pos - voice.Start;
                    if (off >= 0 && off < voice.Pcm.LongLength) mixed += voice.Pcm[(int)off];
                }
                if (mixed > 32767) mixed = 32767;
                else if (mixed < -32768) mixed = -32768;
                dst[i] = (short)mixed;
            }
            long end = baseFrame + dst.Length;
            for (int v = Voices.Count - 1; v >= 0; --v) {
                if (Voices[v].Start + Voices[v].Pcm.LongLength <= end) Voices.RemoveAt(v);
            }
        }
    }

    static bool QueueHeader(int index) {
        long baseFrame;
        lock (Gate) { baseFrame = ProducedFrame; ProducedFrame += BUFFER_FRAMES; }
        Fill(Buffers[index], baseFrame);
        WAVEHDR hdr = (WAVEHDR)Marshal.PtrToStructure(Headers[index], typeof(WAVEHDR));
        hdr.dwFlags &= ~WHDR_DONE;
        Marshal.StructureToPtr(hdr, Headers[index], false);
        return waveOutWrite(Wave, Headers[index], (uint)Marshal.SizeOf(typeof(WAVEHDR))) == 0;
    }

    static void MixerLoop() {
        uint taskIndex;
        IntPtr mmcss = AvSetMmThreadCharacteristics("Pro Audio", out taskIndex);
        try {
            ResetEvent(Event);
            for (int i = 0; i < BUFFER_COUNT; ++i) QueueHeader(i);
            while (!Stop) {
                int wr = WaitForSingleObject(Event, 1000);
                if (Stop) break;
                if (wr != WAIT_OBJECT_0) continue;
                ResetEvent(Event);
                for (int i = 0; i < BUFFER_COUNT; ++i) {
                    WAVEHDR hdr = (WAVEHDR)Marshal.PtrToStructure(Headers[i], typeof(WAVEHDR));
                    if ((hdr.dwFlags & WHDR_DONE) != 0) QueueHeader(i);
                }
            }
        } finally {
            if (mmcss != IntPtr.Zero) AvRevertMmThreadCharacteristics(mmcss);
        }
    }

    static bool InitWave() {
        timeBeginPeriod(1);
        Event = CreateEvent(IntPtr.Zero, true, false, null);
        if (Event == IntPtr.Zero) return false;
        WAVEFORMATEX fmt = new WAVEFORMATEX();
        fmt.wFormatTag = 1;
        fmt.nChannels = 1;
        fmt.nSamplesPerSec = SAMPLE_RATE;
        fmt.nAvgBytesPerSec = SAMPLE_RATE * 2;
        fmt.nBlockAlign = 2;
        fmt.wBitsPerSample = 16;
        fmt.cbSize = 0;
        if (waveOutOpen(out Wave, WAVE_MAPPER, ref fmt, Event, IntPtr.Zero, CALLBACK_EVENT) != 0) return false;
        uint hsz = (uint)Marshal.SizeOf(typeof(WAVEHDR));
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            Buffers[i] = new short[BUFFER_FRAMES];
            BufferPins[i] = GCHandle.Alloc(Buffers[i], GCHandleType.Pinned);
            WAVEHDR hdr = new WAVEHDR();
            hdr.lpData = BufferPins[i].AddrOfPinnedObject();
            hdr.dwBufferLength = BUFFER_FRAMES * 2;
            Headers[i] = Marshal.AllocHGlobal((int)hsz);
            Marshal.StructureToPtr(hdr, Headers[i], false);
            if (waveOutPrepareHeader(Wave, Headers[i], hsz) != 0) return false;
        }
        return true;
    }

    static void ShutdownWave() {
        Stop = true;
        if (Wave != IntPtr.Zero) waveOutReset(Wave);
        uint hsz = (uint)Marshal.SizeOf(typeof(WAVEHDR));
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            if (Headers[i] != IntPtr.Zero) {
                if (Wave != IntPtr.Zero) waveOutUnprepareHeader(Wave, Headers[i], hsz);
                Marshal.FreeHGlobal(Headers[i]); Headers[i] = IntPtr.Zero;
            }
            if (BufferPins[i].IsAllocated) BufferPins[i].Free();
        }
        if (Wave != IntPtr.Zero) { waveOutClose(Wave); Wave = IntPtr.Zero; }
        if (Event != IntPtr.Zero) { CloseHandle(Event); Event = IntPtr.Zero; }
        timeEndPeriod(1);
    }

    static bool LoadClips(BinaryReader br) {
        byte[] magic = br.ReadBytes(4);
        if (magic.Length != 4 || magic[0] != (byte)'U' || magic[1] != (byte)'1' || magic[2] != (byte)'A' || magic[3] != (byte)'1') return false;
        int count = br.ReadInt32();
        if (count != 10) return false;
        short[][] clips = new short[count][];
        for (int c = 0; c < count; ++c) {
            int frames = br.ReadInt32();
            if (frames < 0 || frames > SAMPLE_RATE * 4) return false;
            short[] pcm = new short[frames];
            for (int i = 0; i < frames; ++i) pcm[i] = br.ReadInt16();
            clips[c] = pcm;
        }
        Clips = clips;
        return true;
    }

    public static int Run() {
        Thread mixer = null;
        try {
            if (!InitWave()) { Console.Out.WriteLine("ERROR waveOut init"); Console.Out.Flush(); return 2; }
            Console.Out.WriteLine("READY waveout 48000 128 3"); Console.Out.Flush();
            BinaryReader br = new BinaryReader(Console.OpenStandardInput());
            if (!LoadClips(br)) { Console.Out.WriteLine("ERROR clip protocol"); Console.Out.Flush(); return 3; }
            mixer = new Thread(MixerLoop); mixer.IsBackground = true; mixer.Priority = ThreadPriority.Highest; mixer.Start();
            Console.Out.WriteLine("LOADED"); Console.Out.Flush();
            while (!Stop) {
                int cmd;
                try { cmd = br.ReadByte(); } catch (EndOfStreamException) { break; }
                if (cmd == 0xFF) break;
                if (cmd != 0x01) continue;
                int id = br.ReadByte();
                if (id < 0 || id >= Clips.Length) continue;
                lock (Gate) { Voices.Add(new Voice(Clips[id], ProducedFrame)); }
            }
            Stop = true;
            if (Wave != IntPtr.Zero) waveOutReset(Wave);
            if (mixer != null && mixer.IsAlive) mixer.Join(1000);
            return 0;
        } catch (Exception ex) {
            try { Console.Error.WriteLine("U1 audio host: " + ex.ToString()); } catch {}
            return 4;
        } finally {
            ShutdownWave();
        }
    }
}
'@

Add-Type -TypeDefinition $source -Language CSharp
exit [U1AudioHost]::Run()
