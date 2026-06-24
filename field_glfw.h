#pragma once

#include <GLFW/glfw3.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define _MULTI_THREADED
#include <pthread.h>

#include "field.h"
#define FIELD_TRANSPARENCY 1

GLFWwindow* window;

static bool _switch_on = true;
static bool _window_on = true;
static bool _handle_events = true;

pthread_t field_thread;
pthread_t event_thread;

pthread_mutex_t _main_lock;
pthread_mutex_t _screen_lock;

pthread_barrier_t _init_barrier;
pthread_barrier_t _exit_barrier;

pthread_cond_t _repaint_condition;
pthread_cond_t _escape_condition;

/*****************************************************************************************************************************/

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    hit_test(context, (int)x, (int)y);
    if(context->current != 0)
    {
        scroll_sector[context->node->at[context->current].type](&context->node->at[context->current], (int)xoffset, (int)yoffset);
    }
    context->current = 0;
}

static void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    if(context->drag)
        hit_test_drag(context, (int)x, (int)y);
    else 
        hit_test(context, (int)x, (int)y);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int/* mods*/)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    switch(button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
        {
            if(action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);

                hit_test_down(context, (int)x, (int)y, LMB);

                if(context->node->at[context->current].flags & MOVEABLE)
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            }
            else if(action == GLFW_RELEASE) {
                double x, y;

                glfwGetCursorPos(window, &x, &y);
                hit_test_up(context, (int)x, (int)y, LMB);
                context->move = false;

                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        break;

        case GLFW_MOUSE_BUTTON_RIGHT:
        {
            if(action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                hit_test_down(context, (int)x, (int)y, RMB);
            }
            else if(action == GLFW_RELEASE)
                printf("RMB Released\n");
        }
        break;

        case GLFW_MOUSE_BUTTON_MIDDLE:
        {
            if(action == GLFW_PRESS)
                printf("MMB Pressed\n");
            else if(action == GLFW_RELEASE)
                printf("MMB Released\n");
        }
        break;

    }
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
    printf("Error: %d\n", error);
}

