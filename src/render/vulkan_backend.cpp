#include "render/vulkan_backend.h"

#if defined(_WIN32) && defined(NAVSCENE_HAS_VULKAN)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "portrayal/catalog.h"
#include "render/projected_chart_scene.h"
#include "render/software_raster.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace navscene::render {
namespace {

constexpr const char* kVertexShaderFile = "fullscreen_texture.vert.spv";
constexpr const char* kFragmentShaderFile = "fullscreen_texture.frag.spv";

Status OkStatus() { return {}; }

Status ErrorStatus(StatusCode code, std::string message) {
  return Status{code, std::move(message)};
}

std::string VkErrorString(VkResult result) {
  return std::to_string(static_cast<int>(result));
}

struct QueueFamilySelection {
  uint32_t graphics_family = std::numeric_limits<uint32_t>::max();
  uint32_t present_family = std::numeric_limits<uint32_t>::max();
};

struct SwapchainSupport {
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

bool IsComplete(const QueueFamilySelection& selection) {
  return selection.graphics_family != std::numeric_limits<uint32_t>::max() &&
         selection.present_family != std::numeric_limits<uint32_t>::max();
}

std::vector<uint32_t> ReadSpirvWords(const std::string& path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream.is_open()) {
    return {};
  }

  const std::streamsize size = stream.tellg();
  if (size <= 0 || (size % 4) != 0) {
    return {};
  }

  stream.seekg(0, std::ios::beg);
  std::vector<uint32_t> words(static_cast<size_t>(size) / 4u);
  stream.read(reinterpret_cast<char*>(words.data()), size);
  if (!stream.good() && !stream.eof()) {
    return {};
  }
  return words;
}

class VulkanRendererBackend final : public IRendererBackend {
 public:
  VulkanRendererBackend() = default;
  ~VulkanRendererBackend() override;

  GraphicsBackend backend_type() const override { return GraphicsBackend::kVulkan; }
  RenderMode last_render_mode() const override { return last_render_mode_; }
  Status AttachSurface(const NativeSurfaceDesc& surface) override;
  Status DetachSurface() override;
  Status Resize(const NativeSurfaceDesc& surface) override;
  Status RenderFrame(const portrayal::PortrayalScene& scene,
                     const GeoBox& coverage,
                     const Viewport& viewport,
                     const NativeSurfaceDesc& surface) override;

 private:
  Status InitializeWindowBackend();
  Status CreateInstance();
  Status CreateSurface();
  QueueFamilySelection FindQueueFamilies(VkPhysicalDevice device) const;
  bool HasRequiredDeviceExtensions(VkPhysicalDevice device) const;
  SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice device) const;
  Status PickPhysicalDeviceAndCreateLogicalDevice();
  Status CreateCommandObjects();
  Status CreateSyncObjects();
  VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
  VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;
  VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
  Status RecreateSwapchain();
  Status CreateSwapchainImageViews();
  Status CreateRenderPass();
  Status CreatePipeline();
  Status CreateNativePipelines();
  Status CreateShaderModule(const std::vector<uint32_t>& words,
                            VkShaderModule* out);
  uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const;
  Status CreateBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer* buffer,
                      VkDeviceMemory* memory);
  Status CreateImage(uint32_t width,
                     uint32_t height,
                     VkFormat format,
                     VkImageUsageFlags usage,
                     VkImage* image,
                     VkDeviceMemory* memory);
  Status EnsureBufferCapacity(VkDeviceSize required_size,
                              VkBufferUsageFlags usage,
                              VkBuffer* buffer,
                              VkDeviceMemory* memory,
                              VkDeviceSize* current_size);
  Status UploadBufferData(VkBuffer buffer,
                          VkDeviceMemory memory,
                          VkDeviceSize size,
                          const void* data);
  Status CreateTextureResources();
  Status CreateFramebuffers();
  Status DrawNativeFrame(const ProjectedChartScene& scene);
  Status DrawFrame(const SoftwareRasterImage& image);
  Status RecordNativeCommandBuffer(uint32_t image_index,
                                   const ProjectedChartScene& scene);
  Status RecordCommandBuffer(uint32_t image_index);
  void DestroySwapchainResources();
  void DestroyAll();

  NativeSurfaceDesc surface_{};
  bool offscreen_attached_ = false;
  bool window_initialized_ = false;
  bool texture_initialized_ = false;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR window_surface_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  QueueFamilySelection queue_families_{};
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat swapchain_format_ = VK_FORMAT_UNDEFINED;
  VkExtent2D swapchain_extent_{};
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_image_views_;
  VkRenderPass render_pass_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout native_pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline native_triangle_pipeline_ = VK_NULL_HANDLE;
  VkPipeline native_line_pipeline_ = VK_NULL_HANDLE;
  VkPipeline native_point_pipeline_ = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> framebuffers_;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
  VkSemaphore image_available_semaphore_ = VK_NULL_HANDLE;
  VkSemaphore render_finished_semaphore_ = VK_NULL_HANDLE;
  VkFence in_flight_fence_ = VK_NULL_HANDLE;
  VkBuffer staging_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory staging_buffer_memory_ = VK_NULL_HANDLE;
  VkDeviceSize staging_buffer_size_ = 0;
  VkImage texture_image_ = VK_NULL_HANDLE;
  VkDeviceMemory texture_image_memory_ = VK_NULL_HANDLE;
  VkImageView texture_image_view_ = VK_NULL_HANDLE;
  VkSampler sampler_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
  VkBuffer triangle_vertex_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory triangle_vertex_buffer_memory_ = VK_NULL_HANDLE;
  VkDeviceSize triangle_vertex_buffer_size_ = 0;
  VkBuffer line_vertex_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory line_vertex_buffer_memory_ = VK_NULL_HANDLE;
  VkDeviceSize line_vertex_buffer_size_ = 0;
  VkBuffer point_vertex_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory point_vertex_buffer_memory_ = VK_NULL_HANDLE;
  VkDeviceSize point_vertex_buffer_size_ = 0;
  portrayal::Rgb8 clear_color_{201, 235, 252};
  RenderMode last_render_mode_ = RenderMode::kUnknown;
};

VulkanRendererBackend::~VulkanRendererBackend() {
  DestroyAll();
}

