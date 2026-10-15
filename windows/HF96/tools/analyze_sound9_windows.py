import wave, numpy as np
for fn in ['sound9_oracle.wav','sound9_current.wav']:
    w=wave.open(fn,'rb'); x=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float); sr=w.getframerate()
    print("\n",fn)
    # zero-crossing based local frequency in consecutive 1 ms windows
    for ms0 in range(0,8):
        a=int(ms0*sr/1000); b=min(len(x),int((ms0+1)*sr/1000))
        seg=x[a:b]
        if len(seg)<2: break
        z=np.where(np.signbit(seg[:-1])!=np.signbit(seg[1:]))[0]
        freq=len(z)*sr/(2*len(seg))
        rms=np.sqrt(np.mean(seg*seg))
        print(ms0,'ms', 'rms',round(float(rms),1),'zc_freq',round(float(freq),1))