#ifndef TRANSCODE_AUDIO_H  
#define TRANSCODE_AUDIO_H  

typedef void(*TranscodeCallbackFcn)(int, int, void*);
extern int transcode_audio(const char *inAudio, SpeechSynsContext *ssc);