Status VulkanRendererBackend::AttachSurface(const NativeSurfaceDesc& surface) {
  surface_ = surface;
  last_render_mode_ = RenderMode::kUnknown;

  if (surface.type == SurfaceType::kOffscreen) {
    offscreen_attached_ = true;
    return OkStatus();
  }

  if (surface.type != SurfaceType::kWindow) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "Vulkan backend currently supports window or offscreen surfaces.");
  }
  if (surface.platform != NativePlatform::kWin32) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "Vulkan backend currently supports only Win32 window surfaces.");
  }
  if (surface.window_handle == nullptr) {
    return ErrorStatus(StatusCode::kInvalidArgument,
                       "Native window handle must not be null.");
  }

  offscreen_attached_ = false;
  DestroyAll();
  surface_ = surface;
  return InitializeWindowBackend();
}

Status VulkanRendererBackend::DetachSurface() {
  DestroyAll();
  surface_ = {};
  offscreen_attached_ = false;
  last_render_mode_ = RenderMode::kUnknown;
  return OkStatus();
}

Status VulkanRendererBackend::Resize(const NativeSurfaceDesc& surface) {
  surface_ = surface;
  if (surface.type == SurfaceType::kOffscreen) {
    return OkStatus();
  }
  if (!window_initialized_) {
    return ErrorStatus(StatusCode::kNotInitialized,
                       "Vulkan backend window surface is not initialized.");
  }
  if (surface.width == 0 || surface.height == 0) {
    return OkStatus();
  }
  return RecreateSwapchain();
}

Status VulkanRendererBackend::RenderFrame(const portrayal::PortrayalScene& scene,
                                          const GeoBox& coverage,
                                          const Viewport& viewport,
                                          const NativeSurfaceDesc& surface) {
  surface_ = surface;
  clear_color_ = scene.background_color;
  if (surface.type == SurfaceType::kOffscreen) {
    return OkStatus();
  }
  if (!window_initialized_) {
    return ErrorStatus(StatusCode::kNotInitialized,
                       "Vulkan backend window surface is not initialized.");
  }
  if (swapchain_ == VK_NULL_HANDLE || swapchain_extent_.width == 0 ||
      swapchain_extent_.height == 0) {
    return OkStatus();
  }

  Viewport projected_viewport = viewport;
  projected_viewport.width = swapchain_extent_.width;
  projected_viewport.height = swapchain_extent_.height;
  const auto projected_scene =
      BuildProjectedChartScene(scene, coverage, projected_viewport);
  if (projected_scene.has_valid_coverage && scene.labels.empty() &&
      !projected_scene.requires_complex_polygon_support &&
      !projected_scene.requires_complex_line_support) {
    last_render_mode_ = RenderMode::kGpuNativeGeometry;
    return DrawNativeFrame(projected_scene);
  }

  SoftwareRasterImage image;
  const auto raster_status = RasterizeChartSceneWin32(scene,
                                                      coverage,
                                                      projected_viewport,
                                                      swapchain_extent_.width,
                                                      swapchain_extent_.height,
                                                      &image);
  if (!raster_status.ok()) {
    return raster_status;
  }

  last_render_mode_ = RenderMode::kGpuRasterUpload;
  return DrawFrame(image);
}

Status VulkanRendererBackend::InitializeWindowBackend() {
  const auto instance_status = CreateInstance();
  if (!instance_status.ok()) {
    return instance_status;
  }

  const auto surface_status = CreateSurface();
  if (!surface_status.ok()) {
    return surface_status;
  }

  const auto device_status = PickPhysicalDeviceAndCreateLogicalDevice();
  if (!device_status.ok()) {
    return device_status;
  }

  const auto command_status = CreateCommandObjects();
  if (!command_status.ok()) {
    return command_status;
  }

  const auto sync_status = CreateSyncObjects();
  if (!sync_status.ok()) {
    return sync_status;
  }

  const auto swapchain_status = RecreateSwapchain();
  if (!swapchain_status.ok()) {
    return swapchain_status;
  }

  window_initialized_ = true;
  return OkStatus();
}

Status VulkanRendererBackend::CreateInstance() {
  const std::array<const char*, 2> extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
  };

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "navscene-sdk";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.pEngineName = "navscene";
  app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  const VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "vkCreateInstance failed with code " + VkErrorString(result) + ".");
  }
  return OkStatus();
}

Status VulkanRendererBackend::CreateSurface() {
  VkWin32SurfaceCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  create_info.hinstance = GetModuleHandle(nullptr);
  create_info.hwnd = static_cast<HWND>(surface_.window_handle);

  const VkResult result =
      vkCreateWin32SurfaceKHR(instance_, &create_info, nullptr, &window_surface_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "vkCreateWin32SurfaceKHR failed with code " + VkErrorString(result) +
                           ".");
  }
  return OkStatus();
}

QueueFamilySelection VulkanRendererBackend::FindQueueFamilies(
    VkPhysicalDevice device) const {
  QueueFamilySelection selection;

  uint32_t family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
  std::vector<VkQueueFamilyProperties> families(family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

  for (uint32_t index = 0; index < family_count; ++index) {
    if (families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      selection.graphics_family = index;
    }

    VkBool32 present_supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, index, window_surface_, &present_supported);
    if (present_supported == VK_TRUE) {
      selection.present_family = index;
    }

    if (IsComplete(selection)) {
      break;
    }
  }

  return selection;
}

bool VulkanRendererBackend::HasRequiredDeviceExtensions(VkPhysicalDevice device) const {
  uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

  for (const auto& extension : extensions) {
    if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
      return true;
    }
  }
  return false;
}

SwapchainSupport VulkanRendererBackend::QuerySwapchainSupport(VkPhysicalDevice device) const {
  SwapchainSupport support;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, window_surface_, &support.capabilities);

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, window_surface_, &format_count, nullptr);
  support.formats.resize(format_count);
  if (format_count > 0) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                                         window_surface_,
                                         &format_count,
                                         support.formats.data());
  }

  uint32_t present_mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                            window_surface_,
                                            &present_mode_count,
                                            nullptr);
  support.present_modes.resize(present_mode_count);
  if (present_mode_count > 0) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                              window_surface_,
                                              &present_mode_count,
                                              support.present_modes.data());
  }
  return support;
}

