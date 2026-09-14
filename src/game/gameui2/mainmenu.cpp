/*
Hyperborea (c) by Nicolas @ https://github.com/NicolasDe

Hyperborea is licensed under a
Creative Commons Attribution-ShareAlike 4.0 International License.

You should have received a copy of the license along with this
work.  If not, see <http://creativecommons.org/licenses/by-sa/4.0/>.
*/
#include "gameui2_interface.h"
#include "basepanel.h"
#include "mainmenu.h"

#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui/IVGui.h"

#include "KeyValues.h"
#include "filesystem.h"

#include <ctime>
#include <cstring>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int32 __cdecl ButtonsPositionBottom(Button_MainMenu* const* s1, Button_MainMenu* const* s2)
{
	return ((*s1)->GetPriority() > (*s2)->GetPriority());
}

int32 __cdecl ButtonsPositionTop(Button_MainMenu* const* s1, Button_MainMenu* const* s2)
{
	return ((*s1)->GetPriority() < (*s2)->GetPriority());
}

MainMenu::MainMenu(vgui::Panel* Parent) : BaseClass(nullptr, "MainMenu")
{
	vgui::HScheme Scheme = vgui::scheme()->LoadSchemeFromFile("resource2/schememainmenu.res", "SchemeMainMenu");
	SetScheme(Scheme);

	SetProportional(false);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetDeleteSelfOnClose(true);
	SetSizeable(false);
	SetMoveable(false);
	SetCloseButtonVisible(false);
	SetMenuButtonVisible(false);

	bIsVisible = false;

	bWasInGame = false;
	bWasInBackgroundLevel = false;

	LogoLeftText = GetGameUI2().ConvertToLocalizedString("#GameUI2_LogoLeft");
	LogoRightText = GetGameUI2().ConvertToLocalizedString("#GameUI2_LogoRight");

	// build info "buildno - Compiled at: dd/mm/yyyy hh:mmAM/PM"
	{
		wchar_t* BuildLabel = GetGameUI2().ConvertToLocalizedString("#GameUI2_BuildLabel");

		// parse __DATE__ ("mmm dd yyyy") and __TIME__ ("hh:mm:ss")
		const char* compDate = __DATE__;
		const char* compTime = __TIME__;

		const char* months[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
		int month = 1;
		for (int i = 0; i < 12; i++)
		{
			if (strncmp(compDate, months[i], 3) == 0)
			{
				month = i + 1;
				break;
			}
		}

		int day = atoi(compDate + 4);
		int year = atoi(compDate + 7);

		int hour24 = atoi(compTime);
		int minute = atoi(compTime + 3);

		const char* ampm = (hour24 >= 12) ? "PM" : "AM";
		int hour12 = hour24 % 12;
		if (hour12 == 0) hour12 = 12;

		char buildInfoAnsi[256];
		Q_snprintf(buildInfoAnsi, sizeof(buildInfoAnsi),
			" - Compiled at: %02d/%02d/%04d %d:%02d%s",
			day, month, year, hour12, minute, ampm);

		// combine BuildLabel + compile info
		wchar_t compilePart[256];
		mbstowcs(compilePart, buildInfoAnsi, sizeof(compilePart) / sizeof(wchar_t));

		wcscpy(BuildInfoText, BuildLabel);
		wcscat(BuildInfoText, compilePart);

		free(BuildLabel);
	}

	CreateMenu("resource2/mainmenu.res");
}

void MainMenu::CreateMenu(const char* MenuScript)
{
	KeyValues* DataFile = new KeyValues("MainMenu");
	DataFile->UsesEscapeSequences(true);
	if (DataFile->LoadFromFile(g_pFullFileSystem, MenuScript) == true)
	{
		for (KeyValues* Data = DataFile->GetFirstSubKey(); Data != nullptr; Data = Data->GetNextKey())
		{
			Button_MainMenu* Button = new Button_MainMenu(this, this, Data->GetString("command", ""));
			Button->SetPriority(atoi(Data->GetString("priority", "1")));
			Button->SetGroup(atoi(Data->GetString("group", "0")));
			Button->SetButtonText(Data->GetString("text", "no text"));
			Button->SetButtonDescription(Data->GetString("description", "no description"));

			const char* Specifics = Data->GetString("specifics", "shared");
			if (Q_stristr(Specifics, "ingame"))
			{
				ButtonsInGame.AddToTail(Button);
			}
			else if (Q_stristr(Specifics, "mainmenu"))
			{
				ButtonsBackground.AddToTail(Button);
			}
			else if (Q_stristr(Specifics, "shared"))
			{
				ButtonsShared.AddToTail(Button);
			}
		}
	}
}

void MainMenu::ApplySchemeSettings(vgui::IScheme* Scheme)
{
	BaseClass::ApplySchemeSettings(Scheme);

	ConColorMsg(Color(0, 148, 0, 255), "Loading main menu scheme...\n");
	
	ButtonsSpace = atof(Scheme->GetResourceString("MainMenu.Buttons.Space"));
	ButtonsOffsetX = atof(Scheme->GetResourceString("MainMenu.Buttons.OffsetX"));
	ButtonsOffsetY = atof(Scheme->GetResourceString("MainMenu.Buttons.OffsetY"));
	LogoOffsetX = atof(Scheme->GetResourceString("MainMenu.Logo.OffsetX"));
	LogoOffsetY = atof(Scheme->GetResourceString("MainMenu.Logo.OffsetY"));
	LogoLeftColor = GetSchemeColor("MainMenu.Logo.Left", Scheme);
	LogoRightColor = GetSchemeColor("MainMenu.Logo.Right", Scheme);
	bLogoAttachToMenu = atoi(Scheme->GetResourceString("MainMenu.Logo.AttachToMenu"));
	bLogoAlignToLeft = atoi(Scheme->GetResourceString("MainMenu.Logo.AlignToLeft"));
	LogoFont = Scheme->GetFont("MainMenu.Logo.Font");

	LogoSubtitleFont = Scheme->GetFont("MainMenu.Subtitle.Font");
	LogoSubtitleColor = GetSchemeColor("MainMenu.Subtitle", Scheme);
	LogoSubtitleOffsetX = atof(Scheme->GetResourceString("MainMenu.Subtitle.OffsetX"));
	LogoSubtitleOffsetY = atof(Scheme->GetResourceString("MainMenu.Subtitle.OffsetY"));

	BuildInfoFont = Scheme->GetFont("MainMenu.BuildInfo.Font");
	BuildInfoColor = GetSchemeColor("MainMenu.BuildInfo", Scheme);
	BuildInfoOffsetX = atof(Scheme->GetResourceString("MainMenu.BuildInfo.OffsetX"));
	BuildInfoOffsetY = atof(Scheme->GetResourceString("MainMenu.BuildInfo.OffsetY"));

	DividerColor = GetSchemeColor("MainMenu.Divider", Scheme);
	DividerWidth = atof(Scheme->GetResourceString("MainMenu.Divider.Width"));
	DividerSpacing = atof(Scheme->GetResourceString("MainMenu.Divider.Spacing"));
}

void MainMenu::OnThink()
{
	BaseClass::OnThink();

	SetBounds(0, 0, GetGameUI2().GetViewport().x, GetGameUI2().GetViewport().y);

	bool bInGame = GetGameUI2().IsInLevel();
	bool bInBackground = GetGameUI2().IsInBackgroundLevel();

	if (bWasInGame != bInGame || bWasInBackgroundLevel != bInBackground)
	{
		bWasInGame = bInGame;
		bWasInBackgroundLevel = bInBackground;
		UpdateMenu();
	}
}

bool MainMenu::IsVisible()
{
	if (GetGameUI2().IsInLoading() == true || GetGameUI2().GetGameUI()->IsMainMenuVisible() == false)
		return false;

	return bIsVisible;
}

void MainMenu::DrawMainMenu()
{
	if (ActiveButtons.IsEmpty() == true)
		return;

	// Position buttons from bottom to top, inserting extra spacing between groups
	for (int8 i = ActiveButtons.Count() - 1; i >= 0; i--)
	{
		if (i == ActiveButtons.Count() - 1)
		{
			// Bottom-most button
			ActiveButtons[i]->SetPos(ButtonsOffsetX, GetTall() - (ButtonsOffsetY + ActiveButtons[i]->GetTall()));
		}
		else
		{
			int32 BelowX, BelowY;
			ActiveButtons[i + 1]->GetPos(BelowX, BelowY);

			float ExtraSpace = 0;
			if (ActiveButtons[i]->GetGroup() != ActiveButtons[i + 1]->GetGroup())
				ExtraSpace = DividerSpacing;

			ActiveButtons[i]->SetPos(ButtonsOffsetX, BelowY - (ActiveButtons[i]->GetTall() + ButtonsSpace + ExtraSpace));
		}
	}

	// draw divider lines between groups
	for (int8 i = 0; i < ActiveButtons.Count() - 1; i++)
	{
		if (ActiveButtons[i]->GetGroup() != ActiveButtons[i + 1]->GetGroup())
		{
			int32 AboveX, AboveY;
			ActiveButtons[i]->GetPos(AboveX, AboveY);
			int32 BelowX, BelowY;
			ActiveButtons[i + 1]->GetPos(BelowX, BelowY);

			int32 DivY = AboveY + ActiveButtons[i]->GetTall() + (int32)(DividerSpacing * 0.5f);

			vgui::surface()->DrawSetColor(DividerColor);
			vgui::surface()->DrawFilledRect((int32)ButtonsOffsetX, DivY, (int32)(ButtonsOffsetX + DividerWidth), DivY + 1);
		}
	}
}

void MainMenu::DrawLogo()
{
	if (LogoLeftText == nullptr || LogoRightText == nullptr)
		return;
	
	vgui::surface()->DrawSetTextColor(LogoLeftColor);
	vgui::surface()->DrawSetTextFont(LogoFont);

	int32 LogoW, LogoH;
	vgui::surface()->GetTextSize(LogoFont, LogoLeftText, LogoW, LogoH);

	int32 LogoX, LogoY;
	if (ActiveButtons.IsEmpty() == true || bLogoAttachToMenu == false)
	{
		if (bLogoAlignToLeft == false)
		{
			wchar_t FullLogoText[256];
			wcscpy(FullLogoText, LogoLeftText);
			wcscat(FullLogoText, LogoRightText);

			int32 FullLogoW, FullLogoH;
			vgui::surface()->GetTextSize(LogoFont, FullLogoText, FullLogoW, FullLogoH);
			
			LogoX = GetWide() - (LogoOffsetX + FullLogoW);
		}
		else
		{
			LogoX = LogoOffsetX;
		}

		LogoY = GetTall() - (LogoOffsetY + LogoH);
	}
	else
	{
		int32 ButtonPositionX, ButtonPositionY;
		ActiveButtons[0]->GetPos(ButtonPositionX, ButtonPositionY);
		LogoX = ButtonsOffsetX + LogoOffsetX;
		LogoY = ButtonPositionY - (LogoH + LogoOffsetY);
	}

	vgui::surface()->DrawSetTextPos(LogoX, LogoY);
	vgui::surface()->DrawPrintText(LogoLeftText, wcslen(LogoLeftText));
}

void MainMenu::DrawSubtitle()
{
	if (LogoRightText == nullptr)
		return;

	vgui::surface()->DrawSetTextColor(LogoSubtitleColor);
	vgui::surface()->DrawSetTextFont(LogoSubtitleFont);

	int32 SubW, SubH;
	vgui::surface()->GetTextSize(LogoSubtitleFont, LogoRightText, SubW, SubH);

	int32 SubX = (int32)LogoSubtitleOffsetX;
	int32 SubY = GetTall() - ((int32)LogoSubtitleOffsetY + SubH);

	vgui::surface()->DrawSetTextPos(SubX, SubY);
	vgui::surface()->DrawPrintText(LogoRightText, wcslen(LogoRightText));
}

void MainMenu::Paint()
{
	BaseClass::Paint();

	// TODO: Add script settings
	vgui::surface()->DrawSetColor(Color(0, 0, 0, 255));
	vgui::surface()->DrawFilledRectFade(0, 0, GetWide(), GetTall(), 255, 0, true);

	DrawMainMenu();
	DrawLogo();
	DrawSubtitle();
	DrawBuildInfo();
}

void MainMenu::DrawBuildInfo()
{
	if (BuildInfoText[0] == L'\0')
		return;

	vgui::surface()->DrawSetTextColor(BuildInfoColor);
	vgui::surface()->DrawSetTextFont(BuildInfoFont);

	int32 InfoW, InfoH;
	vgui::surface()->GetTextSize(BuildInfoFont, BuildInfoText, InfoW, InfoH);

	int32 InfoX = GetWide() - ((int32)BuildInfoOffsetX + InfoW);
	int32 InfoY = GetTall() - ((int32)BuildInfoOffsetY + InfoH);

	vgui::surface()->DrawSetTextPos(InfoX, InfoY);
	vgui::surface()->DrawPrintText(BuildInfoText, wcslen(BuildInfoText));
}

void MainMenu::OnCommand(char const* Command)
{
	if (Q_stristr(Command, "cvar"))
	{
		const char* EngineCommand = strstr(Command, "cvar ") + strlen("cvar ");
		if (strlen(EngineCommand) > 0)
			GetGameUI2().GetEngineClient()->ClientCmd_Unrestricted(const_cast<char*>(EngineCommand));
	}
	else
	{
		BaseClass::OnCommand(Command);
	}
}

void MainMenu::OnSetFocus()
{
	BaseClass::OnSetFocus();

	UpdateMenu();
	
	bIsVisible = true;
	vgui::surface()->PlaySound("interface/ui/menu_open.wav");
}

void MainMenu::OnKillFocus()
{
	BaseClass::OnKillFocus();

	bIsVisible = false;
	vgui::surface()->PlaySound("interface/ui/menu_close.wav");
}

void MainMenu::UpdateMenu()
{
	for (int8 i = 0; i < ButtonsInGame.Count(); i++)
		ButtonsInGame[i]->SetVisible(GetGameUI2().IsInLevel());

	for (int8 i = 0; i < ButtonsBackground.Count(); i++)
		ButtonsBackground[i]->SetVisible(GetGameUI2().IsInBackgroundLevel());

	for (int8 i = 0; i < ButtonsShared.Count(); i++)
		ButtonsShared[i]->SetVisible(GetGameUI2().IsInLevel() || GetGameUI2().IsInBackgroundLevel());

	CUtlVector<Button_MainMenu*> UpdatedActiveButtons;

	if (GetGameUI2().IsInLevel() == true)
	{
		if (ButtonsInGame.IsEmpty() == false)
			UpdatedActiveButtons.AddVectorToTail(ButtonsInGame);
	}
	
	if (GetGameUI2().IsInBackgroundLevel() == true)
	{
		if (ButtonsBackground.IsEmpty() == false)
			UpdatedActiveButtons.AddVectorToTail(ButtonsBackground);
	}

	if (ButtonsShared.IsEmpty() == false)
		UpdatedActiveButtons.AddVectorToTail(ButtonsShared);

	if (UpdatedActiveButtons.IsEmpty() == false)
		UpdatedActiveButtons.Sort(ButtonsPositionTop);

	ActiveButtons = UpdatedActiveButtons;

	ConColorMsg(Color(0, 148, 0, 255), "Updating main menu...\n");
}

MainMenuHelper::MainMenuHelper(MainMenu* Menu, vgui::Panel* Parent) : BaseClass(Parent)
{
	Menu->SetParent(this);
	Menu->MakeReadyForUse();
	Menu->SetZPos(0);
}