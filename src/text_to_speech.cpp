/* wav音频头部格式 */
typedef struct _wave_pcm_hdr
{
	char            riff[4];                // = "RIFF"  
	int             size_8;                 // = FileSize - 8  
	char            wave[4];                // = "WAVE"  
	char            fmt[4];                 // = "fmt "  
	int             fmt_size;               /* = 下一个结构体的大小 : 16 */

	short int       format_tag;             // = PCM : 1  
	short int       channels;               /* = 通道数 : 1*/
	int             samples_per_sec;        /* = 采样率 : 8000 | 6000 | 11025 | 16000 */
	int             avg_bytes_per_sec;      /* = 每秒字节数 : samples_per_sec * bits_per_sample / 8 */
	short int       block_align;            /* = 每采样点字节数 : wBitsPerSample / 8 */
	short int       bits_per_sample;        /* = 量化比特数: 8 | 16 */

	char            data[4];                // = "data";  
	int             data_size;              /* = 纯数据长度 : FileSize - 44 */
} wave_pcm_hdr;

typedef struct TTS_Options {

	pthread_t tid;              /* 线程id */

}TTS_Options;

/*
*   fun:        回调函数
*   status:     结束状态
*   percent:    进度百分比
*   identifier: 结构体对象
*/
typedef void(*TTSCallbackFunction)(int, int, void*);

/* 默认wav音频头部数据 */
wave_pcm_hdr default_wav_hdr =
{
	{ 'R', 'I', 'F', 'F' },
	0,
{ 'W', 'A', 'V', 'E' },
{ 'f', 'm', 't', ' ' },
16,
1,
1,
16000,
32000,
2,
16,
{ 'd', 'a', 't', 'a' },
0
};

static void tts_free(SpeechSynsContext *ssc);
static SpeechSynsContext *tts_alloc(const char *text, const char *out_path, const char *voice_name, const char *volume,
	const char *speed, const char *pitch, const char *sample_rate, const char *text_encoding,
	const char *rdn, void *callback_fun, void *fre_ctx);
static int tts_main(SpeechSynsContext *ssc);
/* 当前进度百分比 */
static int CurrentPercent(unsigned int wav_current_size, const char *src_text, const char *speed);
static void *tts_thread(void *SpeechSynsCtx);
static char* CreateTempFileName();

/*
*   fun:        回调函数
*   status:     结束状态
*   percent:    进度百分比
*   identifier: 结构体对象
*/
static void callback_function(TTSCallbackFunction fun, int status, int percent, void *identifier);

void tts(const char *text, const char *out_path, const char *voice_name, const char *volume,
	const char *speed, const char *pitch, const char *sample_rate, const char *text_encoding,
	const char *rdn, void *callback_fun, void *fre_ctx)
{

	int ret;
	SpeechSynsContext *ssc = tts_alloc(text, out_path, voice_name, volume,
		speed, pitch, sample_rate, text_encoding,
		rdn, callback_fun, fre_ctx);
	if (ssc == NULL) {
		return;
	}
	TTS_Options *op = (TTS_Options *)ssc->options;
	if ((ret = pthread_create(&op->tid, NULL, tts_thread, ssc)) != 0) {
		printf("pthread_create failed\n");
		return;
	}
	/* Release all resource when thread destoryed */
	pthread_detach(op->tid);
	return;
}

