#include "imgui.hpp"

namespace {

static ImGuiKey SDLKeyToImGuiKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_TAB:
            return ImGuiKey_Tab;
        case SDLK_LEFT:
            return ImGuiKey_LeftArrow;
        case SDLK_RIGHT:
            return ImGuiKey_RightArrow;
        case SDLK_UP:
            return ImGuiKey_UpArrow;
        case SDLK_DOWN:
            return ImGuiKey_DownArrow;
        case SDLK_PAGEUP:
            return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN:
            return ImGuiKey_PageDown;
        case SDLK_HOME:
            return ImGuiKey_Home;
        case SDLK_END:
            return ImGuiKey_End;
        case SDLK_INSERT:
            return ImGuiKey_Insert;
        case SDLK_DELETE:
            return ImGuiKey_Delete;
        case SDLK_BACKSPACE:
            return ImGuiKey_Backspace;
        case SDLK_SPACE:
            return ImGuiKey_Space;
        case SDLK_RETURN:
            return ImGuiKey_Enter;
        case SDLK_ESCAPE:
            return ImGuiKey_Escape;
        case SDLK_LCTRL:
            return ImGuiKey_LeftCtrl;
        case SDLK_LSHIFT:
            return ImGuiKey_LeftShift;
        case SDLK_LALT:
            return ImGuiKey_LeftAlt;
        case SDLK_LGUI:
            return ImGuiKey_LeftSuper;
        case SDLK_RCTRL:
            return ImGuiKey_RightCtrl;
        case SDLK_RSHIFT:
            return ImGuiKey_RightShift;
        case SDLK_RALT:
            return ImGuiKey_RightAlt;
        case SDLK_RGUI:
            return ImGuiKey_RightSuper;
        case SDLK_APPLICATION:
            return ImGuiKey_Menu;
        case SDLK_0:
            return ImGuiKey_0;
        case SDLK_1:
            return ImGuiKey_1;
        case SDLK_2:
            return ImGuiKey_2;
        case SDLK_3:
            return ImGuiKey_3;
        case SDLK_4:
            return ImGuiKey_4;
        case SDLK_5:
            return ImGuiKey_5;
        case SDLK_6:
            return ImGuiKey_6;
        case SDLK_7:
            return ImGuiKey_7;
        case SDLK_8:
            return ImGuiKey_8;
        case SDLK_9:
            return ImGuiKey_9;
        case SDLK_a:
            return ImGuiKey_A;
        case SDLK_b:
            return ImGuiKey_B;
        case SDLK_c:
            return ImGuiKey_C;
        case SDLK_d:
            return ImGuiKey_D;
        case SDLK_e:
            return ImGuiKey_E;
        case SDLK_f:
            return ImGuiKey_F;
        case SDLK_g:
            return ImGuiKey_G;
        case SDLK_h:
            return ImGuiKey_H;
        case SDLK_i:
            return ImGuiKey_I;
        case SDLK_j:
            return ImGuiKey_J;
        case SDLK_k:
            return ImGuiKey_K;
        case SDLK_l:
            return ImGuiKey_L;
        case SDLK_m:
            return ImGuiKey_M;
        case SDLK_n:
            return ImGuiKey_N;
        case SDLK_o:
            return ImGuiKey_O;
        case SDLK_p:
            return ImGuiKey_P;
        case SDLK_q:
            return ImGuiKey_Q;
        case SDLK_r:
            return ImGuiKey_R;
        case SDLK_s:
            return ImGuiKey_S;
        case SDLK_t:
            return ImGuiKey_T;
        case SDLK_u:
            return ImGuiKey_U;
        case SDLK_v:
            return ImGuiKey_V;
        case SDLK_w:
            return ImGuiKey_W;
        case SDLK_x:
            return ImGuiKey_X;
        case SDLK_y:
            return ImGuiKey_Y;
        case SDLK_z:
            return ImGuiKey_Z;
        case SDLK_F1:
            return ImGuiKey_F1;
        case SDLK_F2:
            return ImGuiKey_F2;
        case SDLK_F3:
            return ImGuiKey_F3;
        case SDLK_F4:
            return ImGuiKey_F4;
        case SDLK_F5:
            return ImGuiKey_F5;
        case SDLK_F6:
            return ImGuiKey_F6;
        case SDLK_F7:
            return ImGuiKey_F7;
        case SDLK_F8:
            return ImGuiKey_F8;
        case SDLK_F9:
            return ImGuiKey_F9;
        case SDLK_F10:
            return ImGuiKey_F10;
        case SDLK_F11:
            return ImGuiKey_F11;
        case SDLK_F12:
            return ImGuiKey_F12;
        default:
            return ImGuiKey_None;
    }
}

