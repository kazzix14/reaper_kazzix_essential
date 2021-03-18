#include <string>

#include <color/color.hpp>

#include "reaper_plugin.h"
#include "reaper_plugin_functions.h"

enum Section
{
	Main             = 0,
	MainAlt          = 100,
	MIDIEditor       = 32060,
	MIDIEventList    = 32061,
	MIDIInlineEditor = 32062,
	MediaExplorer    = 32063,
};

static void insert_ready_to_create_track(){
	int num_selected_tracks = CountSelectedTracks(0);
	MediaTrack* selected_track = GetSelectedTrack(0, num_selected_tracks - 1);

	// decrement the return value because it is 1 based
	int selected_track_index = GetMediaTrackInfo_Value(selected_track, "IP_TRACKNUMBER") - 1;

	// insert new track bellow the selected track
	int new_track_index = selected_track_index + 1;
	InsertTrackAtIndex(new_track_index, true);
	MediaTrack* new_track = GetTrack(0, new_track_index);

	// adjest track height in TCP and lock it
	SetMediaTrackInfo_Value(new_track, "I_HEIGHTOVERRIDE", 24.0);
	SetMediaTrackInfo_Value(new_track, "B_HEIGHTLOCK", 1.0);

	// name new track
	GetSetMediaTrackInfo_String(new_track, "P_NAME", const_cast<char*>("New Track"), true);

	// insert console track at the end of TCP
	int new_console_track_index = GetNumTracks();
	InsertTrackAtIndex(new_console_track_index, false);
	MediaTrack* new_console_track = GetTrack(0, new_console_track_index);

	// hide console track
	SetMediaTrackInfo_Value(new_console_track, "B_SHOWINMIXER", 0.0);
	SetMediaTrackInfo_Value(new_console_track, "B_SHOWINTCP", 0.0);

	// adjest console track height in TCP and lock it
	SetMediaTrackInfo_Value(new_console_track, "I_HEIGHTOVERRIDE", 24.0);
	SetMediaTrackInfo_Value(new_console_track, "B_HEIGHTLOCK", 1.0);

	// name new console track
	GetSetMediaTrackInfo_String(new_console_track, "P_NAME", const_cast<char*>("Console Emulation"), true);

	// turn off the main send(master send)
	SetMediaTrackInfo_Value(new_track, "B_MAINSEND", 0.0);

	// create send to console track
	if (CreateTrackSend(new_track, new_console_track) < 0)
		ShowMessageBox("Failed to create send to console track", "Error", 5);

	// turn off the main send(master send)
	SetMediaTrackInfo_Value(new_track, "B_MAINSEND", 0.0);
	
	// insert console 7 plugin
	if (TrackFX_AddByName(new_console_track, "Console7Channel (airwindows)", false, 1) == -1)
		ShowMessageBox("Failed to insert 'Console7Channel (airwindows)'into console track", "Error", 5);

	// color tracks
	int num_tracks = GetNumTracks();
	for (int track_index = 0; track_index < num_tracks; track_index++) {
		float track_position = static_cast<float>(track_index) / static_cast<float>(num_tracks);
		float hue = track_position * 255.0 * 2.0;
		color::hsv<uint8_t> hsv({ static_cast<uint8_t>(hue), 255, 255 });
		int color = ColorToNative(color::get::red(hsv), color::get::green(hsv), color::get::blue(hsv));

		MediaTrack* track = GetTrack(0, track_index);
		SetTrackColor(track, color);
	}

	// select new track
	SetOnlyTrackSelected(new_track);
}

static struct CustomAction {
	int action_id;
	custom_action_register_t action_information;
	void (*function)(void);
} actions[] = {
	{
		0,
		{
			static_cast< int >( Section::Main ), // section that this action appears
			"kazzix_insert_ready_to_create_track", // action id
			"Kazzix: Insert a track and add some essential FX", // description
			0, // reserved
		},
		insert_ready_to_create_track,
	}
};

static bool action_callback_handler( KbdSectionInfo* kbd_section_information, int action_id, int value, int valhw, int relmode, HWND hwnd )
{
	for (CustomAction& action : actions) {
		if (action_id == action.action_id && kbd_section_information->uniqueID == action.action_information.uniqueSectionId) {
			(*action.function)();
			return true;
		}
	}

	return false; // action is not processed
}

// dll entry point
extern "C"
{
	REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec)
	{
		if (rec)
		{
			// this have to be done at first
			int numOfErrors = REAPERAPI_LoadAPI(rec->GetFunc);

			for (CustomAction& action : actions) {
				action.action_id = plugin_register("custom_action", &(action.action_information));
			}

			plugin_register("hookcommand2", action_callback_handler);

			return 1;
		}
		else // at exit
		{
			// remove action
			for (CustomAction& action : actions) {
				action.action_id = plugin_register("-custom_action", &(action.action_information));
			}

			// remove callback handler
			plugin_register("-hookcommand2", action_callback_handler);

			return 0;
		}
	}
}