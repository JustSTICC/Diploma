#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <iostream>


#define ASSERT_VK_RESULT(result, msg) \
    if (result != VK_SUCCESS) { \
        std::cerr << msg << " - Error Code: " << result << std::endl; \
        throw std::runtime_error(msg); \
    } \

#define VK_VERIFY(cond) Assert((cond), __FILE__, __LINE__, #cond)
#define VK_ASSERT(cond) (void)VK_VERIFY(cond)
#define VK_ASSERT_MSG(cond, format, ...) (void)::Assert((cond), __FILE__, __LINE__, (format), ##__VA_ARGS__)

class VulkanValidator
{
public:
	VulkanValidator();
    ~VulkanValidator(){}

    void cleanup(VkInstance* instance);

	bool isValidationEnabled() const { return enableValidationLayers; }

	const std::vector<const char*>& getValidationLayers() const { return validationLayers; }

    void setupDebugMessenger(VkInstance* instance);

    bool checkValidationLayerSupport();

    void setUtilMessenger(VkInstanceCreateInfo* createInfo);

private:

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool enableValidationLayers;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

struct Result {
    enum class Code {
        Ok,
        ArgumentOutOfRange,
        RuntimeError,
    };

    Code code = Code::Ok;
    const char* message = "";
    explicit Result() = default;
    explicit Result(Code code, const char* message = "") : code(code), message(message) {}

    bool isOk() const {
        return code == Result::Code::Ok;
    }

    static void setResult(Result* outResult, Code code, const char* message = "") {
        if (outResult) {
            outResult->code = code;
            outResult->message = message;
        }
    }

    static void setResult(Result* outResult, const Result& sourceResult) {
        if (outResult) {
            *outResult = sourceResult;
        }
    }
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance* instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance* instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);

VkResult setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, const char* name);

bool Assert(bool cond, const char* file, int line, const char* format, ...);