static ImGuiKey SDLScancodeToImGuiKey(SDL_Scancode scancode) {
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
        return static_cast<ImGuiKey>(ImGuiKey_A + (scancode - SDL_SCANCODE_A));

    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
        return static_cast<ImGuiKey>(ImGuiKey_1 + (scancode - SDL_SCANCODE_1));

    if (scancode == SDL_SCANCODE_0)
        return ImGuiKey_0;

    return ImGuiKey_None;
}

} // namespace

namespace gfx::ui {

Imgui::Imgui(
    win::Window& window, vk::Device& device, vk::Sampler& sampler,
    mngrs::BindlessManager& bindlessManager
) : m_window(window), m_device(device), m_bindlessManager(bindlessManager) {
    initImGui();

    createVertexBuffer();
    createIndexBuffer();

    createFontTexture(sampler);
}

Imgui::~Imgui() {
    ImGui::DestroyContext();
}

void Imgui::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    io.ClipboardUserData = &m_window;
    io.GetClipboardTextFn = getClipboardText;
    io.SetClipboardTextFn = setClipboardText;

    ImGui::StyleColorsDark();
}

const char* Imgui::getClipboardText(void* userData) {
    win::Window* window = static_cast<win::Window*>(userData);
    return window->getClipboardString();
}

void Imgui::setClipboardText(void* userData, const char* text) {
    win::Window* window = static_cast<win::Window*>(userData);
    window->setClipboardString(text);
}

void Imgui::createVertexBuffer() {
    VkDeviceSize bufferSize = static_cast<VkDeviceSize>(10000 * sizeof(Vertex));

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
}

void Imgui::createIndexBuffer() {
    VkDeviceSize bufferSize =
        static_cast<VkDeviceSize>(20000 * sizeof(ImDrawIdx));

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
}

