$ErrorActionPreference = 'Stop'

$source = @'
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;

public static class U1AudioHost {
    const int SAMPLE_RATE = 48000;
    const int CHANNELS = 2;
    const int BYTES_PER_SAMPLE = 2;
    const int CLSCTX_ALL = 23;
    const uint DEVICE_STATE_ACTIVE = 0x00000001;
    const uint AUDCLNT_STREAMFLAGS_EVENTCALLBACK = 0x00040000;
    const uint AUDCLNT_STREAMFLAGS_NOPERSIST = 0x00080000;
    const uint AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM = 0x80000000;
    const uint AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY = 0x08000000;
    const uint AUDCLNT_BUFFERFLAGS_SILENT = 0x00000002;
    const int AUDCLNT_SHAREMODE_SHARED = 0;
    const int AUDCLNT_SHAREMODE_EXCLUSIVE = 1;
    const int S_OK = 0;
    const uint COINIT_MULTITHREADED = 0x0;

    enum EDataFlow { eRender = 0, eCapture = 1, eAll = 2 }
    enum ERole { eConsole = 0, eMultimedia = 1, eCommunications = 2 }

    [StructLayout(LayoutKind.Sequential, Pack=2)]
    struct WAVEFORMATEX {
        public ushort wFormatTag;
        public ushort nChannels;
        public uint nSamplesPerSec;
        public uint nAvgBytesPerSec;
        public ushort nBlockAlign;
        public ushort wBitsPerSample;
        public ushort cbSize;
    }

    [ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IMMDeviceEnumerator {
        [PreserveSig] int EnumAudioEndpoints(EDataFlow dataFlow, uint dwStateMask, out IntPtr ppDevices);
        [PreserveSig] int GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role, out IMMDevice ppEndpoint);
        [PreserveSig] int GetDevice([MarshalAs(UnmanagedType.LPWStr)] string pwstrId, out IMMDevice ppDevice);
        [PreserveSig] int RegisterEndpointNotificationCallback(IntPtr pClient);
        [PreserveSig] int UnregisterEndpointNotificationCallback(IntPtr pClient);
    }

