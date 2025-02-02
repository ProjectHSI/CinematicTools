#include "UI.h"
#include "Main.h"
#include "Util/Util.h"
#include "Util/ImGuiEXT.h"
#include "imgui/imgui_impl_dx11.h"
#include "resource.h"

#include <winerror.h>
#include <WICTextureLoader.h>
#pragma comment(lib, "DirectXTK.lib")

const char* g_creditsText = ""
"The following libraries are used in making the Cinematic Tools:\n"
" - MinHook by Tsuda Kageyu, inih by Ben Hoyt\n"
" - ImGui by Omar Cornut\n"
" - C++ libraries by Boost\n"
" - DirectXTK and DirectXTex by Microsoft.\n\n"
"Thank you for using the tools and making awesome stuff with it!\n\n"
"Remember to report bugs at www.cinetools.xyz/bugs\n\n\n";

UI::UI() :
  m_Enabled(false),
  m_FramesToSkip(0),
  m_HasKeyboardFocus(false),
  m_HasMouseFocus(false),
  m_IsResizing(false),
  m_pRTV(nullptr)
{
}

UI::~UI()
{
  ImGui_ImplDX11_Shutdown();
  ImGui::DestroyContext();
}

bool UI::Initialize()
{
  /////////////////////////
  // ImGui Configuration //
  /////////////////////////

  // ImGui is an immensely useful and simple-to-use UI system made by Omar Cornut.
  // Definitely check it out:
  // https://github.com/ocornut/imgui

  ImGui::CreateContext();
  if (!ImGui_ImplDX11_Init(g_gameHwnd, g_d3d11Device, g_d3d11Context))
  {
    util::log::Error("ImGui Dx11 implementation failed to initialize");
    return false;
  }

  ImGuiStyle& Style = ImGui::GetStyle();
  Style.WindowRounding = 0.0f;
  Style.ChildRounding = 0.0f;
  Style.WindowPadding = ImVec2(0, 0);
  Style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  Style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  Style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  Style.Colors[ImGuiCol_Button] = ImVec4(.2f, .2f, .2f, 1.0f);
  Style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);
  Style.Colors[ImGuiCol_ButtonActive] = ImVec4(1, 1, 1, 1);
  Style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0, 0, 0, 0.7f);
  Style.Colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
  Style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

  ///////////////////////////
  // Font loading & config //
  ///////////////////////////

  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontDefault();

  ImFontConfig fontConfig;
  fontConfig.OversampleH = 8;
  fontConfig.OversampleV = 8;
  fontConfig.FontDataOwnedByAtlas = false;

  ImFontConfig fontOffsetConfig = fontConfig;
  fontOffsetConfig.GlyphOffset = ImVec2(0, -2);

  void* pData;
  DWORD szData;

  if (!util::GetResource(IDR_FONT_PURISTA, pData, szData))
  {
    util::log::Error("Failed to load IDR_FONT_PURISTASEMI from resources");
    return false;
  }

  io.Fonts->AddFontFromMemoryTTF(pData, szData, 28, &fontConfig);
  io.Fonts->AddFontFromMemoryTTF(pData, szData, 24, &fontOffsetConfig);
  io.Fonts->AddFontFromMemoryTTF(pData, szData, 22, &fontOffsetConfig);

  if (!util::GetResource(IDR_FONT_SEGOE, pData, szData))
  {
    util::log::Error("Failed to load IDR_FONT_SEGOEUI from resources");
    return false;
  }

  io.Fonts->AddFontFromMemoryTTF(pData, szData, 20, &fontConfig);
  io.Fonts->AddFontFromMemoryTTF(pData, szData, 24, &fontConfig);


  //////////////////////////////////////
  // Background & title image loading //
  //////////////////////////////////////

  m_TitleImage = g_mainHandle->GetRenderer()->CreateImageFromResource(IDR_IMG_TITLE);
  for (int i = IDR_IMG_BG1; i <= IDR_IMG_BG1; ++i)
    m_BgImages.emplace_back(g_mainHandle->GetRenderer()->CreateImageFromResource(i));

   if (!CreateRenderTarget())
     return false;

  util::log::Ok("UI Initialized");
  UI::Toggle();
  return true;
}

void UI::BindRenderTarget()
{
  if (m_IsResizing || !m_pRTV) return;

  ID3D11RenderTargetView* pRtv = m_pRTV.Get();
  g_d3d11Context->OMSetRenderTargets(1, &pRtv, nullptr);
}

float g_UIScale = 1.4f;

static ImVec2 ScaledImVec2(float x, float y)
{
  return ImVec2(x*g_UIScale, y*g_UIScale);
}


