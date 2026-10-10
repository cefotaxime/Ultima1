
from pathlib import Path
import re,struct,wave,hashlib
root=Path(__file__).resolve().parent
src=(root.parents[1]/'src'/'HF93'/'src'/'SfxPcmOracleM225.cpp').read_text(encoding='utf8')
for n in (0,4):
 m=re.search(r'const\s+std::int16_t\s+SOUND_%02d\s*\[\s*\]\s*=\s*\{(.*?)\};'%n,src,re.S)
 if m is None: raise ValueError('missing oracle '+str(n))
 samples=[int(v) for v in re.findall(r'-?\d+',m.group(1))]
 data=struct.pack('<%dh'%len(samples),*samples)
 path=root/('SOUND_%02d_ORACLE.wav'%n)
 with wave.open(str(path),'wb') as w:
  w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes(data)
 print(path.name,'samples',len(samples),'duration_ms',round(len(samples)/48,2),'sha256_pcm',hashlib.sha256(data).hexdigest())