Status VulkanRendererBackend::PickPhysicalDeviceAndCreateLogicalDevice() {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
  if (device_count == 0) {
    return ErrorStatus(StatusCode::kUnsupported, "No Vulkan physical devices were found.");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

  for (VkPhysicalDevice device : devices) {
    const auto families = FindQueueFamilies(device);
    if (!IsComplete(families) || !HasRequiredDeviceExtensions(device)) {
      continue;
    }

    const auto swapchain_support = QuerySwapchainSupport(device);
    if (swapchain_support.formats.empty() || swapchain_support.present_modes.empty()) {
      continue;
    }

    physical_device_ = device;
    queue_families_ = families;
    break;
  }

  if (physical_device_ == VK_NULL_HANDLE) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "No suitable Vulkan device with swapchain support was found.");
  }

  std::vector<uint32_t> unique_families = {queue_families_.graphics_family};
  if (queue_families_.present_family != queue_families_.graphics_family) {
    unique_families.push_back(queue_families_.present_family);
  }

  const float queue_priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queue_infos;
  queue_infos.reserve(unique_families.size());
  for (uint32_t family : unique_families) {
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;
    queue_infos.push_back(queue_info);
  }

  const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
  create_info.pQueueCreateInfos = queue_infos.data();
  create_info.enabledExtensionCount = 1;
  create_info.ppEnabledExtensionNames = device_extensions;

  const VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kUnsupported,
                       "vkCreateDevice failed with code " + VkErrorString(result) + ".");
  }

  vkGetDeviceQueue(device_, queue_families_.graphics_family, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, queue_families_.present_family, 0, &present_queue_);
  return OkStatus();
}

Status VulkanRendererBackend::CreateCommandObjects() {
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = queue_families_.graphics_family;

  VkResult result = vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateCommandPool failed with code " + VkErrorString(result) + ".");
  }

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  result = vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkAllocateCommandBuffers failed with code " +
                           VkErrorString(result) + ".");
  }
  return OkStatus();
}

Status VulkanRendererBackend::CreateSyncObjects() {
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkResult result =
      vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphore_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateSemaphore(image_available) failed with code " +
                           VkErrorString(result) + ".");
  }

  result = vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphore_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateSemaphore(render_finished) failed with code " +
                           VkErrorString(result) + ".");
  }

  result = vkCreateFence(device_, &fence_info, nullptr, &in_flight_fence_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateFence failed with code " + VkErrorString(result) + ".");
  }
  return OkStatus();
}

VkSurfaceFormatKHR VulkanRendererBackend::ChooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& formats) const {
  for (const auto& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return formats.front();
}

VkPresentModeKHR VulkanRendererBackend::ChoosePresentMode(
    const std::vector<VkPresentModeKHR>& modes) const {
  for (VkPresentModeKHR mode : modes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRendererBackend::ChooseExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) const {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  VkExtent2D extent{};
  extent.width = std::clamp(surface_.width,
                            capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  extent.height = std::clamp(surface_.height,
                             capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
  return extent;
}

Status VulkanRendererBackend::RecreateSwapchain() {
  if (device_ == VK_NULL_HANDLE || window_surface_ == VK_NULL_HANDLE) {
    return ErrorStatus(StatusCode::kNotInitialized,
                       "Vulkan device or surface is not initialized.");
  }

  if (surface_.width == 0 || surface_.height == 0) {
    return OkStatus();
  }

  vkDeviceWaitIdle(device_);
  DestroySwapchainResources();

  const auto support = QuerySwapchainSupport(physical_device_);
  if (support.formats.empty() || support.present_modes.empty()) {
    return ErrorStatus(StatusCode::kUnsupported, "Vulkan swapchain support is incomplete.");
  }

  const VkSurfaceFormatKHR surface_format = ChooseSurfaceFormat(support.formats);
  const VkPresentModeKHR present_mode = ChoosePresentMode(support.present_modes);
  const VkExtent2D extent = ChooseExtent(support.capabilities);

  uint32_t image_count = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 &&
      image_count > support.capabilities.maxImageCount) {
    image_count = support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = window_surface_;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  const uint32_t queue_family_indices[] = {
      queue_families_.graphics_family,
      queue_families_.present_family,
  };
  if (queue_families_.graphics_family != queue_families_.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;

  VkResult result = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateSwapchainKHR failed with code " + VkErrorString(result) +
                           ".");
  }

  uint32_t swapchain_image_count = 0;
  vkGetSwapchainImagesKHR(device_, swapchain_, &swapchain_image_count, nullptr);
  swapchain_images_.resize(swapchain_image_count);
  vkGetSwapchainImagesKHR(device_,
                          swapchain_,
                          &swapchain_image_count,
                          swapchain_images_.data());

  swapchain_format_ = surface_format.format;
  swapchain_extent_ = extent;

  const auto views_status = CreateSwapchainImageViews();
  if (!views_status.ok()) {
    return views_status;
  }
  const auto render_pass_status = CreateRenderPass();
  if (!render_pass_status.ok()) {
    return render_pass_status;
  }
  const auto pipeline_status = CreatePipeline();
  if (!pipeline_status.ok()) {
    return pipeline_status;
  }
  const auto native_pipeline_status = CreateNativePipelines();
  if (!native_pipeline_status.ok()) {
    return native_pipeline_status;
  }
  const auto texture_status = CreateTextureResources();
  if (!texture_status.ok()) {
    return texture_status;
  }
  const auto framebuffer_status = CreateFramebuffers();
  if (!framebuffer_status.ok()) {
    return framebuffer_status;
  }
  texture_initialized_ = false;
  return OkStatus();
}

Status VulkanRendererBackend::CreateSwapchainImageViews() {
  swapchain_image_views_.clear();
  swapchain_image_views_.reserve(swapchain_images_.size());

  for (VkImage image : swapchain_images_) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swapchain_format_;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VkImageView image_view = VK_NULL_HANDLE;
    const VkResult result = vkCreateImageView(device_, &create_info, nullptr, &image_view);
    if (result != VK_SUCCESS) {
      return ErrorStatus(StatusCode::kInternalError,
                         "vkCreateImageView(swapchain) failed with code " +
                             VkErrorString(result) + ".");
    }
    swapchain_image_views_.push_back(image_view);
  }

  return OkStatus();
}

Status VulkanRendererBackend::CreateRenderPass() {
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swapchain_format_;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  const VkResult result =
      vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateRenderPass failed with code " + VkErrorString(result) + ".");
  }
  return OkStatus();
}

Status VulkanRendererBackend::CreatePipeline() {
  const std::string shader_dir = NAVSCENE_VK_SHADER_DIR;
  const auto vertex_words = ReadSpirvWords(shader_dir + "/" + kVertexShaderFile);
  const auto fragment_words = ReadSpirvWords(shader_dir + "/" + kFragmentShaderFile);
  if (vertex_words.empty() || fragment_words.empty()) {
    return ErrorStatus(StatusCode::kIoError,
                       "Failed to load compiled Vulkan shader binaries.");
  }

  VkShaderModule vertex_module = VK_NULL_HANDLE;
  VkShaderModule fragment_module = VK_NULL_HANDLE;
  const auto vertex_status = CreateShaderModule(vertex_words, &vertex_module);
  if (!vertex_status.ok()) {
    return vertex_status;
  }
  const auto fragment_status = CreateShaderModule(fragment_words, &fragment_module);
  if (!fragment_status.ok()) {
    vkDestroyShaderModule(device_, vertex_module, nullptr);
    return fragment_status;
  }

  VkPipelineShaderStageCreateInfo vertex_stage{};
  vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_stage.module = vertex_module;
  vertex_stage.pName = "main";

  VkPipelineShaderStageCreateInfo fragment_stage{};
  fragment_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_stage.module = fragment_module;
  fragment_stage.pName = "main";

  const std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
      vertex_stage,
      fragment_stage,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input{};
  vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;

  const std::array<VkDynamicState, 2> dynamic_states = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_state.pDynamicStates = dynamic_states.data();

  VkDescriptorSetLayoutBinding sampler_binding{};
  sampler_binding.binding = 0;
  sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_binding.descriptorCount = 1;
  sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 1;
  layout_info.pBindings = &sampler_binding;

  VkResult result =
      vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_);
  if (result != VK_SUCCESS) {
    vkDestroyShaderModule(device_, fragment_module, nullptr);
    vkDestroyShaderModule(device_, vertex_module, nullptr);
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateDescriptorSetLayout failed with code " +
                           VkErrorString(result) + ".");
  }

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;

  result =
      vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_);
  if (result != VK_SUCCESS) {
    vkDestroyShaderModule(device_, fragment_module, nullptr);
    vkDestroyShaderModule(device_, vertex_module, nullptr);
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreatePipelineLayout failed with code " +
                           VkErrorString(result) + ".");
  }

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = static_cast<uint32_t>(stages.size());
  pipeline_info.pStages = stages.data();
  pipeline_info.pVertexInputState = &vertex_input;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = pipeline_layout_;
  pipeline_info.renderPass = render_pass_;
  pipeline_info.subpass = 0;

  result = vkCreateGraphicsPipelines(
      device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_);

  vkDestroyShaderModule(device_, fragment_module, nullptr);
  vkDestroyShaderModule(device_, vertex_module, nullptr);

  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateGraphicsPipelines failed with code " +
                           VkErrorString(result) + ".");
  }
  return OkStatus();
}

