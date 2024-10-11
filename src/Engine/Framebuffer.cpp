#include "Framebuffer.h"
#include "Engine.h"



void create_framebuffers(VkDevice* device, std::vector<VkFramebuffer>* swapChainFramebuffers, std::vector<VkImageView> swapChainImageViews, VkExtent2D swapChainExtent, VkRenderPass renderPass,VkImageView colorImageView, VkImageView depthImageView)
{
    swapChainFramebuffers->resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr, &(*swapChainFramebuffers)[i]) != VK_SUCCESS) {
            throw std::runtime_error("ERROR: failed to create framebuffer!");
        }
    }
}

void create_shadow_framebuffer(VkDevice* device, VkFramebuffer* shadowFramebuffer, VkImageView* depthImageView, VkRenderPass* shadowRenderPass, VkExtent2D shadowExtent)
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = *shadowRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = depthImageView;
    framebufferInfo.width = shadowExtent.width;
    framebufferInfo.height = shadowExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr, shadowFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
    return;
}