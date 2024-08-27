#include <iostream>     // Standard input-output stream library
#include <fstream>      // File input-output stream library
#define CPPHTTPLIB_OPENSSL_SUPPORT  // Define to enable OpenSSL support in httplib
#include "json.hpp"
#include "httplib.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <d3d9.h>
#include <tchar.h>
#include <string>
#include <map>
#include <thread>
#include <mutex>



// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static bool                     g_DeviceLost = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static char search_keyword[128] = "";// מילה לחיפוש
std::vector<nlohmann::json> filtered_data; // נתונים מסוננים
std::vector<nlohmann::json> data;// נתונים גולמיים

void printString(std::string str) //  פונקציה להדפסת מחרוזת
{
    std::cout << str << '\n';
}


std::mutex mtx; //  Mutex לסנכרון גישה קריטית

void print_task(int id) {
    //// נעילת ה-mutex לפני הכניסה לחלק הקריטי
    std::lock_guard<std::mutex> lock(mtx);
    // Critical section
    std::cout << "Thread " << id << " is working.\n";// הדפס הודעה עם מזהה התהליך
    // ה-mutex משתחרר אוטומטית כאשר ה-lock_guard יוצא מהתחום
}


nlohmann::json httpRequest() {
    std::string api_key{ "b29f3e0bfd57489d9ed8bf46ca39866c" };// מפתח API
    std::string server = "newsapi.org";// כתובת השרת

    httplib::SSLClient cli(server, 443);// יצירת לקוח SSL
    std::string get_all = "/v2/everything?q=apple&from=2024-08-01&sortBy=popularity&apiKey=" + api_key;
    auto res = cli.Get(get_all.c_str()); // שלח בקשת GET

    if (res) {
        if (res->status == 200) {
            nlohmann::json json_response = nlohmann::json::parse(res->body);// ניתוח תגובת JSON
            return json_response;  // החזר את ה-JSON ישירות
        }
        else {
            std::cout << "GET request failed with status: " << res->status << std::endl;// הודעת שגיאה אם הסטטוס אינו 200
            return {};
        }
    }
    else {
        auto err = res.error();// קבלת השגיאה אם הבקשה נכשלה
        std::cout << "GET request failed with error: " << httplib::to_string(err) << std::endl;
        return {};
    }
}

void printJson(const nlohmann::json& json_obj, int indent = 0) {
    if (json_obj.is_object()) {
        for (auto it = json_obj.begin(); it != json_obj.end(); ++it) {
            std::cout << std::string(indent, ' ') << it.key() << ": ";
            if (it->is_object() || it->is_array()) {
                std::cout << std::endl;
                printJson(it.value(), indent + 4);// קריאה רקורסיבית עבור אובייקטים או מערכים
            }
            else {
                std::cout << it.value() << std::endl;
            }
        }
    }
    else if (json_obj.is_array()) {
        for (const auto& item : json_obj) {
            if (item.is_object() || item.is_array()) {
                printJson(item, indent + 4);// קריאה רקורסיבית עבור אובייקטים או מערכים
            }
            else {
                std::cout << std::string(indent, ' ') << item << std::endl;// הדפס את הפריט
            }
        }
    }
}

// פונקציה לסינון נתונים
void filterData(const std::string& keyword) {
    filtered_data.clear(); // ניקוי הנתונים מהטבלה
    if (keyword.empty()) {
        // אם מילת המפתח ריקה, פשוט לא להוסיף נתונים
        std::cout << "Search keyword is empty." << std::endl;
        return;
    }
    for (const auto& item : data) {
        std::string title = item.value("title", "");
        std::string description = item.value("description", "");
        if (title.find(keyword) != std::string::npos || description.find(keyword) != std::string::npos) {
            filtered_data.push_back(item);
        }
    }
    if (filtered_data.empty()) {
        std::cout << "No results found for keyword: " << keyword << std::endl;
    }
}
// Function to save titles to a file
void saveTitlesToFile(const std::vector<nlohmann::json>& filtered_data) {
    std::ofstream file("saved_titles.txt"); // Open file for writing
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return;
    }

    for (const auto& item : filtered_data) {
        std::string title = item.value("title", "");
        if (!title.empty()) {
            file << title << std::endl; // Write title to file
        }
    }

    file.close(); // Close the file
    std::cout << "Titles saved to 'saved_titles.txt'" << std::endl;
}