static void key_callback(GLFWwindow* /*window*/, int key, int/* scancode*/, int action, int/* mods*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        _window_on = false;
    }
}

void window_pos_callback(GLFWwindow* window, int, int)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);
}

static GLuint get_buffer(Frame* frame) 
{
	GLuint texture;

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame->width, frame->height, 0, GL_ABGR_EXT, GL_UNSIGNED_BYTE, frame->data);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return texture;
}

static void update_buffer(Frame* frame, GLuint texture) 
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height, GL_ABGR_EXT, GL_UNSIGNED_BYTE, frame->data);
    
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 0.0f);
    glEnd();    

    glBindTexture( GL_TEXTURE_2D, 0);
}

/*****************************************************************************************************************************/

void* events_process(void*)
{
    pthread_barrier_wait(&_init_barrier);

    while(_handle_events)
    {
        glfwWaitEvents();
        pthread_cond_signal(&_repaint_condition);
    }

    printf("Event handler terminated...\n");
    pthread_barrier_wait(&_exit_barrier);

    return nullptr;
}

void* draw_field(void* arg)
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) exit(EXIT_FAILURE);

    printf("Compiled against GLFW %i.%i.%i\n", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);
    printf("%s\n", glfwGetVersionString());

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 8); 

    Field* context = (Field*)arg;

    window = glfwCreateWindow(context->layer[FG]->width, context->layer[FG]->height, "FIELD", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowUserPointer(window, context);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowPosCallback(window, window_pos_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.f, 0.f, 0.f, 0.f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    pthread_barrier_wait(&_init_barrier);
    printf("GLFW Initialized...\n");

    GLuint bg = get_buffer(context->layer[BG]);
    GLuint ng = get_buffer(context->layer[NG]);
    GLuint fg = get_buffer(context->layer[FG]);
    GLuint st = get_buffer(context->layer[ST]);
    constexpr uint32_t CORD_NORMAL = GRAY;
    constexpr uint32_t CORD_SELECT = GREEN;

    #ifdef DEBUG_OVERLAY
        GLuint debug_layer = get_buffer(context->layer[DO]);
    #endif

    while (_window_on)
    {
        pthread_mutex_lock(&_screen_lock);
        pthread_cond_wait(&_repaint_condition, &_screen_lock);
        pthread_mutex_unlock(&_screen_lock);

        #ifdef DEBUG_OVERLAY
            if(atomic_load(&force_repaint)) context->repaint = true;
        #endif
        
        bool swap = false;
        
        if(context->repaint)
        {
            glClear(GL_COLOR_BUFFER_BIT);
            glColor3f(1.0f, 1.0f, 1.0f);
            draw_scene(context);
           
            update_buffer(context->layer[BG], bg);
            update_buffer(context->layer[NG], ng);
            update_buffer(context->layer[FG], fg);

            if(context->staging) {
                update_buffer(context->layer[ST], st);
            }

            #ifdef DEBUG_OVERLAY
                if(atomic_load_explicit(&force_repaint, memory_order_acquire)) {
                    atomic_store_explicit(&force_repaint, false, memory_order_release);
                }
                update_buffer(context->layer[DO], debug_layer);
            #endif

            swap = true;
        }

        if(context->connecting)
        {
            glLineWidth(2.0f); 
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, context->pressed->data);

            glColor4ub(
                extract_byte(CORD_SELECT, 3),
                extract_byte(CORD_SELECT, 2),
                extract_byte(CORD_SELECT, 1),
                extract_byte(CORD_SELECT, 0)
            );

            glDrawArrays(GL_LINE_STRIP, 0, SPLINE_SEGMENTS);
            glDisableClientState(GL_VERTEX_ARRAY);
            swap = true;
        }

        if(swap)
        {
            for(uint32_t n = 0; n < context->nodes; ++n) {
                for(uint32_t i = 0; i < context->node[n].entities; ++i) {
                    auto entity = &context->node[n].at[i];
                    if(entity->connected && entity->has_data) {
                       
                        auto colour = entity->hovered || entity->connection->hovered ? CORD_SELECT : CORD_NORMAL;

                        glLineWidth(2.0f); 
                        glEnableClientState(GL_VERTEX_ARRAY);
                        glVertexPointer(2, GL_FLOAT, 0, context->node[n].at[i].data);

                        glColor4ub(
                            extract_byte(colour, 3),
                            extract_byte(colour, 2),
                            extract_byte(colour, 1),
                            extract_byte(colour, 0)
                        );

                        glDrawArrays(GL_LINE_STRIP, 0, SPLINE_SEGMENTS);
                        glDisableClientState(GL_VERTEX_ARRAY);
                    }
                }
            }

            glfwSwapBuffers(window);  
        }          
    }
    
    glDeleteTextures(1, &bg);
    glDeleteTextures(1, &fg);
    glDeleteTextures(1, &st);

    pthread_cond_signal(&_escape_condition);

    _switch_on      = false;
    _handle_events  = false;
    glfwPostEmptyEvent();

    destroyField(context);
    glfwSetWindowShouldClose(window, GL_TRUE);
    glfwDestroyWindow(window);

    glfwTerminate();
    printf("GLFW Teminated...\n");

    pthread_barrier_wait(&_exit_barrier);

    return nullptr;
}

static void field_loop(Field* restrict context) {
    pthread_mutex_init(&_screen_lock, NULL);
    pthread_mutex_init(&_main_lock, NULL);

    pthread_barrier_init(&_init_barrier, NULL, 2);
    printf("Creating threads\n");

    pthread_create(&field_thread, NULL, draw_field, context);
    pthread_create(&event_thread, NULL, events_process, NULL);

    pthread_barrier_destroy(&_init_barrier);

    while (_switch_on)
    {   
        pthread_mutex_lock(&_main_lock);
        pthread_cond_wait(&_escape_condition, &_main_lock);
        pthread_mutex_unlock(&_main_lock);
    }

    pthread_barrier_init(&_exit_barrier, NULL, 3);
    pthread_barrier_wait(&_exit_barrier);
    pthread_barrier_destroy(&_exit_barrier);

    printf("Main Terminated...\n");
    pthread_mutex_destroy(&_screen_lock);
    pthread_mutex_destroy(&_main_lock);

    exit(EXIT_SUCCESS);
}
