#include "imgui.hpp"

namespace gfx::ui {

Imgui::Imgui(win::Window& window, vk::Device& device, vk::Sampler& sampler, mngrs::BindlessManager& bindlessManager) :
    m_window(window), m_device(device), m_bindlessManager(bindlessManager) {
    initImGui();

    createVertexBuffer();
    createIndexBuffer();

    createFontTexture(sampler);
}

Imgui::~Imgui() { ImGui::DestroyContext(); }

void Imgui::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
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
    VkDeviceSize bufferSize = static_cast<VkDeviceSize>(10000 * sizeof(ImDrawVert));

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

void Imgui::createIndexBuffer() {
    VkDeviceSize bufferSize = static_cast<VkDeviceSize>(20000 * sizeof(ImDrawIdx));

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

void Imgui::createFontTexture(vk::Sampler& sampler) {
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    VkDeviceSize imageSize = width * height * 4;

    vk::Buffer stagingBuffer(
        m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(pixels, imageSize);

    auto fontTexture
        = std::make_unique<Texture>(m_device, sampler, stagingBuffer, width, height, VK_FORMAT_R8G8B8A8_UNORM);

    m_fontTextureIndex = m_bindlessManager.addTexture(std::move(fontTexture));

    io.Fonts->SetTexID(static_cast<ImTextureID>(m_fontTextureIndex));
}

void Imgui::uploadDrawData(ImDrawData* drawData) {
    VkDeviceSize vertexSize = static_cast<VkDeviceSize>(drawData->TotalVtxCount * sizeof(ImDrawVert));
    VkDeviceSize indexSize = static_cast<VkDeviceSize>(drawData->TotalIdxCount * sizeof(ImDrawIdx));

    if (vertexSize == 0 || indexSize == 0)
        return;

    if (m_vertexBuffer->getSize() < vertexSize) {
        VkDeviceSize newSize = static_cast<VkDeviceSize>(vertexSize * 1.5f);

        m_vertexBuffer.reset();
        m_vertexBuffer = std::make_unique<vk::Buffer>(
            m_device, newSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    }

    if (m_indexBuffer->getSize() < indexSize) {
        VkDeviceSize newSize = static_cast<VkDeviceSize>(indexSize * 1.5f);

        m_indexBuffer.reset();
        m_indexBuffer = std::make_unique<vk::Buffer>(
            m_device, newSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    }

    ImDrawVert* vtxDst = static_cast<ImDrawVert*>(m_vertexBuffer->map());
    ImDrawIdx* idxDst = static_cast<ImDrawIdx*>(m_indexBuffer->map());

    for (size_t i = 0; i < drawData->CmdListsCount; i++) {
        const ImDrawList* cmdList = drawData->CmdLists[i];

        std::memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

        vtxDst += cmdList->VtxBuffer.Size;
        idxDst += cmdList->IdxBuffer.Size;
    }

    m_vertexBuffer->unmap();
    m_indexBuffer->unmap();
}

void Imgui::update() {
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize
        = ImVec2(static_cast<float>(m_window.getWidth()), static_cast<float>(m_window.getHeight()));

    m_isKeyboardFocused = io.WantCaptureKeyboard;
    m_isMouseFocused = io.WantCaptureMouse;

    float dt = m_window.getDeltaTime();
    io.DeltaTime = dt > 0.0f ? dt : 1.0f / 60.0f;

    if (!m_window.isMinimized()) {
        auto mousePos = m_window.getMousePosFb();
        io.AddMousePosEvent(mousePos.x, mousePos.y);
    } else {
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    io.AddKeyEvent(ImGuiKey_Backspace, m_window.isMouseButtonPressed(GLFW_KEY_BACKSPACE));
    io.AddKeyEvent(ImGuiKey_Delete, m_window.isMouseButtonPressed(GLFW_KEY_DELETE));

    io.AddMouseButtonEvent(ImGuiMouseButton_Left, m_window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT));
    io.AddMouseButtonEvent(ImGuiMouseButton_Right, m_window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT));
    io.AddMouseButtonEvent(ImGuiMouseButton_Middle, m_window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE));

    auto scrollDt = m_window.getScrollDelta();
    io.AddMouseWheelEvent(scrollDt.x, scrollDt.y);

    auto& codepoints = m_window.getInputCodepoints();
    for (auto& codepoint : codepoints) {
        io.AddInputCharacter(codepoint);
    }

    ImGui::NewFrame();
}

void Imgui::draw(VkCommandBuffer commandBuffer) {
    for (auto& gui : m_guis) {
        ImGui::Begin(gui.title.c_str());
        gui.design();
        ImGui::End();
    }

    if (m_showDemoWindow)
        ImGui::ShowDemoWindow(&m_showDemoWindow);

    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData)
        return;

    uploadDrawData(drawData);

    VkBuffer vertexBuffers[] = {m_vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    VkIndexType indexType = sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, indexType);

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;

    for (size_t i = 0; i < drawData->CmdListsCount; i++) {
        const ImDrawList* cmdList = drawData->CmdLists[i];
        for (size_t cmdI = 0; cmdI < cmdList->CmdBuffer.Size; cmdI++) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdI];

            VkRect2D scissor;
            scissor.offset.x = glm::max(static_cast<int32_t>(pcmd->ClipRect.x - clipOff.x), 0);
            scissor.offset.y = glm::max(static_cast<int32_t>(pcmd->ClipRect.y - clipOff.y), 0);
            scissor.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            uint32_t texIndex = static_cast<uint32_t>(pcmd->GetTexID());

            vkCmdDrawIndexed(
                commandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + indexOffset,
                pcmd->VtxOffset + vertexOffset, 0);
        }

        vertexOffset += cmdList->VtxBuffer.Size;
        indexOffset += cmdList->IdxBuffer.Size;
    }
}

std::vector<VkVertexInputBindingDescription> Imgui::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;

    bindingDescriptions.push_back({0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX});

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Imgui::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)});
    attributeDescriptions.push_back({2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)});

    return attributeDescriptions;
}

} // namespace gfx::ui