Status VulkanRendererBackend::CreateNativePipelines() {
  const std::string shader_dir = NAVSCENE_VK_SHADER_DIR;
  const auto color_vertex_words =
      ReadSpirvWords(shader_dir + "/solid_color.vert.spv");
  const auto color_fragment_words =
      ReadSpirvWords(shader_dir + "/solid_color.frag.spv");
  const auto point_vertex_words =
      ReadSpirvWords(shader_dir + "/point_color.vert.spv");
  const auto point_fragment_words =
      ReadSpirvWords(shader_dir + "/point_color.frag.spv");
  if (color_vertex_words.empty() || color_fragment_words.empty() ||
      point_vertex_words.empty() || point_fragment_words.empty()) {
    return ErrorStatus(StatusCode::kIoError,
                       "Failed to load compiled native Vulkan primitive shader binaries.");
  }

  VkPipelineLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  VkResult result =
      vkCreatePipelineLayout(device_, &layout_info, nullptr, &native_pipeline_layout_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreatePipelineLayout(native) failed with code " +
                           VkErrorString(result) + ".");
  }

  auto create_pipeline = [&](const std::vector<uint32_t>& vertex_words,
                             const std::vector<uint32_t>& fragment_words,
                             VkPrimitiveTopology topology,
                             const VkVertexInputBindingDescription& binding,
                             const VkVertexInputAttributeDescription* attributes,
                             uint32_t attribute_count,
                             VkPipeline* out_pipeline) -> Status {
    VkShaderModule vertex_module = VK_NULL_HANDLE;
    VkShaderModule fragment_module = VK_NULL_HANDLE;
    const auto vertex_status = CreateShaderModule(vertex_words, &vertex_module);
    if (!vertex_status.ok()) {
      return vertex_status;
    }
    const auto fragment_status = CreateShaderModule(fragment_words, &fragment_module);
    if (!fragment_status.ok()) {
      vkDestroyShaderModule(device_, vertex_module, nullptr);
      return fragment_status;
    }

    VkPipelineShaderStageCreateInfo vertex_stage{};
    vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_stage.module = vertex_module;
    vertex_stage.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_stage{};
    fragment_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_stage.module = fragment_module;
    fragment_stage.pName = "main";

    const std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
        vertex_stage,
        fragment_stage,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = attribute_count;
    vertex_input.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = topology;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    const std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = static_cast<uint32_t>(stages.size());
    pipeline_info.pStages = stages.data();
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = native_pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;

    result = vkCreateGraphicsPipelines(
        device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, out_pipeline);

    vkDestroyShaderModule(device_, fragment_module, nullptr);
    vkDestroyShaderModule(device_, vertex_module, nullptr);

    if (result != VK_SUCCESS) {
      return ErrorStatus(StatusCode::kInternalError,
                         "vkCreateGraphicsPipelines(native) failed with code " +
                             VkErrorString(result) + ".");
    }
    return OkStatus();
  };

  VkVertexInputBindingDescription color_binding{};
  color_binding.binding = 0;
  color_binding.stride = sizeof(ProjectedColorVertex);
  color_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  const std::array<VkVertexInputAttributeDescription, 2> color_attributes = {
      VkVertexInputAttributeDescription{
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = static_cast<uint32_t>(offsetof(ProjectedColorVertex, x)),
      },
      VkVertexInputAttributeDescription{
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32A32_SFLOAT,
          .offset = static_cast<uint32_t>(offsetof(ProjectedColorVertex, r)),
      },
  };

  auto status = create_pipeline(color_vertex_words,
                                color_fragment_words,
                                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                color_binding,
                                color_attributes.data(),
                                static_cast<uint32_t>(color_attributes.size()),
                                &native_triangle_pipeline_);
  if (!status.ok()) {
    return status;
  }

  status = create_pipeline(color_vertex_words,
                           color_fragment_words,
                           VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                           color_binding,
                           color_attributes.data(),
                           static_cast<uint32_t>(color_attributes.size()),
                           &native_line_pipeline_);
  if (!status.ok()) {
    return status;
  }

  VkVertexInputBindingDescription point_binding{};
  point_binding.binding = 0;
  point_binding.stride = sizeof(ProjectedPointVertex);
  point_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  const std::array<VkVertexInputAttributeDescription, 3> point_attributes = {
      VkVertexInputAttributeDescription{
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = static_cast<uint32_t>(offsetof(ProjectedPointVertex, x)),
      },
      VkVertexInputAttributeDescription{
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32_SFLOAT,
          .offset = static_cast<uint32_t>(offsetof(ProjectedPointVertex, size_px)),
      },
      VkVertexInputAttributeDescription{
          .location = 2,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32A32_SFLOAT,
          .offset = static_cast<uint32_t>(offsetof(ProjectedPointVertex, r)),
      },
  };

  return create_pipeline(point_vertex_words,
                         point_fragment_words,
                         VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                         point_binding,
                         point_attributes.data(),
                         static_cast<uint32_t>(point_attributes.size()),
                         &native_point_pipeline_);
}

