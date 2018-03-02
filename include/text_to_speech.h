/* 音频合成参数 */
typedef struct SpeechSynsContext {
	const char *text;           /* 输入文本 */
	const char *out_path;       /* 音频输出路径 */
	const char *voice_name;     /* 发音人[xiaoqian,dalong,yufeng] */
	const char *volume;         /* 音量[0,100] */
	const char *speed;          /* 语速[0,100] */
	const char *pitch;          /* 音调[0,100] */
	const char *sample_rate;    /* 采样率[8000,16000] */
	const char *text_encoding;  /* 文本编码格式[GB2312；GBK；BIG5；UNICODE；GB18030；UTF8] */
	const char *rdn;            /* 音频数字发音方式[0：数值优先,1：完全数值,2：完全字符串，3：字符串优先。] */
	char * session_parm;        /* 会话参数 */
	void *callback_fun;         /* 回调函数 */
	void *current_ctx;          /* air当前的FREContext结构体 */
	int progress;               /* 回调进度（0-100） */
	void *options;              /* Structure TTS_Options */
	char *tmp_file;     /* wav临时文件 */
}SpeechSynsContext;

//该函数还不是对外接口，外面还有一层使用函数指针的封装
void tts(const char *text, const char *out_path, const char *voice_name, const char *volume,
	const char *speed, const char *pitch, const char *sample_rate, const char *text_encoding,
	const char *rdn, void *callback_fun, void *fre_ctx);