// Main code
int main(int, char**)
{

    nlohmann::json response = httpRequest();// שלח בקשה ל-API וקבל את התגובה
    std::thread t1(print_task, 1);
    std::thread t2(print_task, 2);
    std::thread t3(print_task, 3);

    // Wait for the threads to finish
    t1.join();
    t2.join();
    t3.join();

    // הדפסת התגובה כדי לראות מה התקבל
    std::cout << response.dump(4) << std::endl;  // הדפס את ה-JSON המתקבל באופן קריא עם הזחה

    // בדיקה אם השדה 'articles' קיים והאם הוא מערך
    if (response.contains("articles") && response["articles"].is_array()) {
        data = response["articles"].get<std::vector<nlohmann::json>>();
    }
    else {
        std::cout << "Error: 'articles' field is missing or not an array." << std::endl;
    }


    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX9 Example", WS_OVERLAPPEDWINDOW, 0, 0, 1500, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);


    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    //0.45f, 0.55f, 0.60f, 1.00f

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

    
        // Begin main window

        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::BeginChild("Search", ImVec2(300, 200), true);
        ImGui::Text("Search by keyword");
        ImGui::InputText("##search", search_keyword, IM_ARRAYSIZE(search_keyword));
        if (ImGui::Button("Search"))
        {
            std::string keyword(search_keyword);
            std::cout << "Search button pressed. Keyword: " << keyword << std::endl;
            filterData(keyword); // Trigger search
        }

        // Add the Save for Later button directly below the Search button
        if (ImGui::Button("Save for Later"))
        {
            saveTitlesToFile(filtered_data);
        }

        ImGui::EndChild();

        // Same table as before, but in the same window
        ImGui::SameLine();
        ImGui::BeginChild("Articles", ImVec2(1000, 600), true);
        ImGui::BeginTable("Articles", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti);
        ImGui::TableSetupColumn("author", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending);
        ImGui::TableSetupColumn("title", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("url", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("urlToImage", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("publishedAt", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        if (filtered_data.empty()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("No results found.");
        }


        else {
            for (const auto& news_data : filtered_data) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                if (!news_data["author"].is_null()) {
                    ImGui::TextWrapped("%s", news_data["author"].get<std::string>().c_str());
                }
                else {
                    ImGui::TextWrapped("N/A");
                }

                ImGui::TableNextColumn();
                if (!news_data["title"].is_null()) {
                    ImGui::TextWrapped("%s", news_data["title"].get<std::string>().c_str());
                }
                else {
                    ImGui::TextWrapped("N/A");
                }

                ImGui::TableNextColumn();
                if (!news_data["description"].is_null()) {
                    ImGui::TextWrapped("%s", news_data["description"].get<std::string>().c_str());
                }
                else {
                    ImGui::TextWrapped("N/A");
                }

                ImGui::TableNextColumn();
                if (!news_data["url"].is_null()) {
                    ImGui::TextWrapped("%s", news_data["url"].get<std::string>().c_str());
                }
                else {
                    ImGui::TextWrapped("N/A");
                }

                ImGui::TableNextColumn();
                if (!news_data["urlToImage"].is_null()) {
                    ImGui::TextWrapped("%s", news_data["urlToImage"].get<std::string>().c_str());
                }
                else {
                    ImGui::TextWrapped("N/A");
                }

                ImGui::TableNextColumn();
                if (!news_data["publishedAt"].is_null()) {
                    ImGui::TextWrapped("%s", news_data["publishedAt"].get<std::string>().c_str());
                }
                else {
                    ImGui::TextWrapped("N/A");
                }

                ImGui::TableNextColumn();
                if (!news_data["content"].is_null()) {
                    ImGui::TextWrapped("%s", news_data["content"].get<std::string>().c_str());
                }
                else {
                    ImGui::TextWrapped("N/A");
                }
            }
        }

        ImGui::EndTable();
        ImGui::EndChild();
        ImGui::End();



        // Rendering
        ImGui::EndFrame();


        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST)
            g_DeviceLost = true;
    }

    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