void UI::Draw()
{
  if (!m_Enabled) return;
  if (m_IsResizing)
  {
    // I think it's a good idea to skip some frames after a resize
    // juuuuust to make sure the game's done with resizing stuff.

    m_FramesToSkip -= 1;
    if (m_FramesToSkip == 0)
    {
      m_IsResizing = false;
      CreateRenderTarget();
    }
    return;
  }

  g_d3d11Context->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);

  ImGuiIO& io = ImGui::GetIO();
  ImGui_ImplDX11_NewFrame();

  ImGui::SetNextWindowSize(ScaledImVec2(800, 460));
  ImGui::Begin("Cinematic Tools", nullptr, ImGuiWindowFlags_NoResize);
  {
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImGui::Dummy(ImVec2(0, 20));

    ImGui::GetWindowDrawList()->AddImage(m_BgImages[0].pSRV.Get(), ImVec2(windowPos.x, windowPos.y + 19), ImVec2(windowPos.x + 1120, windowPos.y + 644));
    //if (m_isFading)
    //  ImGui::GetWindowDrawList()->AddImage(m_bgImages[m_nextIndex].pShaderResourceView.Get(), ImVec2(windowPos.x, windowPos.y + 19), ImVec2(windowPos.x + 800, windowPos.y + 460),
    //    ImVec2(0, 0), ImVec2(1, 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, m_opacity)));

    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(windowPos.x, windowPos.y + 19), ImVec2(windowPos.x + 1120, windowPos.y + 644), 0x10000000);
    ImGui::GetWindowDrawList()->AddImage(m_TitleImage.pSRV.Get(), ImVec2(windowPos.x + 369, windowPos.y + 85), ImVec2(windowPos.x + 751, windowPos.y + 131));

    ImGui::PushFont(io.Fonts->Fonts[2]);
    ImGui::Dummy(ImVec2(143, 33));
    ImGui::SameLine(0, 0);

    if (ImGui::ToggleButton("CAMERA", ImVec2(158, 33), m_SelectedMenu == UIMenu_Camera, false))
      m_SelectedMenu = UIMenu_Camera;

    ImGui::SameLine(0, 20);
    if (ImGui::ToggleButton("VISUALS", ImVec2(158, 33), m_SelectedMenu == UIMenu_Visuals, false))
      m_SelectedMenu = UIMenu_Visuals;

    ImGui::SameLine(0, 20);
    if (ImGui::ToggleButton("MISC", ImVec2(158, 33), m_SelectedMenu == UIMenu_Misc, false))
      m_SelectedMenu = UIMenu_Misc;

    ImGui::PopFont();

    ImGui::GetWindowDrawList()->AddLine(ImVec2(windowPos.x + 143, windowPos.y + 76), ImVec2(windowPos.x + 301, windowPos.y + 76), 0xFF1C79E5, 2);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(windowPos.x + 321, windowPos.y + 76), ImVec2(windowPos.x + 479, windowPos.y + 76), 0xFF1C79E5, 2);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(windowPos.x + 499, windowPos.y + 76), ImVec2(windowPos.x + 657, windowPos.y + 76), 0xFF1C79E5, 2);

    ImGui::Dummy(ImVec2(0, 50));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::SameLine(10);

    {
      ImGui::BeginChild("contentChild", ImVec2(-10, -10), false);
      
      if (m_SelectedMenu == UIMenu_Camera)
        g_mainHandle->GetCameraManager()->DrawUI();
      else if (m_SelectedMenu == UIMenu_Visuals)
      {
        g_mainHandle->GetVisualsController()->DrawUI();
      }
      else if (m_SelectedMenu == UIMenu_Misc)
      {
        ImGui::Columns(3, "miscColumns", false);
        ImGui::NextColumn();
        ImGui::SetColumnOffset(-1, 5);

        ImGui::PushFont(io.Fonts->Fonts[4]);
        ImGui::PushItemWidth(130);

        ImGui::PushFont(io.Fonts->Fonts[3]);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 1));
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::DrawWithBorders([=]
        {
          if (ImGui::Button("Config", ImVec2(158, 33)))
            g_mainHandle->GetInputSystem()->ShowUI();

        });

        ImGui::PopStyleColor();
        ImGui::PopFont();
        
        ImGui::NextColumn();
        ImGui::SetColumnOffset(-1, 388.5f);

        ImGui::PushFont(io.Fonts->Fonts[2]);
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Text("CREDITS"); 
        ImGui::PopFont();

        ImGui::TextWrapped(g_creditsText);

        ImGui::PopFont();
      }
      ImGui::EndChild();
    }

  } ImGui::End();

  g_mainHandle->GetInputSystem()->DrawUI();

  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  m_HasKeyboardFocus = ImGui::GetIO().WantCaptureKeyboard;
  m_HasMouseFocus = ImGui::GetIO().WantCaptureMouse;
}

void UI::OnResize()
{
  // Backbuffer needs to be resized when the window size/resolution changes.
  // However it's bound to the UI's RenderTarget (CreateRenderTarget()), so
  // it needs to be released before the backbuffer can be resized.
  // Failing to handle this will usually result in a crash.

  m_IsResizing = true;
  m_FramesToSkip = 100;
  if (m_pRTV)
    m_pRTV.Reset();
}

void UI::Update(float dt)
{
  // For fancy background fading effects
}

void UI::Toggle()
{
  m_Enabled = !m_Enabled;
  ClipCursor(NULL);
  CATHODE::ShowMouse(m_Enabled);
}

bool UI::CreateRenderTarget()
{
  // Creates a render target to backbuffer resource
  // Should guarantee that stuff actually gets drawn

  ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
  HRESULT hr = g_dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)pBackBuffer.ReleaseAndGetAddressOf());
  if (FAILED(hr) || pBackBuffer == nullptr)
  {
    util::log::Error("Failed to retrieve backbuffer from SwapChain, HRESULT 0x%X", hr);
    return false;
  }

  hr = g_d3d11Device->CreateRenderTargetView(pBackBuffer.Get(), NULL, m_pRTV.ReleaseAndGetAddressOf());
  if (FAILED(hr))
  {
    util::log::Error("CreateRenderTargetView failed, HRESULT 0x%X", hr);
    return false;
  }

  return true;
}