Status VulkanRendererBackend::CreateShaderModule(const std::vector<uint32_t>& words,
                                                 VkShaderModule* out) {
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = words.size() * sizeof(uint32_t);
  create_info.pCode = words.data();

  const VkResult result = vkCreateShaderModule(device_, &create_info, nullptr, out);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateShaderModule failed with code " + VkErrorString(result) + ".");
  }
  return OkStatus();
}

uint32_t VulkanRendererBackend::FindMemoryType(uint32_t type_filter,
                                               VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties memory_properties{};
  vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties);
  for (uint32_t index = 0; index < memory_properties.memoryTypeCount; ++index) {
    const bool type_supported = (type_filter & (1u << index)) != 0;
    const bool properties_match =
        (memory_properties.memoryTypes[index].propertyFlags & properties) == properties;
    if (type_supported && properties_match) {
      return index;
    }
  }
  return std::numeric_limits<uint32_t>::max();
}

Status VulkanRendererBackend::CreateBuffer(VkDeviceSize size,
                                           VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags properties,
                                           VkBuffer* buffer,
                                           VkDeviceMemory* memory) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(device_, &buffer_info, nullptr, buffer);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateBuffer failed with code " + VkErrorString(result) + ".");
  }

  VkMemoryRequirements requirements{};
  vkGetBufferMemoryRequirements(device_, *buffer, &requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = requirements.size;
  alloc_info.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, properties);
  if (alloc_info.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
    return ErrorStatus(StatusCode::kInternalError,
                       "Failed to find a compatible Vulkan buffer memory type.");
  }

  result = vkAllocateMemory(device_, &alloc_info, nullptr, memory);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkAllocateMemory(buffer) failed with code " +
                           VkErrorString(result) + ".");
  }

  result = vkBindBufferMemory(device_, *buffer, *memory, 0);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkBindBufferMemory failed with code " + VkErrorString(result) + ".");
  }
  return OkStatus();
}

Status VulkanRendererBackend::CreateImage(uint32_t width,
                                          uint32_t height,
                                          VkFormat format,
                                          VkImageUsageFlags usage,
                                          VkImage* image,
                                          VkDeviceMemory* memory) {
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = usage;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateImage(device_, &image_info, nullptr, image);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateImage failed with code " + VkErrorString(result) + ".");
  }

  VkMemoryRequirements requirements{};
  vkGetImageMemoryRequirements(device_, *image, &requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = requirements.size;
  alloc_info.memoryTypeIndex =
      FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (alloc_info.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
    return ErrorStatus(StatusCode::kInternalError,
                       "Failed to find a compatible Vulkan image memory type.");
  }

  result = vkAllocateMemory(device_, &alloc_info, nullptr, memory);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkAllocateMemory(image) failed with code " +
                           VkErrorString(result) + ".");
  }

  result = vkBindImageMemory(device_, *image, *memory, 0);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkBindImageMemory failed with code " + VkErrorString(result) + ".");
  }
  return OkStatus();
}

Status VulkanRendererBackend::EnsureBufferCapacity(VkDeviceSize required_size,
                                                   VkBufferUsageFlags usage,
                                                   VkBuffer* buffer,
                                                   VkDeviceMemory* memory,
                                                   VkDeviceSize* current_size) {
  if (buffer == nullptr || memory == nullptr || current_size == nullptr) {
    return ErrorStatus(StatusCode::kInvalidArgument,
                       "Vulkan buffer capacity arguments must not be null.");
  }
  if (required_size == 0) {
    return OkStatus();
  }
  if (*buffer != VK_NULL_HANDLE && *current_size >= required_size) {
    return OkStatus();
  }

  if (*buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device_, *buffer, nullptr);
    *buffer = VK_NULL_HANDLE;
  }
  if (*memory != VK_NULL_HANDLE) {
    vkFreeMemory(device_, *memory, nullptr);
    *memory = VK_NULL_HANDLE;
  }

  const VkDeviceSize new_size = std::max(required_size, *current_size == 0 ? required_size
                                                                            : *current_size * 2);
  const auto status = CreateBuffer(new_size,
                                   usage,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   buffer,
                                   memory);
  if (!status.ok()) {
    return status;
  }

  *current_size = new_size;
  return OkStatus();
}

Status VulkanRendererBackend::UploadBufferData(VkBuffer,
                                               VkDeviceMemory memory,
                                               VkDeviceSize size,
                                               const void* data) {
  if (size == 0 || data == nullptr) {
    return OkStatus();
  }

  void* mapped = nullptr;
  const VkResult result = vkMapMemory(device_, memory, 0, size, 0, &mapped);
  if (result != VK_SUCCESS || mapped == nullptr) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkMapMemory(vertex upload) failed with code " +
                           VkErrorString(result) + ".");
  }

  std::memcpy(mapped, data, static_cast<size_t>(size));
  vkUnmapMemory(device_, memory);
  return OkStatus();
}

