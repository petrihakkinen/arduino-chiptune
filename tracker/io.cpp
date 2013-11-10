#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <Windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include "../playroutine.h"

std::string fileDialog( const char* title, bool save ) 
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME) );

	char file[260] = "";

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = 0;
	ofn.lpstrFile = file;
	ofn.nMaxFile = sizeof(file);
	ofn.lpstrFilter = "Tracker Files (*.tracker)\0*.tracker";
	ofn.nFilterIndex = 0;
	ofn.lpstrFileTitle = 0;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = "";
	ofn.lpstrDefExt = "*.tracker";
	ofn.lpstrTitle = title;
	if( !save )
		ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON;
	else
		ofn.Flags = OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON;

	if( !save )
	{
		if( GetOpenFileName( &ofn ) )
			return file;
	}
	else
	{
		if( GetSaveFileName( &ofn ) )
			return file;
	}

	return "";
}

void loadSong()
{
	std::string filename = fileDialog( "Open Tracker File", false );
	if( filename.empty() )
		return;

	FILE* fp = fopen( filename.c_str(), "r" );
	fread( tracks, sizeof( tracks ), 1, fp );
	fread( &song, sizeof( song ), 1, fp );
	fread( instruments, sizeof( instruments ), 1, fp );
	fread( &tempo, sizeof( tempo ), 1, fp );
	fclose( fp );
}

void saveSong()
{
	std::string filename = fileDialog( "Save Tracker File As", true );
	if( filename.empty() )
		return;

	FILE* fp = fopen( filename.c_str(), "w" );
	fwrite( tracks, sizeof( tracks ), 1, fp );
	fwrite( &song, sizeof( song ), 1, fp );
	fwrite( instruments, sizeof( instruments ), 1, fp );
	fwrite( &tempo, sizeof( tempo ), 1, fp );
	fclose( fp );
}

void exportSong()
{
	std::string filename = fileDialog( "Export Song As", true );
	if( filename.empty() )
		return;

	// TODO
}
