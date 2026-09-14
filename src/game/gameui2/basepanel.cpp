/*
Hyperborea (c) by Nicolas @ https://github.com/NicolasDe

Hyperborea is licensed under a
Creative Commons Attribution-ShareAlike 4.0 International License.

You should have received a copy of the license along with this
work.  If not, see <http://creativecommons.org/licenses/by-sa/4.0/>.
*/
#include "gameui2_interface.h"
#include "basepanel.h"

#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"

#include "tier0/icommandline.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "texture_group_names.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar gameui2_background_music_duck("gameui2_background_music_duck", "0.15", FCVAR_ARCHIVE);
ConVar gameui2_background_effects("gameui2_background_effects", "1", FCVAR_ARCHIVE,
	"Blur the last completed world frame behind the in-game menu.");

static CMaterialReference s_GameUI2BlurX;
static CMaterialReference s_GameUI2BlurY;
static CMaterialReference s_GameUI2Composite;

static void DrawGameUI2Background(int nWidth, int nHeight)
{
	if (!gameui2_background_effects.GetBool() || !GetGameUI2().IsInLevel() ||
		!GetGameUI2().GetEngineVGui()->IsGameUIVisible())
		return;

	if (!s_GameUI2BlurX)
		s_GameUI2BlurX.Init("gameui2/bgblur_x", TEXTURE_GROUP_OTHER, true);
	if (!s_GameUI2BlurY)
		s_GameUI2BlurY.Init("gameui2/bgblur_y", TEXTURE_GROUP_OTHER, true);
	if (!s_GameUI2Composite)
		s_GameUI2Composite.Init("gameui2/blur_composite", TEXTURE_GROUP_OTHER, true);

	IMaterial* pBlurX = s_GameUI2BlurX;
	IMaterial* pBlurY = s_GameUI2BlurY;
	IMaterial* pComposite = s_GameUI2Composite;
	if (!pBlurX || !pBlurY || !pComposite ||
		pBlurX->IsErrorMaterial() || pBlurY->IsErrorMaterial() || pComposite->IsErrorMaterial())
		return;

	IMaterialSystem* pMaterialSystem = GetGameUI2().GetMaterialSystem();
	ITexture* pSnap = pMaterialSystem->FindTexture("_rt_GameUI2BG_Snap", TEXTURE_GROUP_RENDER_TARGET);
	ITexture* pTemp = pMaterialSystem->FindTexture("_rt_GameUI2BG_Temp", TEXTURE_GROUP_RENDER_TARGET);
	ITexture* pBlurred = pMaterialSystem->FindTexture("_rt_GameUI2BG_Blur", TEXTURE_GROUP_RENDER_TARGET);
	if (!pSnap || !pTemp || !pBlurred || pSnap->IsError() || pTemp->IsError() || pBlurred->IsError())
		return;

	const int nSmallWidth = pSnap->GetActualWidth();
	const int nSmallHeight = pSnap->GetActualHeight();
	if (nSmallWidth <= 0 || nSmallHeight <= 0 || nWidth <= 0 || nHeight <= 0)
		return;

	CMatRenderContextPtr pRenderContext(pMaterialSystem);

	pRenderContext->PushRenderTargetAndViewport(pTemp);
	pRenderContext->DrawScreenSpaceRectangle(pBlurX, 0, 0, nSmallWidth, nSmallHeight,
		0, 0, nSmallWidth - 1, nSmallHeight - 1, nSmallWidth, nSmallHeight);
	pRenderContext->PopRenderTargetAndViewport();

	pRenderContext->PushRenderTargetAndViewport(pBlurred);
	pRenderContext->DrawScreenSpaceRectangle(pBlurY, 0, 0, nSmallWidth, nSmallHeight,
		0, 0, nSmallWidth - 1, nSmallHeight - 1, nSmallWidth, nSmallHeight);
	pRenderContext->PopRenderTargetAndViewport();

	// Composite once, opaquely, in the GameUI paint pass. This keeps the menu
	// out of its own input and avoids the old additive grayscale brightness drift.
	pRenderContext->DrawScreenSpaceRectangle(pComposite, 0, 0, nWidth, nHeight,
		0, 0, nSmallWidth - 1, nSmallHeight - 1, nSmallWidth, nSmallHeight);
}

