"MainMenu"
{	
	"ResumeGame"
	{
		"text"			"#GameUI2_ResumeGame"
		"description"	"#GameUI2_ResumeGameDescription"
		"command"		"cvar gamemenucommand resumegame"
		"priority"		"7"
		"group"			"1"
		"specifics"		"ingame"
	}

	"ReturnToMainMenu"
	{
		"text"			"#GameUI2_ReturnToMainMenu"
		"description"	"#GameUI2_ReturnToMainMenuDescription"
		"command"		"cvar disconnect"
		"priority"		"6"
		"group"			"1"
		"specifics"		"ingame"
	}
	
	"SaveGame"
	{
		"text"			"#GameUI2_SaveGame"
		"description"	"#GameUI2_SaveGameDescription"
		"command"		"cvar gamemenucommand opensavegamedialog"
		"priority"		"5"
		"group"			"2"
		"specifics"		"ingame"
	}
	
	"NewGame"
	{
		"text"			"#GameUI2_NewGame"
		"description"	"#GameUI2_NewGameDescription"
		"command"		"cvar gamemenucommand opennewgamedialog"
		"priority"		"5"
		"group"			"2"
		"specifics"		"mainmenu"
	}
	
	"LoadGame"
	{
		"text"			"#GameUI2_LoadGame"
		"description"	"#GameUI2_LoadGameDescription"
		"command"		"cvar gamemenucommand openloadgamedialog"
		"priority"		"4"
		"group"			"2"
		"specifics"		"shared"
	}
	
	"Options"
	{
		"text"			"#GameUI2_Options"
		"description"	"#GameUI2_OptionsDescription"
		"command"		"cvar gamemenucommand openoptionsdialog"
	//	"command"		"cvar gameui2_openoptionsdialog"
		"priority"		"3"
		"group"			"3"
		"specifics"		"shared"
	}

	"Quit"
	{
		"text"			"#GameUI2_Quit"
		"description"	"#GameUI2_QuitDescription"
		"command"		"cvar gamemenucommand quit"
	//	"command"		"cvar gameui2_openquitgamedialog"
		"priority"		"2"
		"group"			"3"
		"specifics"		"shared"
	}
}