static SpeechSynsContext *tts_alloc(const char *text, const char *out_path, const char *voice_name, const char *volume,
	const char *speed, const char *pitch, const char *sample_rate, const char *text_encoding,
	const char *rdn, void *callback_fun, void *fre_ctx)
{
	char *UTF8_Path;
	int ret;

	/* 保证所有参数不为NULL */
	if (!(text&&out_path&&voice_name&&volume&&speed&&pitch&&sample_rate&&text_encoding&&rdn/*&&callback_fun*/)) {
		LOG_PRINT("%s....Parameter invalid", __FUNCTION__);
		return NULL;
	}
	/* 没有判断指针为空 */
	SpeechSynsContext *ssc = (SpeechSynsContext*)malloc(sizeof(SpeechSynsContext));
	ssc->session_parm = (char *)malloc(300);
	ssc->options = malloc(sizeof(TTS_Options));

	const char *vn = "voice_name = ";
	const char *vol = "volume = ";
	const char *sp = "speed = ";
	const char *pit = "pitch = ";
	const char *sr = "sample_rate = ";
	const char *te = "text_encoding = ";
	const char *rd = "rdn = ";
	const char *pnct = ", ";

	if (!strcmp(text, "")) {
		tts_free(ssc);
		return NULL;
	}
	else {
		ssc->text = tts_parm_copy(text);
		LOG_PRINT("%s....text is %s", __FUNCTION__, ssc->text);
	}
	if (!strcmp(out_path, "")) {
		return NULL;
	}
	else {
		ssc->out_path = tts_parm_copy(out_path);
		LOG_PRINT("%s....out_path2 is %s", __FUNCTION__, ssc->out_path);
	}
	if (!strcmp(voice_name, "")) {
		ssc->voice_name = tts_parm_copy("xiaoyan");
	}
	else {
		ssc->voice_name = tts_parm_copy(voice_name);
	}
	if (!strcmp(volume, "")) {
		ssc->volume = tts_parm_copy("50");
	}
	else {
		ssc->volume = tts_parm_copy(volume);
	}
	if (!strcmp(speed, "")) {
		ssc->speed = tts_parm_copy("50");
	}
	else {
		ssc->speed = tts_parm_copy(speed);
	}
	if (!strcmp(pitch, "")) {
		ssc->pitch = tts_parm_copy("50");
	}
	else {
		ssc->pitch = tts_parm_copy(pitch);
	}
	if (!strcmp(sample_rate, "")) {
		ssc->sample_rate = tts_parm_copy("16000");
	}
	else {
		ssc->sample_rate = tts_parm_copy(sample_rate);
	}
	if (!strcmp(text_encoding, "")) {
		ssc->text_encoding = tts_parm_copy("utf8");      /*"gb2312"*/
	}
	else {
		ssc->text_encoding = tts_parm_copy(text_encoding);
	}
	if (!strcmp(rdn, "")) {
		ssc->rdn = tts_parm_copy("2");
	}
	else {
		ssc->rdn = tts_parm_copy(rdn);
	}

	ssc->callback_fun = callback_fun;
	ssc->current_ctx = fre_ctx;
	ssc->progress = 0;

	TTS_Options *op = (TTS_Options *)ssc->options;
	op->tid.p = NULL;
	op->tid.x = 0;
	callback_function((TTSCallbackFunction)ssc->callback_fun, 0, 0, ssc);

	/* UTF8 */
	UTF8_Path = CreateTempFileName();
	if (UTF8_Path == NULL) {
		LOG_PRINT("%s....Create temp file UTF8_Path failed", __FUNCTION__);
		return NULL;
	}

	ssc->tmp_file = (char*)malloc(1024);
	memset(ssc->tmp_file, 0, 1024);

	/* UTF8转ANSI */
	UTF8toANSI(UTF8_Path, ssc->tmp_file);

	free(UTF8_Path);

	if (ssc->tmp_file == NULL) {
		LOG_PRINT("%s....UTF8 to ANSI failed", __FUNCTION__);
		return NULL;
	}

	sprintf_s(ssc->session_parm, 300, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		sp, ssc->speed, pnct,
		te, ssc->text_encoding, pnct,
		vn, ssc->voice_name, pnct,
		sr, ssc->sample_rate, pnct,
		vol, ssc->volume, pnct,
		pit, ssc->pitch, pnct,
		rd, ssc->rdn);

	return ssc;
}

static void *tts_thread(void *SpeechSynsCtx)
{
	__try {
		int ret = -1;
		SpeechSynsContext *ssc = (SpeechSynsContext*)SpeechSynsCtx;
		ret = tts_main(ssc);
		if (ret) {
			LOG_PRINT("%s....tts_main failed", __FUNCTION__);
			callback_function((TTSCallbackFunction)ssc->callback_fun, -1, ssc->progress, ssc);
			//return 0;  
			return NULL;
		}

		ret = transcode_audio(ssc->tmp_file, ssc);
		if (ret) {
			callback_function((TTSCallbackFunction)ssc->callback_fun, -1, ssc->progress, ssc);
			LOG_PRINT("%s....transcode_audio failed", __FUNCTION__);
			goto cleanup;
		}

		/* 回调100% */
		callback_function((TTSCallbackFunction)ssc->callback_fun, 1, 100, ssc);

		/* free */
	cleanup:
		tts_free(ssc);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		LOG_PRINT("%s error\n", __FUNCTION__);
	}
	return NULL;
}

static int tts_main(SpeechSynsContext *ssc)
{
	int         ret = MSP_SUCCESS;
	/*
	*--Insufficient authorization, and there are 500 daily restrictions on the number of online calls
	*/
	const char* login_params = "appid = 596f2c50, work_dir = .";//登录参数,appid与msc库绑定,请勿随意改动  

																/* 用户登录 */
	ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，第三个参数是登录参数，用户名和密码可在http://www.xfyun.cn注册获取  
	if (MSP_SUCCESS != ret)
	{
		printf("MSPLogin failed, error code: %d.\n", ret);
		MSPLogout();
		tts_free(ssc);
		return -1;
	}

	/* 文本合成 */
	printf("开始合成 ...\n");
	ret = text_to_speech(ssc->text, ssc->tmp_file, ssc->session_parm, ssc);
	if (MSP_SUCCESS != ret)
	{
		printf("text_to_speech failed, error code: %d.\n", ret);
		MSPLogout();
		tts_free(ssc);
		return -1;
	}

	printf("合成完毕\n");
	MSPLogout();    //退出登录  
	return 0;
}