Status VulkanRendererBackend::CreateTextureResources() {
  const VkDeviceSize texture_size = static_cast<VkDeviceSize>(swapchain_extent_.width) *
                                    static_cast<VkDeviceSize>(swapchain_extent_.height) * 4u;

  const auto staging_status = CreateBuffer(texture_size,
                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           &staging_buffer_,
                                           &staging_buffer_memory_);
  if (!staging_status.ok()) {
    return staging_status;
  }
  staging_buffer_size_ = texture_size;

  const auto image_status = CreateImage(swapchain_extent_.width,
                                        swapchain_extent_.height,
                                        VK_FORMAT_B8G8R8A8_UNORM,
                                        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                            VK_IMAGE_USAGE_SAMPLED_BIT,
                                        &texture_image_,
                                        &texture_image_memory_);
  if (!image_status.ok()) {
    return image_status;
  }

  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = texture_image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_B8G8R8A8_UNORM;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VkResult result = vkCreateImageView(device_, &view_info, nullptr, &texture_image_view_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateImageView(texture) failed with code " +
                           VkErrorString(result) + ".");
  }

  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxAnisotropy = 1.0f;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  result = vkCreateSampler(device_, &sampler_info, nullptr, &sampler_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateSampler failed with code " + VkErrorString(result) + ".");
  }

  VkDescriptorPoolSize pool_size{};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = 1;

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  pool_info.maxSets = 1;

  result = vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkCreateDescriptorPool failed with code " +
                           VkErrorString(result) + ".");
  }

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool_;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &descriptor_set_layout_;

  result = vkAllocateDescriptorSets(device_, &alloc_info, &descriptor_set_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkAllocateDescriptorSets failed with code " +
                           VkErrorString(result) + ".");
  }

  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = texture_image_view_;
  image_info.sampler = sampler_;

  VkWriteDescriptorSet descriptor_write{};
  descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet = descriptor_set_;
  descriptor_write.dstBinding = 0;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptor_write.descriptorCount = 1;
  descriptor_write.pImageInfo = &image_info;

  vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
  return OkStatus();
}

Status VulkanRendererBackend::CreateFramebuffers() {
  framebuffers_.clear();
  framebuffers_.reserve(swapchain_image_views_.size());

  for (VkImageView image_view : swapchain_image_views_) {
    VkImageView attachments[] = {image_view};

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = swapchain_extent_.width;
    framebuffer_info.height = swapchain_extent_.height;
    framebuffer_info.layers = 1;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    const VkResult result =
        vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &framebuffer);
    if (result != VK_SUCCESS) {
      return ErrorStatus(StatusCode::kInternalError,
                         "vkCreateFramebuffer failed with code " +
                             VkErrorString(result) + ".");
    }
    framebuffers_.push_back(framebuffer);
  }
  return OkStatus();
}

Status VulkanRendererBackend::DrawNativeFrame(const ProjectedChartScene& scene) {
  const VkDeviceSize triangle_bytes =
      static_cast<VkDeviceSize>(scene.triangle_vertices.size() * sizeof(ProjectedColorVertex));
  const VkDeviceSize line_bytes =
      static_cast<VkDeviceSize>(scene.line_vertices.size() * sizeof(ProjectedColorVertex));
  const VkDeviceSize point_bytes =
      static_cast<VkDeviceSize>(scene.point_vertices.size() * sizeof(ProjectedPointVertex));

  auto status = EnsureBufferCapacity(triangle_bytes,
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     &triangle_vertex_buffer_,
                                     &triangle_vertex_buffer_memory_,
                                     &triangle_vertex_buffer_size_);
  if (!status.ok()) {
    return status;
  }
  status = EnsureBufferCapacity(line_bytes,
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                &line_vertex_buffer_,
                                &line_vertex_buffer_memory_,
                                &line_vertex_buffer_size_);
  if (!status.ok()) {
    return status;
  }
  status = EnsureBufferCapacity(point_bytes,
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                &point_vertex_buffer_,
                                &point_vertex_buffer_memory_,
                                &point_vertex_buffer_size_);
  if (!status.ok()) {
    return status;
  }

  status = UploadBufferData(triangle_vertex_buffer_,
                            triangle_vertex_buffer_memory_,
                            triangle_bytes,
                            scene.triangle_vertices.data());
  if (!status.ok()) {
    return status;
  }
  status = UploadBufferData(line_vertex_buffer_,
                            line_vertex_buffer_memory_,
                            line_bytes,
                            scene.line_vertices.data());
  if (!status.ok()) {
    return status;
  }
  status = UploadBufferData(point_vertex_buffer_,
                            point_vertex_buffer_memory_,
                            point_bytes,
                            scene.point_vertices.data());
  if (!status.ok()) {
    return status;
  }

  vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
  vkResetFences(device_, 1, &in_flight_fence_);

  uint32_t image_index = 0;
  VkResult result = vkAcquireNextImageKHR(device_,
                                          swapchain_,
                                          UINT64_MAX,
                                          image_available_semaphore_,
                                          VK_NULL_HANDLE,
                                          &image_index);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return RecreateSwapchain();
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkAcquireNextImageKHR(native) failed with code " +
                           VkErrorString(result) + ".");
  }

  vkResetCommandBuffer(command_buffer_, 0);
  status = RecordNativeCommandBuffer(image_index, scene);
  if (!status.ok()) {
    return status;
  }

  const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_available_semaphore_;
  submit_info.pWaitDstStageMask = &wait_stage;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &render_finished_semaphore_;

  result = vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fence_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkQueueSubmit(native) failed with code " +
                           VkErrorString(result) + ".");
  }

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &render_finished_semaphore_;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain_;
  present_info.pImageIndices = &image_index;

  result = vkQueuePresentKHR(present_queue_, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return RecreateSwapchain();
  }
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkQueuePresentKHR(native) failed with code " +
                           VkErrorString(result) + ".");
  }

  return OkStatus();
}

