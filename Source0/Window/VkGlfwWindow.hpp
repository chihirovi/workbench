#pragma once
// #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1 
#include <vulkan/vulkan.hpp>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <boost_1_88_0/boost/move/move.hpp>

class UniqueVkGlfwWindow{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueVkGlfwWindow);

public:
    GLFWwindow* window_handler = nullptr;

    //window info
    uint32_t size[2];
    float scale = 100.0f;

	//mouse 
    bool reset_mouse_offset = false;
	float mouse_location[2] = {0, 0};
	float mouse_offset[2] = {0, 0};

	
	//[W,A,S,D,space,X,R]
	bool key_response[7];

	//fps counter
	double last_time=0;
	int frame_count = 0;

    //fps limit
    double fps_limit  = 1.0 / 60.0;
    
    //time delta
    double pre_time = 0.0;

public:
    UniqueVkGlfwWindow(BOOST_RV_REF(UniqueVkGlfwWindow) rhs);
    UniqueVkGlfwWindow& operator=(BOOST_RV_REF(UniqueVkGlfwWindow) rhs);
                                                                    
    UniqueVkGlfwWindow(){;}
    UniqueVkGlfwWindow(uint32_t w, uint32_t h, char* title);
    ~UniqueVkGlfwWindow();
    void set_cursor_visible();
    void set_instance_extention(std::vector<const char *>* inst_req_ext);
    VkSurfaceKHR get_VkSurfaceKHR(vk::UniqueInstance& instance);
	bool poll_events();
	bool* get_key_response();
	uint32_t* get_size();
    uint32_t w();
    uint32_t h();
	float get_scale();
	float* get_mouse_location();
	float* get_mouse_offset(); 
    bool is_mouse_left_botton_pressed();
    bool is_cursor_in_window();
    void set_fps_limit(double limit);
    double time_delta();
    void loop(std::function<void()> process);

private:	
	static void wheel(GLFWwindow* window, double x, double y);
	void reset_key_response(void);
};