    [ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IMMDevice {
        [PreserveSig] int Activate(ref Guid iid, int dwClsCtx, IntPtr pActivationParams,
                                   [MarshalAs(UnmanagedType.IUnknown)] out object ppInterface);
        [PreserveSig] int OpenPropertyStore(int stgmAccess, out IntPtr ppProperties);
        [PreserveSig] int GetId([MarshalAs(UnmanagedType.LPWStr)] out string ppstrId);
        [PreserveSig] int GetState(out uint pdwState);
    }

    [ComImport, Guid("1CB9AD4C-DBFA-4c32-B178-C2F568A703B2"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IAudioClient {
        [PreserveSig] int Initialize(int ShareMode, uint StreamFlags,
                                     long hnsBufferDuration, long hnsPeriodicity,
                                     IntPtr pFormat, IntPtr AudioSessionGuid);
        [PreserveSig] int GetBufferSize(out uint pNumBufferFrames);
        [PreserveSig] int GetStreamLatency(out long phnsLatency);
        [PreserveSig] int GetCurrentPadding(out uint pNumPaddingFrames);
        [PreserveSig] int IsFormatSupported(int ShareMode, IntPtr pFormat, IntPtr ppClosestMatch);
        [PreserveSig] int GetMixFormat(out IntPtr ppDeviceFormat);
        [PreserveSig] int GetDevicePeriod(out long phnsDefaultDevicePeriod, out long phnsMinimumDevicePeriod);
        [PreserveSig] int Start();
        [PreserveSig] int Stop();
        [PreserveSig] int Reset();
        [PreserveSig] int SetEventHandle(IntPtr eventHandle);
        [PreserveSig] int GetService(ref Guid riid, [MarshalAs(UnmanagedType.IUnknown)] out object ppv);
    }

    [ComImport, Guid("F294ACFC-3146-4483-A7BF-ADDCA7C260E2"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IAudioRenderClient {
        [PreserveSig] int GetBuffer(uint NumFramesRequested, out IntPtr ppData);
        [PreserveSig] int ReleaseBuffer(uint NumFramesWritten, uint dwFlags);
    }

    sealed class Voice {
        public short[] Pcm;
        public long Start;
        public Voice(short[] pcm, long start) { Pcm = pcm; Start = start; }
    }

    static readonly object Gate = new object();
    static readonly List<Voice> Voices = new List<Voice>();
    static readonly ManualResetEvent InitDone = new ManualResetEvent(false);
    static readonly ManualResetEvent ClipsLoaded = new ManualResetEvent(false);
    static readonly ManualResetEvent AudioStarted = new ManualResetEvent(false);
    static short[][] Clips = new short[0][];
    static long ProducedFrame = 0;
    static volatile bool StopRequested = false;

    // Owned and used only by the MTA mixer thread.
    static IMMDeviceEnumerator Enumerator;
    static IMMDevice Device;
    static IAudioClient Client;
    static IAudioRenderClient Render;
    static EventWaitHandle AudioEvent;
    static Thread Mixer;
    static uint BufferFrames;
    static bool Exclusive;
    static long DefaultPeriodHns;
    static long MinimumPeriodHns;
    static int InitHr = -1;
    static int StartHr = -1;
    static string InitDetail = "not initialized";
    static long StreamLatencyHns = 0;

    static void ReleaseCom(object o) {
        if (o != null && Marshal.IsComObject(o)) {
            try { Marshal.FinalReleaseComObject(o); } catch { }
        }
    }

    static IntPtr AllocFormat() {
        WAVEFORMATEX fmt = new WAVEFORMATEX();
        fmt.wFormatTag = 1;
        fmt.nChannels = CHANNELS;
        fmt.nSamplesPerSec = SAMPLE_RATE;
        fmt.nBlockAlign = (ushort)(CHANNELS * BYTES_PER_SAMPLE);
        fmt.nAvgBytesPerSec = (uint)(SAMPLE_RATE * fmt.nBlockAlign);
        fmt.wBitsPerSample = 16;
        fmt.cbSize = 0;
        IntPtr p = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(WAVEFORMATEX)));
        Marshal.StructureToPtr(fmt, p, false);
        return p;
    }

    static int OpenClient(out string detail) {
        detail = "";
        Type t = Type.GetTypeFromCLSID(new Guid("BCDE0395-E52F-467C-8E3D-C4579291692E"), true);
        Enumerator = (IMMDeviceEnumerator)Activator.CreateInstance(t);
        int hr = Enumerator.GetDefaultAudioEndpoint(EDataFlow.eRender, ERole.eMultimedia, out Device);
        if (hr < 0) { detail = "GetDefaultAudioEndpoint=0x" + hr.ToString("X8"); return hr; }
        uint state;
        if (Device.GetState(out state) >= 0 && (state & DEVICE_STATE_ACTIVE) == 0) {
            detail = "default endpoint inactive"; return unchecked((int)0x88890004);
        }

        Guid iid = typeof(IAudioClient).GUID;
        object obj;
        hr = Device.Activate(ref iid, CLSCTX_ALL, IntPtr.Zero, out obj);
        if (hr < 0) { detail = "IMMDevice.Activate=0x" + hr.ToString("X8"); return hr; }
        Client = (IAudioClient)obj;
        hr = Client.GetDevicePeriod(out DefaultPeriodHns, out MinimumPeriodHns);
        if (hr < 0) { detail = "GetDevicePeriod=0x" + hr.ToString("X8"); return hr; }

        IntPtr fmt = AllocFormat();
        try {
            int fmtHr = Client.IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, fmt, IntPtr.Zero);
            if (fmtHr == S_OK) {
                long period = MinimumPeriodHns > 0 ? MinimumPeriodHns : 30000;
                hr = Client.Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
                    AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                    period, period, fmt, IntPtr.Zero);
                if (hr >= 0) {
                    Exclusive = true;
                    detail = "wasapi-exclusive";
                }
            }

            if (!Exclusive) {
                ReleaseCom(Client); Client = null;
                Guid iid2 = typeof(IAudioClient).GUID;
                object obj2;
                hr = Device.Activate(ref iid2, CLSCTX_ALL, IntPtr.Zero, out obj2);
                if (hr < 0) { detail = "shared Activate=0x" + hr.ToString("X8"); return hr; }
                Client = (IAudioClient)obj2;
                hr = Client.Initialize(AUDCLNT_SHAREMODE_SHARED,
                    AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST |
                    AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                    0, 0, fmt, IntPtr.Zero);
                if (hr < 0) { detail = "shared Initialize=0x" + hr.ToString("X8"); return hr; }
                detail = "wasapi-shared";
            }
        } finally {
            Marshal.FreeHGlobal(fmt);
        }

        hr = Client.GetBufferSize(out BufferFrames);
        if (hr < 0 || BufferFrames == 0) { detail = "GetBufferSize=0x" + hr.ToString("X8"); return hr < 0 ? hr : -1; }
        Client.GetStreamLatency(out StreamLatencyHns);

        AudioEvent = new EventWaitHandle(false, EventResetMode.AutoReset);
        hr = Client.SetEventHandle(AudioEvent.SafeWaitHandle.DangerousGetHandle());
        if (hr < 0) { detail = "SetEventHandle=0x" + hr.ToString("X8"); return hr; }

        Guid renderIid = typeof(IAudioRenderClient).GUID;
        object renderObj;
        hr = Client.GetService(ref renderIid, out renderObj);
        if (hr < 0) { detail = "GetService(IAudioRenderClient)=0x" + hr.ToString("X8"); return hr; }
        Render = (IAudioRenderClient)renderObj;
        return S_OK;
    }

    static void MixMono(short[] mono, long baseFrame) {
        lock (Gate) {
            for (int i = 0; i < mono.Length; ++i) {
                long pos = baseFrame + i;
                int mixed = 0;
                for (int v = 0; v < Voices.Count; ++v) {
                    Voice voice = Voices[v];
                    long off = pos - voice.Start;
                    if (off >= 0 && off < voice.Pcm.LongLength) mixed += voice.Pcm[(int)off];
                }
                if (mixed > 32767) mixed = 32767;
                else if (mixed < -32768) mixed = -32768;
                mono[i] = (short)mixed;
            }
            long end = baseFrame + mono.Length;
            for (int v = Voices.Count - 1; v >= 0; --v) {
                if (Voices[v].Start + Voices[v].Pcm.LongLength <= end) Voices.RemoveAt(v);
            }
        }
    }

    static bool WriteFrames(uint frames) {
        if (frames == 0) return true;
        IntPtr p;
        int hr = Render.GetBuffer(frames, out p);
        if (hr < 0) return false;
        long baseFrame;
        lock (Gate) { baseFrame = ProducedFrame; ProducedFrame += frames; }
        short[] mono = new short[(int)frames];
        MixMono(mono, baseFrame);
        short[] stereo = new short[(int)frames * CHANNELS];
        bool any = false;
        for (int i = 0; i < mono.Length; ++i) {
            short v = mono[i];
            if (v != 0) any = true;
            stereo[i * 2] = v;
            stereo[i * 2 + 1] = v;
        }
        Marshal.Copy(stereo, 0, p, stereo.Length);
        hr = Render.ReleaseBuffer(frames, any ? 0u : AUDCLNT_BUFFERFLAGS_SILENT);
        return hr >= 0;
    }

    static void ReleaseAudioObjects() {
        try { if (Client != null) Client.Stop(); } catch { }
        try { if (Client != null) Client.Reset(); } catch { }
        if (AudioEvent != null) { AudioEvent.Dispose(); AudioEvent = null; }
        ReleaseCom(Render); Render = null;
        ReleaseCom(Client); Client = null;
        ReleaseCom(Device); Device = null;
        ReleaseCom(Enumerator); Enumerator = null;
    }

    static void MixerLoop() {
        Native.CoInitializeEx(IntPtr.Zero, COINIT_MULTITHREADED);
        uint taskIndex;
        IntPtr mmcss = Native.AvSetMmThreadCharacteristics("Pro Audio", out taskIndex);
        try {
            InitHr = OpenClient(out InitDetail);
            InitDone.Set();
            if (InitHr < 0) return;

            ClipsLoaded.WaitOne();
            if (StopRequested) return;
            if (!WriteFrames(BufferFrames)) { StartHr = -1; InitDetail = "initial GetBuffer/ReleaseBuffer failed"; AudioStarted.Set(); return; }
            StartHr = Client.Start();
            AudioStarted.Set();
            if (StartHr < 0) return;

            while (!StopRequested) {
                if (!AudioEvent.WaitOne(1000)) continue;
                if (StopRequested) break;
                if (Exclusive) {
                    if (!WriteFrames(BufferFrames)) break;
                } else {
                    uint padding;
                    if (Client.GetCurrentPadding(out padding) < 0) break;
                    uint writable = BufferFrames > padding ? BufferFrames - padding : 0;
                    if (writable != 0 && !WriteFrames(writable)) break;
                }
            }
        } finally {
            ReleaseAudioObjects();
            if (mmcss != IntPtr.Zero) Native.AvRevertMmThreadCharacteristics(mmcss);
            Native.CoUninitialize();
            InitDone.Set();
            AudioStarted.Set();
        }
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

    static void Shutdown() {
        StopRequested = true;
        ClipsLoaded.Set();
        try { if (AudioEvent != null) AudioEvent.Set(); } catch { }
        try { if (Mixer != null && Mixer.IsAlive) Mixer.Join(1500); } catch { }
    }

    public static int Run() {
        try {
            Mixer = new Thread(MixerLoop);
            Mixer.IsBackground = true;
            Mixer.Priority = ThreadPriority.Highest;
            Mixer.SetApartmentState(ApartmentState.MTA);
            Mixer.Start();
            if (!InitDone.WaitOne(10000) || InitHr < 0) {
                Console.Out.WriteLine("ERROR " + InitDetail); Console.Out.Flush(); return 2;
            }

            Console.Out.WriteLine("READY " + InitDetail + " 48000 stereo16 buffer_frames=" + BufferFrames +
                                  " min_period_hns=" + MinimumPeriodHns + " stream_latency_hns=" + StreamLatencyHns);
            Console.Out.Flush();
            BinaryReader br = new BinaryReader(Console.OpenStandardInput());
            if (!LoadClips(br)) { Console.Out.WriteLine("ERROR clip protocol"); Console.Out.Flush(); return 3; }
            ClipsLoaded.Set();
            if (!AudioStarted.WaitOne(5000) || StartHr < 0) {
                Console.Out.WriteLine("ERROR Start=0x" + StartHr.ToString("X8") + " " + InitDetail); Console.Out.Flush(); return 4;
            }
            Console.Out.WriteLine("LOADED"); Console.Out.Flush();

            while (!StopRequested) {
                int cmd;
                try { cmd = br.ReadByte(); } catch (EndOfStreamException) { break; }
                if (cmd == 0xFF) break;
                if (cmd != 0x01) continue;
                int id = br.ReadByte();
                if (id < 0 || id >= Clips.Length) continue;
                lock (Gate) { Voices.Add(new Voice(Clips[id], ProducedFrame)); }
            }
            return 0;
        } catch (Exception ex) {
            try { Console.Error.WriteLine("U1 WASAPI host: " + ex.ToString()); } catch { }
            return 6;
        } finally {
            Shutdown();
        }
    }

    static class Native {
        [DllImport("ole32.dll")]
        public static extern int CoInitializeEx(IntPtr pvReserved, uint dwCoInit);
        [DllImport("ole32.dll")]
        public static extern void CoUninitialize();
        [DllImport("avrt.dll", CharSet=CharSet.Unicode)]
        public static extern IntPtr AvSetMmThreadCharacteristics(string TaskName, out uint TaskIndex);
        [DllImport("avrt.dll")]
        public static extern bool AvRevertMmThreadCharacteristics(IntPtr AvrtHandle);
    }
}
'@

Add-Type -TypeDefinition $source -Language CSharp
exit [U1AudioHost]::Run()
