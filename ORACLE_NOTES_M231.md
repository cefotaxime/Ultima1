# M231 — leading-silence hypothesis / host-audio instrumentation

M230 사용자 검증에서 7걸음의 local footstep이 이동 종료 뒤에 뭉쳐 들렸다. M231은 사용자가 제시한 "효과음 앞의 빈 공간" 가설을 sample/transport 양쪽으로 분리한다.

M225 direct-capture oracle sound_04.wav는 첫 유의미 sample이 약 12 samples(48 kHz 기준 약 0.25 ms), M230+ PcSpeakerEmulator sound(4)는 첫 nonzero sample이 약 23 samples(약 0.48 ms)다. 따라서 수십~수백 ms의 source-leading-silence는 없다.

반면 M230 feeder는 active voice가 없어도 한 SDL device period의 0 PCM을 queue했다. 새 sound는 producedSamples_ 뒤에 배치되므로 host transport에는 [미리 queue된 silence][sound]가 만들어질 수 있었다. M231은 active/future voice가 없으면 아무 sample도 queue하지 않는다.

또 SDL_GetQueuedAudioSize 감소는 물리적 speaker presentation 시각이 아니다. M231은 U1_SFX_TRACE=1로 sound별 source/SDL timestamps를 기록하고, launcher에서 pactl이 있으면 PulseAudio/WSLg reported/configured latency도 출력한다.