void Imgui::createFontTexture(vk::Sampler& sampler) {
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    VkDeviceSize imageSize = width * height * 4;

    vk::Buffer stagingBuffer(
        m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
    stagingBuffer.uploadData(pixels, imageSize);

    auto fontTexture = std::make_unique<Texture>(
        m_device, sampler, stagingBuffer, width, height,
        VK_FORMAT_R8G8B8A8_UNORM
    );

    m_fontTextureIndex = m_bindlessManager.addTexture(std::move(fontTexture));

    io.Fonts->SetTexID(static_cast<ImTextureID>(m_fontTextureIndex));
}

void Imgui::uploadDrawData(ImDrawData* drawData) {
    VkDeviceSize vertexSize =
        static_cast<VkDeviceSize>(drawData->TotalVtxCount * sizeof(Vertex));
    VkDeviceSize indexSize =
        static_cast<VkDeviceSize>(drawData->TotalIdxCount * sizeof(ImDrawIdx));

    if (vertexSize == 0 || indexSize == 0)
        return;

    if (m_vertexBuffer->getSize() < vertexSize) {
        VkDeviceSize newSize = static_cast<VkDeviceSize>(vertexSize * 1.5f);
        m_vertexBuffer.reset();
        m_vertexBuffer = std::make_unique<vk::Buffer>(
            m_device, newSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
    }

    if (m_indexBuffer->getSize() < indexSize) {
        VkDeviceSize newSize = static_cast<VkDeviceSize>(indexSize * 1.5f);
        m_indexBuffer.reset();
        m_indexBuffer = std::make_unique<vk::Buffer>(
            m_device, newSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
    }

    auto* vtxDst = static_cast<Vertex*>(m_vertexBuffer->map());
    auto* idxDst = static_cast<ImDrawIdx*>(m_indexBuffer->map());

    for (size_t i = 0; i < drawData->CmdListsCount; i++) {
        const ImDrawList* cmdList = drawData->CmdLists[i];

        std::memcpy(
            idxDst, cmdList->IdxBuffer.Data,
            cmdList->IdxBuffer.Size * sizeof(ImDrawIdx)
        );
        idxDst += cmdList->IdxBuffer.Size;

        for (int v = 0; v < cmdList->VtxBuffer.Size; v++) {
            const ImDrawVert& imVtx = cmdList->VtxBuffer[v];
            vtxDst[v].position = glm::vec2(imVtx.pos.x, imVtx.pos.y);
            vtxDst[v].uv = glm::vec2(imVtx.uv.x, imVtx.uv.y);
            vtxDst[v].color = imVtx.col;
            vtxDst[v].texIndex = m_fontTextureIndex;
        }

        for (int cmdI = 0; cmdI < cmdList->CmdBuffer.Size; cmdI++) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdI];
            uint32_t currentTexID = static_cast<uint32_t>(pcmd->GetTexID());

            for (uint32_t idx = pcmd->IdxOffset;
                 idx < pcmd->IdxOffset + pcmd->ElemCount; idx++) {
                ImDrawIdx vtxIndex = cmdList->IdxBuffer[idx];
                vtxDst[vtxIndex].texIndex = currentTexID;
            }
        }

        vtxDst += cmdList->VtxBuffer.Size;
    }

    m_vertexBuffer->unmap();
    m_indexBuffer->unmap();
}

void Imgui::update() {
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2(
        static_cast<float>(m_window.getWidth()),
        static_cast<float>(m_window.getHeight())
    );

    float dt = m_window.getDeltaTime();
    io.DeltaTime = dt > 0.0f ? dt : 1.0f / 60.0f;

    if (!m_window.isMinimized()) {
        auto mousePos = m_window.getMousePosFb();
        io.AddMousePosEvent(mousePos.x, mousePos.y);
    } else {
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    io.AddMouseButtonEvent(
        ImGuiMouseButton_Left, m_window.isMouseButtonPressed(SDL_BUTTON_LEFT)
    );
    io.AddMouseButtonEvent(
        ImGuiMouseButton_Right, m_window.isMouseButtonPressed(SDL_BUTTON_RIGHT)
    );
    io.AddMouseButtonEvent(
        ImGuiMouseButton_Middle,
        m_window.isMouseButtonPressed(SDL_BUTTON_MIDDLE)
    );

    auto scrollDt = m_window.getScrollDelta();
    io.AddMouseWheelEvent(scrollDt.x, scrollDt.y);

    auto& codepoints = m_window.getInputCodepoints();
    for (auto& codepoint : codepoints) {
        io.AddInputCharacter(codepoint);
    }

    for (const auto& event : m_window.getPollEvents()) {
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            ImGuiKey imguiKey = SDLKeyToImGuiKey(event.key.keysym.sym);
            if (imguiKey == ImGuiKey_None)
                imguiKey = SDLScancodeToImGuiKey(event.key.keysym.scancode);

            if (imguiKey != ImGuiKey_None) {
                io.AddKeyEvent(imguiKey, (event.type == SDL_KEYDOWN));
            }
        }
    }

    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void Imgui::draw(VkCommandBuffer commandBuffer) {
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData)
        return;

    uploadDrawData(drawData);

    VkBuffer vertexBuffers[] = {m_vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    VkIndexType indexType =
        sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(
        commandBuffer, m_indexBuffer->getBuffer(), 0, indexType
    );

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;

    for (size_t i = 0; i < drawData->CmdListsCount; i++) {
        const ImDrawList* cmdList = drawData->CmdLists[i];
        for (size_t cmdI = 0; cmdI < cmdList->CmdBuffer.Size; cmdI++) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdI];

            VkRect2D scissor;
            scissor.offset.x =
                glm::max(static_cast<int32_t>(pcmd->ClipRect.x - clipOff.x), 0);
            scissor.offset.y =
                glm::max(static_cast<int32_t>(pcmd->ClipRect.y - clipOff.y), 0);
            scissor.extent.width =
                static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissor.extent.height =
                static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdDrawIndexed(
                commandBuffer, pcmd->ElemCount, 1,
                pcmd->IdxOffset + indexOffset, pcmd->VtxOffset + vertexOffset, 0
            );
        }

        vertexOffset += cmdList->VtxBuffer.Size;
        indexOffset += cmdList->IdxBuffer.Size;
    }
}

std::vector<VkVertexInputBindingDescription>
Imgui::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;

    bindingDescriptions.push_back(
        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
    );

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
Imgui::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back(
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)}
    );
    attributeDescriptions.push_back(
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}
    );
    attributeDescriptions.push_back(
        {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex, color)}
    );

    attributeDescriptions.push_back(
        {3, 0, VK_FORMAT_R32_UINT, offsetof(Vertex, texIndex)}
    );

    return attributeDescriptions;
}

} // namespace gfx::ui