/* 文本合成 */
static int text_to_speech(const char* src_text, const char* des_path, const char* params, SpeechSynsContext *ssc)
{
	int          ret = -1;
	FILE*        fp = NULL;
	const char*  sessionID = NULL;
	unsigned int audio_len = 0;
	wave_pcm_hdr wav_hdr = default_wav_hdr;
	int          synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;

	if (NULL == src_text || NULL == des_path)
	{
		printf("params is error!\n");
		return ret;
	}

	fp = fopen(des_path, "wb");
	if (NULL == fp)
	{
		printf("fopen %s error.\n", des_path);
		return ret;
	}

	/* 开始合成 */
	sessionID = QTTSSessionBegin(params, &ret);
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSSessionBegin failed, error code: %d.\n", ret);
		fclose(fp);
		return ret;
	}
	ret = QTTSTextPut(sessionID, src_text, (unsigned int)strlen(src_text), NULL);
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSTextPut failed, error code: %d.\n", ret);
		QTTSSessionEnd(sessionID, "TextPutError");
		fclose(fp);
		return ret;
	}
	printf("正在合成 ...\n");
	fwrite(&wav_hdr, sizeof(wav_hdr), 1, fp); //添加wav音频头，使用采样率为16000  
	while (1)
	{
		/* 获取合成音频 */
		const void *data = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
		if (MSP_SUCCESS != ret)
			break;
		if (NULL != data)
		{

			wav_hdr.data_size += audio_len; //计算data_size大小  
			ssc->progress = CurrentPercent(wav_hdr.data_size, src_text, ssc->speed);
			callback_function((TTSCallbackFunction)ssc->callback_fun, 0, ssc->progress, ssc);
			fwrite(data, audio_len, 1, fp);
		}
		if (MSP_TTS_FLAG_DATA_END == synth_status)
			break;
#ifdef DEBUG  
		printf(">");
#endif  
		Sleep(150); //防止频繁占用CPU  
	}

#ifdef DEBUG  
	printf("\n");
#endif  
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSAudioGet failed, error code: %d.\n", ret);
		QTTSSessionEnd(sessionID, "AudioGetError");
		fclose(fp);
		return ret;
	}
#if DEBUG  
	printf("Total length is %d\n", wav_hdr.data_size);
#endif // DEBUG  

	/* 修正wav文件头数据的大小 */
	wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);

	/* 将修正过的数据写回文件头部,音频文件为wav格式 */
	fseek(fp, 4, 0);
	fwrite(&wav_hdr.size_8, sizeof(wav_hdr.size_8), 1, fp); //写入size_8的值  
	fseek(fp, 40, 0); //将文件指针偏移到存储data_size值的位置  
	fwrite(&wav_hdr.data_size, sizeof(wav_hdr.data_size), 1, fp); //写入data_size的值  
	fclose(fp);
	fp = NULL;

	/* 合成完毕 */
	ret = QTTSSessionEnd(sessionID, "Normal");
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSSessionEnd failed, error code: %d.\n", ret);
	}
	return ret;
}

/*
*Warning*
*malloc strlen(src) + 1
*memset strlen(src) + 1
*memcpy strlen(src)
*/
char* tts_parm_copy(const char *src)
{
	char *dst;
	int len = strlen(src) + 1;
	dst = (char*)malloc(len);
	if (dst == NULL) {
		return NULL;
	}
	memset(dst, 0, len);
	memcpy(dst, src, strlen(src));
	return dst;
}

static void tts_free(SpeechSynsContext *ssc)
{
	/* ssc->text(and so on) is const char*, so change it to void*, so can free the memory allocated by malloc */
	if (ssc->text) {
		free((void*)ssc->text);
	}
	if (ssc->out_path) {
		free((void*)ssc->out_path);
	}
	if (ssc->voice_name) {
		free((void*)ssc->voice_name);
	}
	if (ssc->volume) {
		free((void*)ssc->volume);
	}
	if (ssc->speed) {
		free((void*)ssc->speed);
	}
	if (ssc->pitch) {
		free((void*)ssc->pitch);
	}
	if (ssc->sample_rate) {
		free((void*)ssc->sample_rate);
	}
	if (ssc->text_encoding) {
		free((void*)ssc->text_encoding);
	}
	if (ssc->rdn) {
		free((void*)ssc->rdn);
	}
	if (ssc->session_parm) {
		free(ssc->session_parm);
	}
	if (ssc->options) {
		free(ssc->options);
	}
	if (ssc->tmp_file) {
		if (remove(ssc->tmp_file))
			LOG_PRINT("%s....Remove Temp file failed", __FUNCTION__);
		free(ssc->tmp_file);
	}
	if (ssc) {
		free(ssc);
	}
}