static BasePanel* gBasePanel;
BasePanel* GetBasePanel()
{
	return gBasePanel;
}

BasePanel::BasePanel(vgui::VPANEL Parent) : BaseClass(nullptr)
{
	SetParent(Parent);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetProportional(false);
	SetVisible(false);
	SetPostChildPaintEnabled(true);
	
	BackgroundMusic = "Interface.Music";
	BackgroundMusicGUID = 0;

	g_pVGuiLocalize->AddFile("resource2/localization/gameui2_%language%.txt");

	MainMenuPanel = new MainMenu(nullptr);
	MainMenuHelperPanel = new MainMenuHelper(MainMenuPanel, nullptr);
	MainMenuHelperPanel->SetParent(this);
}

vgui::VPANEL BasePanel::GetVPanel()
{
	return BaseClass::GetVPanel();
}

void BasePanel::Create()
{
	ConColorMsg(Color(0, 148, 255, 255), "Trying to create BasePanel...\n");

	if (gBasePanel == nullptr)
	{
		gBasePanel = new BasePanel(GetGameUI2().GetRootPanel());
		ConColorMsg(Color(0, 148, 255, 255), "BasePanel created.\n");
	}
	else
	{
		ConColorMsg(Color(0, 148, 255, 255), "BasePanel already exists.\n");
	}
}

void BasePanel::OnThink()
{
	SetBounds(0, 0, GetGameUI2().GetViewport().x, GetGameUI2().GetViewport().y);

	if (!CommandLine()->FindParm("-nostartupsound"))
	{
		if (IsBackgroundMusicPlaying() == false)
		{
			if (GetGameUI2().IsInBackgroundLevel() == true || GetGameUI2().IsInLevel() == false)
				StartBackgroundMusic(1.0f);
		}
		else if (IsBackgroundMusicPlaying() == true)
		{
			if (GetGameUI2().IsInBackgroundLevel() == false || GetGameUI2().IsInLevel() == true)
				ReleaseBackgroundMusic();
		}
	}

	BaseClass::OnThink();
}

void BasePanel::Paint()
{
	DrawGameUI2Background(GetWide(), GetTall());
	BaseClass::Paint();
}

void BasePanel::PaintBlurMask()
{
	BaseClass::PaintBlurMask();

	if (GetGameUI2().IsInLevel() == true)
	{
		vgui::surface()->DrawSetColor(Color(255, 255, 255, 255));
		vgui::surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
	}
}

bool BasePanel::IsBackgroundMusicPlaying()
{
	if (BackgroundMusic.IsEmpty() == true)
		return false;

	if (BackgroundMusicGUID == 0)
		return false;

	return GetGameUI2().GetEngineSound()->IsSoundStillPlaying(BackgroundMusicGUID);
}

bool BasePanel::StartBackgroundMusic(float Volume)
{
	if (IsBackgroundMusicPlaying() == true)
		return true;

	if (BackgroundMusic.IsEmpty() == true)
		return false;

	CSoundParameters SoundParameters;
	if (GetGameUI2().GetSoundEmitterSystemBase()->GetParametersForSound(BackgroundMusic.Get(), SoundParameters, GENDER_NONE) == false)
		return false;

	GetGameUI2().GetEngineSound()->EmitAmbientSound(SoundParameters.soundname, SoundParameters.volume * Volume, SoundParameters.pitch);
	BackgroundMusicGUID = GetGameUI2().GetEngineSound()->GetGuidForLastSoundEmitted();

	return (BackgroundMusicGUID != 0);
}

void BasePanel::UpdateBackgroundMusicVolume(float Volume)
{
	if (IsBackgroundMusicPlaying() == false)
		return;

	GetGameUI2().GetEngineSound()->SetVolumeByGuid(BackgroundMusicGUID, gameui2_background_music_duck.GetFloat() * Volume);
}

void BasePanel::ReleaseBackgroundMusic()
{
	if (BackgroundMusic.IsEmpty() == true)
		return;

	if (BackgroundMusicGUID == 0)
		return;

	GetGameUI2().GetEngineSound()->StopSoundByGuid(BackgroundMusicGUID);

	BackgroundMusicGUID = 0;
}
