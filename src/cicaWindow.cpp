#include "cicaWindow.h"

//==========================================================================================================================================================
//==========================================================================================================================================================
cicaWindow::cicaWindow() { initWindow(); }

//==========================================================================================================================================================
//==========================================================================================================================================================
void cicaWindow::initWindow() {
    glfwInit();                                     // Init API
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API ); // Disable OpenGL Context
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );   // Disable Resize
    m_window = glfwCreateWindow( WIDTH, HEIGHT, "Cicada-Engine", nullptr, nullptr );
}

//==========================================================================================================================================================
//==========================================================================================================================================================
void cicaWindow::createSurface( const cicaInstance &_cicaInstance ) {
    VkSurfaceKHR _surface;
    if ( glfwCreateWindowSurface( *_cicaInstance.GetInstance(), m_window, nullptr, &_surface ) != 0 ) {
        throw std::runtime_error( "failed to create window surface!" );
    }
    m_surface = vk::raii::SurfaceKHR( _cicaInstance.GetInstance(), _surface );
}

//==========================================================================================================================================================
//==========================================================================================================================================================
GLFWwindow *cicaWindow::GrabWindow() const {
    if ( m_window == nullptr ) {
        assert( "You are returning a cicaWindow::m_window that is nullptr" ); // To replace with an std::exception
    }
    return m_window;
}