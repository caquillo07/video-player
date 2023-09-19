#include <GLFW/glfw3.h>
#include <cstdio>

int main() {
    GLFWwindow *window;
    if (!glfwInit()) {
        printf("failed to init glfw\n");
        return 1;
    }

    window = glfwCreateWindow(640, 480, "Hello World", nullptr, nullptr);
    if (!window) {
        printf("failed to create window\n");
        glfwTerminate();
        return 1;
    }

    auto *data = new unsigned char[100 * 100 * 3];
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 100; x++) {
            // RGB
            // 0xff0000 -> red
            // it's a flat buffer, so we have to calculate the offset
            // 0xff0000 -> 0xff, 0x00, 0x00
            data[y * 100 * 3 + x * 3 + 0] = 0xff;
            data[y * 100 * 3 + x * 3 + 1] = 0x00;
            data[y * 100 * 3 + x * 3 + 2] = 0x00;
        }
    }
    for (int y = 25; y < 75; y++) {
        for (int x = 25; x < 75; x++) {
            // RGB
            // 0xff0000 -> red
            // it's a flat buffer, so we have to calculate the offset
            // 0xff0000 -> 0xff, 0x00, 0x00
            data[y * 100 * 3 + x * 3 + 0] = 0x00;
            data[y * 100 * 3 + x * 3 + 1] = 0x00;
            data[y * 100 * 3 + x * 3 + 2] = 0xff;
        }
    }

    // make the window's context current. this HAS to be done before the texture
    // is generated, otherwise it will not be added to the right context.
    glfwMakeContextCurrent(window);

    // load the above in a texture for caching in gpu
    GLuint texture_handle;
    glGenTextures(1, &texture_handle);
    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,  GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,  GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 100, 100, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);

        // how things are projected, how they are rendered, the camera essentially.
        glMatrixMode(GL_PROJECTION); // make it orthographic projection
        glLoadIdentity();
        // map the coordinates map the ones from the screen
        glOrtho(0, window_width, 0, window_height, -1, 1); // left, right, bottom, top, near, far
        // how things are placed in the world, the coordinates
        glMatrixMode(GL_MODELVIEW);

        // now we render what we want
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture_handle);
        glBegin(GL_QUADS);
            glTexCoord2d(0,0); glVertex2d(0,0);
            glTexCoord2d(1,0); glVertex2d(100,0);
            glTexCoord2d(1,1); glVertex2d(100,100);
            glTexCoord2d(0,1); glVertex2d(0,100);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    return 0;
}