Status VulkanRendererBackend::DrawFrame(const SoftwareRasterImage& image) {
  vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
  vkResetFences(device_, 1, &in_flight_fence_);

  void* mapped = nullptr;
  VkResult result =
      vkMapMemory(device_, staging_buffer_memory_, 0, staging_buffer_size_, 0, &mapped);
  if (result != VK_SUCCESS || mapped == nullptr) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkMapMemory failed with code " + VkErrorString(result) + ".");
  }
  std::memcpy(mapped, image.bgra_pixels.data(), image.bgra_pixels.size());
  vkUnmapMemory(device_, staging_buffer_memory_);

  uint32_t image_index = 0;
  result = vkAcquireNextImageKHR(device_,
                                 swapchain_,
                                 UINT64_MAX,
                                 image_available_semaphore_,
                                 VK_NULL_HANDLE,
                                 &image_index);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return RecreateSwapchain();
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkAcquireNextImageKHR failed with code " +
                           VkErrorString(result) + ".");
  }

  vkResetCommandBuffer(command_buffer_, 0);
  const auto record_status = RecordCommandBuffer(image_index);
  if (!record_status.ok()) {
    return record_status;
  }

  const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_available_semaphore_;
  submit_info.pWaitDstStageMask = &wait_stage;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &render_finished_semaphore_;

  result = vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fence_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkQueueSubmit failed with code " + VkErrorString(result) + ".");
  }

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &render_finished_semaphore_;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain_;
  present_info.pImageIndices = &image_index;

  result = vkQueuePresentKHR(present_queue_, &present_info);
  texture_initialized_ = true;
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return RecreateSwapchain();
  }
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkQueuePresentKHR failed with code " + VkErrorString(result) + ".");
  }

  return OkStatus();
}

Status VulkanRendererBackend::RecordNativeCommandBuffer(uint32_t image_index,
                                                        const ProjectedChartScene& scene) {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  VkResult result = vkBeginCommandBuffer(command_buffer_, &begin_info);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkBeginCommandBuffer(native) failed with code " +
                           VkErrorString(result) + ".");
  }

  VkClearValue clear_color{};
  clear_color.color = {{static_cast<float>(clear_color_.r) / 255.0f,
                        static_cast<float>(clear_color_.g) / 255.0f,
                        static_cast<float>(clear_color_.b) / 255.0f,
                        1.0f}};

  VkRenderPassBeginInfo render_pass_begin{};
  render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin.renderPass = render_pass_;
  render_pass_begin.framebuffer = framebuffers_[image_index];
  render_pass_begin.renderArea.offset = {0, 0};
  render_pass_begin.renderArea.extent = swapchain_extent_;
  render_pass_begin.clearValueCount = 1;
  render_pass_begin.pClearValues = &clear_color;

  vkCmdBeginRenderPass(command_buffer_, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  // Native projected vertices are authored in top-left-origin screen space and
  // converted to Vulkan NDC, so the viewport must stay vertically flipped.
  viewport.y = static_cast<float>(swapchain_extent_.height);
  viewport.width = static_cast<float>(swapchain_extent_.width);
  viewport.height = -static_cast<float>(swapchain_extent_.height);
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer_, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.extent = swapchain_extent_;
  vkCmdSetScissor(command_buffer_, 0, 1, &scissor);

  const VkDeviceSize offset = 0;
  if (!scene.triangle_vertices.empty()) {
    vkCmdBindPipeline(command_buffer_,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      native_triangle_pipeline_);
    vkCmdBindVertexBuffers(command_buffer_, 0, 1, &triangle_vertex_buffer_, &offset);
    vkCmdDraw(command_buffer_,
              static_cast<uint32_t>(scene.triangle_vertices.size()),
              1,
              0,
              0);
  }

  if (!scene.line_vertices.empty()) {
    vkCmdBindPipeline(command_buffer_,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      native_line_pipeline_);
    vkCmdBindVertexBuffers(command_buffer_, 0, 1, &line_vertex_buffer_, &offset);
    vkCmdDraw(command_buffer_,
              static_cast<uint32_t>(scene.line_vertices.size()),
              1,
              0,
              0);
  }

  if (!scene.point_vertices.empty()) {
    vkCmdBindPipeline(command_buffer_,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      native_point_pipeline_);
    vkCmdBindVertexBuffers(command_buffer_, 0, 1, &point_vertex_buffer_, &offset);
    vkCmdDraw(command_buffer_,
              static_cast<uint32_t>(scene.point_vertices.size()),
              1,
              0,
              0);
  }

  vkCmdEndRenderPass(command_buffer_);

  result = vkEndCommandBuffer(command_buffer_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkEndCommandBuffer(native) failed with code " +
                           VkErrorString(result) + ".");
  }
  return OkStatus();
}

Status VulkanRendererBackend::RecordCommandBuffer(uint32_t image_index) {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  VkResult result = vkBeginCommandBuffer(command_buffer_, &begin_info);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkBeginCommandBuffer failed with code " +
                           VkErrorString(result) + ".");
  }

  VkImageMemoryBarrier to_transfer{};
  to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  to_transfer.oldLayout =
      texture_initialized_ ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                           : VK_IMAGE_LAYOUT_UNDEFINED;
  to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to_transfer.image = texture_image_;
  to_transfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  to_transfer.subresourceRange.baseMipLevel = 0;
  to_transfer.subresourceRange.levelCount = 1;
  to_transfer.subresourceRange.baseArrayLayer = 0;
  to_transfer.subresourceRange.layerCount = 1;
  to_transfer.srcAccessMask = texture_initialized_ ? VK_ACCESS_SHADER_READ_BIT : 0;
  to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(command_buffer_,
                       texture_initialized_ ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                            : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &to_transfer);

  VkBufferImageCopy region{};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageExtent.width = swapchain_extent_.width;
  region.imageExtent.height = swapchain_extent_.height;
  region.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(command_buffer_,
                         staging_buffer_,
                         texture_image_,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  VkImageMemoryBarrier to_shader_read = to_transfer;
  to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer_,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &to_shader_read);

  VkClearValue clear_color{};
  clear_color.color = {{static_cast<float>(clear_color_.r) / 255.0f,
                        static_cast<float>(clear_color_.g) / 255.0f,
                        static_cast<float>(clear_color_.b) / 255.0f,
                        1.0f}};

  VkRenderPassBeginInfo render_pass_begin{};
  render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin.renderPass = render_pass_;
  render_pass_begin.framebuffer = framebuffers_[image_index];
  render_pass_begin.renderArea.offset = {0, 0};
  render_pass_begin.renderArea.extent = swapchain_extent_;
  render_pass_begin.clearValueCount = 1;
  render_pass_begin.pClearValues = &clear_color;

  vkCmdBeginRenderPass(command_buffer_, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

  VkViewport viewport{};
  // Uploaded software-raster images are already stored in top-down image space,
  // so keep the presentation viewport unflipped on this path.
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapchain_extent_.width);
  viewport.height = static_cast<float>(swapchain_extent_.height);
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer_, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.extent = swapchain_extent_;
  vkCmdSetScissor(command_buffer_, 0, 1, &scissor);

  vkCmdBindDescriptorSets(command_buffer_,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_,
                          0,
                          1,
                          &descriptor_set_,
                          0,
                          nullptr);
  vkCmdDraw(command_buffer_, 3, 1, 0, 0);
  vkCmdEndRenderPass(command_buffer_);

  result = vkEndCommandBuffer(command_buffer_);
  if (result != VK_SUCCESS) {
    return ErrorStatus(StatusCode::kInternalError,
                       "vkEndCommandBuffer failed with code " +
                           VkErrorString(result) + ".");
  }
  return OkStatus();
}

