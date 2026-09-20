#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <psputility.h>
#include <string.h>

#include "ui.h"

static unsigned int __attribute__((aligned(16))) s_list[262144];
static int s_gu;

static void gu_start_frame(unsigned int color)
{
	sceGuStart(GU_DIRECT, s_list);
	sceGuClearColor(color);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, 0);
}

static void gu_init(void)
{
	if (s_gu) {
		return;
	}
	sceGuInit();
	sceGuStart(GU_DIRECT, s_list);
	sceGuDrawBuffer(GU_PSM_8888, (void *)0, 512);
	sceGuDispBuffer(480, 272, (void *)0x88000, 512);
	sceGuDepthBuffer((void *)0x110000, 512);
	sceGuOffset(2048 - (480 / 2), 2048 - (272 / 2));
	sceGuViewport(2048, 2048, 480, 272);
	sceGuDepthRange(0xC350, 0x2710);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
	s_gu = 1;
}

void ui_init(void)
{
	pspDebugScreenInit();
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void ui_clear(void)
{
	pspDebugScreenClear();
	pspDebugScreenSetXY(0, 0);
}

unsigned int ui_buttons(void)
{
	SceCtrlData pad;
	static unsigned int prev;

	sceCtrlReadBufferPositive(&pad, 1);
	if (pad.Buttons != prev) {
		unsigned int pressed = pad.Buttons & ~prev;
		prev = pad.Buttons;
		return pressed;
	}
	prev = pad.Buttons;
	return 0;
}

int ui_osk(const char *title, const char *initial, char *out, int maxlen, int password)
{
	unsigned short intext[128];
	unsigned short outtext[128];
	unsigned short desc[32];
	SceUtilityOskData data;
	SceUtilityOskParams params;
	int i, done = 0;

	gu_init();

	memset(&data, 0, sizeof(data));
	memset(&params, 0, sizeof(params));
	memset(intext, 0, sizeof(intext));
	memset(outtext, 0, sizeof(outtext));
	memset(desc, 0, sizeof(desc));

	for (i = 0; title[i] && i < 31; i++) {
		desc[i] = (unsigned short)(unsigned char)title[i];
	}
	for (i = 0; initial && initial[i] && i < 127; i++) {
		intext[i] = (unsigned short)(unsigned char)initial[i];
	}

	data.language = PSP_UTILITY_OSK_LANGUAGE_DEFAULT;
	data.lines = 1;
	data.unk_24 = 1;
	data.inputtype = password
		? PSP_UTILITY_OSK_INPUTTYPE_ALL
		: (PSP_UTILITY_OSK_INPUTTYPE_LATIN_LOWERCASE |
		   PSP_UTILITY_OSK_INPUTTYPE_LATIN_UPPERCASE |
		   PSP_UTILITY_OSK_INPUTTYPE_LATIN_DIGIT |
		   PSP_UTILITY_OSK_INPUTTYPE_LATIN_SYMBOL);
	data.desc = desc;
	data.intext = intext;
	data.outtextlength = 128;
	data.outtextlimit = maxlen;
	data.outtext = outtext;

	params.base.size = sizeof(params);
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &params.base.language);
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_BUTTON_SWAP, &params.base.buttonSwap);
	params.base.graphicsThread = 17;
	params.base.accessThread = 19;
	params.base.fontThread = 18;
	params.base.soundThread = 16;
	params.datacount = 1;
	params.data = &data;

	if (sceUtilityOskInitStart(&params) < 0) {
		pspDebugScreenInit();
		return -1;
	}

	while (!done) {
		gu_start_frame(0xFF1A1A1A);
		switch (sceUtilityOskGetStatus()) {
		case PSP_UTILITY_DIALOG_VISIBLE:
			sceUtilityOskUpdate(1);
			break;
		case PSP_UTILITY_DIALOG_QUIT:
			sceUtilityOskShutdownStart();
			break;
		case PSP_UTILITY_DIALOG_NONE:
			done = 1;
			break;
		default:
			break;
		}
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}

	pspDebugScreenInit();

	if (data.result == PSP_UTILITY_OSK_RESULT_CANCELLED) {
		return 0;
	}

	for (i = 0; i < maxlen && outtext[i]; i++) {
		out[i] = (char)(outtext[i] & 0xFF);
	}
	out[i < maxlen ? i : maxlen] = 0;
	return 1;
}
