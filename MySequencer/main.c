#include <windows.h>

#include "resource.h"

#pragma comment(lib, "winmm.lib") // todo: remove vs-only code

#define APP_NAME "Sequencer"
#define STEPS_PER_BEAT 4 // sixteenth notes
#define BEATS_PER_MEASURE 4
#define MEASURES_PER_PTN 4
#define MS_PER_MINUTE 60000
#define PTN_LENGTH 4 //(STEPS_PER_BEAT * BEATS_PER_MEASURE * MEASURES_PER_PTN)

UINT_PTR timer_id;
HMIDIOUT cur_device;
int on;

struct note {
	int pitch;
	int duration;
	int elapsed_steps;
	struct note *next;
};

struct note *current_notes;

struct note notes[PTN_LENGTH] = {
	{ 0x3c, 1 },
	{ 0x3f, 1 },
	{ 0x43, 1 },
	{ 0x48, 1 },
};
int index = 0;

void send_midi_event(int status, int channel, int data1, int data2)
{
	midiOutShortMsg(cur_device, (data2 << 16) | (data1 << 8) | status | channel);
}

void CALLBACK timer_proc(HWND window, UINT msg, UINT_PTR event, DWORD time)
{
	struct note *p, *prev = NULL;

	// add any new notes to list
	struct note *new_note = malloc(sizeof *new_note);
	new_note->pitch = notes[index].pitch;
	new_note->elapsed_steps = 0;
	new_note->duration = notes[index].duration;
	new_note->next = current_notes;
	current_notes = new_note;

	p = current_notes;
	while (p != NULL) {
		if (p->elapsed_steps >= p->duration) {
			// turn off this note
			send_midi_event(0x90, 0, p->pitch, 0);

			// remove from list
			if (prev == NULL) {
				// first item
				current_notes = p->next;
			}
			else {
				prev->next = p->next;
			}
		} else {
			if (p->elapsed_steps == 0) {
				// turn on this new note
				send_midi_event(0x90, 0, p->pitch, 0x7f);
			}

			p->elapsed_steps++;
			prev = p;
		}

		p = p->next;
	}

	index = (index + 1) % PTN_LENGTH;
}

void setup_timer(int bpm)
{
	if (timer_id) {
		KillTimer(NULL, timer_id);
	}
	int resolution = (MS_PER_MINUTE / (bpm * STEPS_PER_BEAT));
	//timer_id = SetTimer(NULL, 0, resolution, timer_proc);
}

void close_midi(void)
{
	if (cur_device) {
		midiOutClose(cur_device);
	}
}

void change_midi_device(HWND combo_box)
{
	int index;
	index = SendMessage(combo_box, CB_GETCURSEL, 0, 0);
	close_midi();
	midiOutOpen(&cur_device, index, 0, 0, 0);
}

int populate_midi_devices(HWND dialog)
{
	int k, num_devices;
	MIDIOUTCAPS moc;
	HWND device_list;

	device_list = GetDlgItem(dialog, IDC_MIDI_DEVICE);
	num_devices = midiOutGetNumDevs();
	for (k = 0; k < num_devices; k++) {
		midiOutGetDevCaps(k, &moc, sizeof moc);
		SendMessage(device_list, CB_ADDSTRING, 0, (LPARAM) moc.szPname);
	}

	if (num_devices > 0) {
		SendMessage(device_list, CB_SETCURSEL, 0, 0);
		change_midi_device(device_list);
		return TRUE;
	}

	MessageBox(dialog, "Couldn't find any MIDI devices - check your USB cables and restart " APP_NAME ".", APP_NAME, 0);
	return FALSE;
}

BOOL CALLBACK dialog_proc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		if (populate_midi_devices(dialog)) {
			setup_timer(120);
		}
		else {
			EndDialog(dialog, 0);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MIDI_DEVICE:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				change_midi_device((HWND) lParam);
				return TRUE;
			}
			return FALSE;
		case IDC_STEP:
			timer_proc(NULL, 0, 0, 0);
		}
		return TRUE;
	case WM_CLOSE:
		close_midi();
		EndDialog(dialog, 0);
		return TRUE;
	}
	return FALSE;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR args, int show)
{
	DialogBox(instance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, dialog_proc);
	return 0;
}