void VulkanRendererBackend::DestroySwapchainResources() {
  for (VkFramebuffer framebuffer : framebuffers_) {
    vkDestroyFramebuffer(device_, framebuffer, nullptr);
  }
  framebuffers_.clear();

  if (descriptor_pool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
    descriptor_pool_ = VK_NULL_HANDLE;
  }
  if (sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(device_, sampler_, nullptr);
    sampler_ = VK_NULL_HANDLE;
  }
  if (texture_image_view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(device_, texture_image_view_, nullptr);
    texture_image_view_ = VK_NULL_HANDLE;
  }
  if (texture_image_ != VK_NULL_HANDLE) {
    vkDestroyImage(device_, texture_image_, nullptr);
    texture_image_ = VK_NULL_HANDLE;
  }
  if (texture_image_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_, texture_image_memory_, nullptr);
    texture_image_memory_ = VK_NULL_HANDLE;
  }
  if (staging_buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(device_, staging_buffer_, nullptr);
    staging_buffer_ = VK_NULL_HANDLE;
  }
  if (staging_buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_, staging_buffer_memory_, nullptr);
    staging_buffer_memory_ = VK_NULL_HANDLE;
  }
  staging_buffer_size_ = 0;

  if (pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, pipeline_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
  }
  if (native_point_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, native_point_pipeline_, nullptr);
    native_point_pipeline_ = VK_NULL_HANDLE;
  }
  if (native_line_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, native_line_pipeline_, nullptr);
    native_line_pipeline_ = VK_NULL_HANDLE;
  }
  if (native_triangle_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, native_triangle_pipeline_, nullptr);
    native_triangle_pipeline_ = VK_NULL_HANDLE;
  }
  if (native_pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, native_pipeline_layout_, nullptr);
    native_pipeline_layout_ = VK_NULL_HANDLE;
  }
  if (pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    pipeline_layout_ = VK_NULL_HANDLE;
  }
  if (descriptor_set_layout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
    descriptor_set_layout_ = VK_NULL_HANDLE;
  }
  if (render_pass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }

  if (triangle_vertex_buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(device_, triangle_vertex_buffer_, nullptr);
    triangle_vertex_buffer_ = VK_NULL_HANDLE;
  }
  if (triangle_vertex_buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_, triangle_vertex_buffer_memory_, nullptr);
    triangle_vertex_buffer_memory_ = VK_NULL_HANDLE;
  }
  triangle_vertex_buffer_size_ = 0;

  if (line_vertex_buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(device_, line_vertex_buffer_, nullptr);
    line_vertex_buffer_ = VK_NULL_HANDLE;
  }
  if (line_vertex_buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_, line_vertex_buffer_memory_, nullptr);
    line_vertex_buffer_memory_ = VK_NULL_HANDLE;
  }
  line_vertex_buffer_size_ = 0;

  if (point_vertex_buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(device_, point_vertex_buffer_, nullptr);
    point_vertex_buffer_ = VK_NULL_HANDLE;
  }
  if (point_vertex_buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device_, point_vertex_buffer_memory_, nullptr);
    point_vertex_buffer_memory_ = VK_NULL_HANDLE;
  }
  point_vertex_buffer_size_ = 0;

  for (VkImageView image_view : swapchain_image_views_) {
    vkDestroyImageView(device_, image_view, nullptr);
  }
  swapchain_image_views_.clear();

  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  swapchain_images_.clear();
  swapchain_extent_ = {};
  texture_initialized_ = false;
}

void VulkanRendererBackend::DestroyAll() {
  if (device_ != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device_);
    DestroySwapchainResources();
  }

  if (image_available_semaphore_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(device_, image_available_semaphore_, nullptr);
    image_available_semaphore_ = VK_NULL_HANDLE;
  }
  if (render_finished_semaphore_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(device_, render_finished_semaphore_, nullptr);
    render_finished_semaphore_ = VK_NULL_HANDLE;
  }
  if (in_flight_fence_ != VK_NULL_HANDLE) {
    vkDestroyFence(device_, in_flight_fence_, nullptr);
    in_flight_fence_ = VK_NULL_HANDLE;
  }
  if (command_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, command_pool_, nullptr);
    command_pool_ = VK_NULL_HANDLE;
  }
  command_buffer_ = VK_NULL_HANDLE;

  if (device_ != VK_NULL_HANDLE) {
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
  if (window_surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance_, window_surface_, nullptr);
    window_surface_ = VK_NULL_HANDLE;
  }
  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }

  graphics_queue_ = VK_NULL_HANDLE;
  present_queue_ = VK_NULL_HANDLE;
  physical_device_ = VK_NULL_HANDLE;
  queue_families_ = {};
  descriptor_set_ = VK_NULL_HANDLE;
  window_initialized_ = false;
}

}  // namespace

std::unique_ptr<IRendererBackend> CreateVulkanRendererBackend() {
  return std::make_unique<VulkanRendererBackend>();
}

}  // namespace navscene::render

#else

namespace navscene::render {

std::unique_ptr<IRendererBackend> CreateVulkanRendererBackend() {
  return nullptr;
}

}  // namespace navscene::render

#endif
