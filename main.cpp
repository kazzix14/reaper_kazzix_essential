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

// 0 based
int last_selected_track_index() {
	int num_selected_tracks = CountSelectedTracks(0);
	MediaTrack* selected_track = GetSelectedTrack(0, num_selected_tracks - 1);

	// decrement the return value because it is 1 based
	int selected_track_index = GetMediaTrackInfo_Value(selected_track, "IP_TRACKNUMBER") - 1;

	// if no track is selected, returns end of TCP
	if (selected_track_index == 0)
		selected_track_index = GetNumTracks() - 1;

	return selected_track_index;
}


void lock_track_height(MediaTrack* track, int track_height) {
	if (track_height <= 0) track_height = 24;
	SetMediaTrackInfo_Value(track, "I_HEIGHTOVERRIDE", track_height);
	SetMediaTrackInfo_Value(track, "B_HEIGHTLOCK", 1.0);
}

void hide_track(MediaTrack* track) {
	SetMediaTrackInfo_Value(track, "B_SHOWINMIXER", 0.0);
	SetMediaTrackInfo_Value(track, "B_SHOWINTCP", 0.0);
}

MediaTrack* insert_track(int new_track_index, std::string track_name, int track_height) {
	InsertTrackAtIndex(new_track_index, true);
	MediaTrack* new_track = GetTrack(0, new_track_index);
	lock_track_height(new_track, track_height);
	GetSetMediaTrackInfo_String(new_track, "P_NAME", const_cast<char*>(track_name.c_str()), true);
	return new_track;
}

void insert_plugin(MediaTrack* track, std::string plugin_name) {
	if (TrackFX_AddByName(track, plugin_name.c_str(), false, 1) == -1)
		ShowMessageBox("Failed to insert 'Console7Channel (airwindows)'into a track", "Error", 5);
}

void color_tracks() {
	int num_tracks = GetNumTracks();
	for (int track_index = 0; track_index < num_tracks; track_index++) {
		float track_position = static_cast<float>(track_index) / static_cast<float>(num_tracks);
		float hue = track_position * 255.0;
		color::hsv<uint8_t> hsv({ static_cast<uint8_t>(hue), 255, 255 });
		int color = ColorToNative(color::get::red(hsv), color::get::green(hsv), color::get::blue(hsv));

		MediaTrack* track = GetTrack(0, track_index);
		SetTrackColor(track, color);
	}
}

static void insert_ready_to_create_track(){
	int index = last_selected_track_index() + 1;
	MediaTrack* new_track = insert_track(index, "Track", 0);

	insert_plugin(new_track, "ReaEQ (Cockos)");
	insert_plugin(new_track, "PurestGain (airwindows)");
	insert_plugin(new_track, "Channel9 (airwindows)");

	color_tracks();
}

static void insert_ready_to_create_folder(){
	int index = last_selected_track_index() + 1;
	MediaTrack* new_track = insert_track(index, "Folder", 0);

	insert_plugin(new_track, "ReaEQ (Cockos)");
	insert_plugin(new_track, "Logical4 (airwindows)");
	insert_plugin(new_track, "Channel9 (airwindows)");

	color_tracks();
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
	},
	{
		0,
		{
			static_cast< int >( Section::Main ), // section that this action appears
			"kazzix_insert_ready_to_create_folder", // action id
			"Kazzix: Insert a folder and add some essential FX", // description
			0, // reserved
		},
		insert_ready_to_create_folder,
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