#include "VkGlfwWindow.hpp"

UniqueVkGlfwWindow::UniqueVkGlfwWindow(uint32_t w, uint32_t h, char* title):size{w,h}{
    if(!glfwInit()) printf("ERROR: glfw init error.\n");
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    window_handler = glfwCreateWindow(w, h, title, NULL, NULL);
    
    if(!window_handler){        
        printf("ERROR: glfw window create error.\n");
        const char* err;
        glfwGetError(&err);
        printf("%s\n",err);
    }

    glfwSetWindowUserPointer(window_handler, this);

    //glfwSetScrollCallback(window_handler, wheel);

    glfwSetInputMode(window_handler, GLFW_CURSOR,GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(window_handler, GLFW_CURSOR,GLFW_CURSOR_DISABLED);
}

UniqueVkGlfwWindow::~UniqueVkGlfwWindow(){ 
    if(this->window_handler == nullptr) return;
    glfwTerminate(); 
}

void UniqueVkGlfwWindow::set_cursor_visible(){
    glfwSetInputMode(window_handler, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void UniqueVkGlfwWindow::set_instance_extention(std::vector<const char *>* inst_req_ext){
    uint32_t req_ext_count;
    const char ** req_ext = glfwGetRequiredInstanceExtensions(&req_ext_count);

    for(size_t i=0; i<req_ext_count; i++) 
        inst_req_ext->push_back(req_ext[i]);
}

VkSurfaceKHR UniqueVkGlfwWindow::get_VkSurfaceKHR(vk::UniqueInstance& instance){
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(instance.get(), window_handler, nullptr, &surface);

    if (result != VK_SUCCESS) {
        const char* err;
        glfwGetError(&err);
        printf("getvksurface: %s\n",err);
    }

    return surface;
}

//return window is alive
bool UniqueVkGlfwWindow::poll_events(){
    glfwPollEvents();
    
    //mouse event
    double x, y;
    glfwGetCursorPos(window_handler, &x, &y);
        
    if(reset_mouse_offset){
        mouse_offset[0] = 0;
        mouse_offset[1] = 0;
        reset_mouse_offset = false;
    }

    mouse_offset[0] += (float)x - mouse_location[0];
    mouse_offset[1] += (float)y - mouse_location[1];

    mouse_location[0] = (float)x;
    mouse_location[1] = (float)y;
    
    //key response
    reset_key_response();
    if (glfwGetKey(window_handler, GLFW_KEY_W)) { key_response[0] = true; }
    if (glfwGetKey(window_handler, GLFW_KEY_A)) {key_response[1] = true;}
    if (glfwGetKey(window_handler, GLFW_KEY_S)) {key_response[2] = true;}
    if (glfwGetKey(window_handler, GLFW_KEY_D)) {key_response[3] = true;}
    if (glfwGetKey(window_handler, GLFW_KEY_SPACE)) {key_response[4] = true;}
    if (glfwGetKey(window_handler, GLFW_KEY_X)) {key_response[5] = true;}
    if (glfwGetKey(window_handler, GLFW_KEY_R)) {key_response[6] = true;}
    
    //window close
    if(	glfwWindowShouldClose(window_handler) || glfwGetKey(window_handler, GLFW_KEY_ESCAPE)){
        return false;
    }else {
        return true;
    }
}
    

bool* UniqueVkGlfwWindow::get_key_response() { return key_response; }

uint32_t* UniqueVkGlfwWindow::get_size() {return size;}

uint32_t UniqueVkGlfwWindow::w(){return size[0];}

uint32_t UniqueVkGlfwWindow::h(){return size[1];}

float UniqueVkGlfwWindow::get_scale() {return scale;}

float* UniqueVkGlfwWindow::get_mouse_location(){ return mouse_location; }	

float* UniqueVkGlfwWindow::get_mouse_offset(){ 
    //reset_mouse_offset = true;
    return mouse_offset; 
}	

bool UniqueVkGlfwWindow::is_mouse_left_botton_pressed(){
    return glfwGetMouseButton(window_handler, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

bool UniqueVkGlfwWindow::is_cursor_in_window(){
    double x, y;
    glfwGetCursorPos(window_handler, &x, &y);

    int width, height;
    glfwGetWindowSize(window_handler, &width, &height);

    if (x >= 0 && y >= 0 && x < width && y < height) {
        return true;
    }        
    return false;
}


void UniqueVkGlfwWindow::set_fps_limit(double limit){fps_limit = 1.0/limit;}

double UniqueVkGlfwWindow::time_delta(){
    double now = glfwGetTime();
    double time_delta = now - pre_time;
    pre_time = now;
    return time_delta;
}

void UniqueVkGlfwWindow::loop(std::function<void()> process){
    
    //fps limit
    double last_update_time = 0;    // number of seconds since the last loop
    double last_frame_time  = 0;    // number of seconds since the last frame
    
    while(poll_events()){
        double now = glfwGetTime();
        //double deltaTime = now - last_update_time;

        if ((now - last_frame_time) >= fps_limit){
            process();

            last_frame_time = now;
            reset_mouse_offset = true;
        }

        last_update_time = now;
    }
}

void UniqueVkGlfwWindow::wheel(GLFWwindow* window, double x, double y) {
    UniqueVkGlfwWindow* ins = (UniqueVkGlfwWindow*)glfwGetWindowUserPointer(window);

    if (ins != NULL) {
        ins->scale += (float)(y*10.0);
    }
}

void UniqueVkGlfwWindow::reset_key_response(void) {
    for (int i = 0; i < 7; i++) key_response[i] = false;
}


UniqueVkGlfwWindow::UniqueVkGlfwWindow(BOOST_RV_REF(UniqueVkGlfwWindow) rhs){
    *this = std::move(rhs);
}

UniqueVkGlfwWindow& UniqueVkGlfwWindow::operator=(BOOST_RV_REF(UniqueVkGlfwWindow) rhs){
    if(this->window_handler != nullptr) this->~UniqueVkGlfwWindow();
    
    this->window_handler = rhs.window_handler;
    this->size[0] = rhs.size[0];
    this->size[1] = rhs.size[1];
    this->scale = rhs.scale;
    this->reset_mouse_offset = rhs.mouse_offset;
    this->mouse_location[0] = rhs.mouse_location[0];
    this->mouse_location[1] = rhs.mouse_location[1];
    this->mouse_offset[0] = rhs.mouse_offset[0];
    this->mouse_offset[1] = rhs.mouse_offset[1];
    for(int i=0; i<7; i++){
        this->key_response[i] = rhs.key_response[i];
    }
    this->last_time = rhs.last_time;
    this->frame_count = rhs.frame_count;
    this->fps_limit = rhs.fps_limit;
    this->pre_time = rhs.pre_time;
    
    rhs.window_handler = nullptr;
    
    return *this;
}