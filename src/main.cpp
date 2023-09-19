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

    glfwMakeContextCurrent(window);
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawPixels(100, 100, GL_RGB, GL_UNSIGNED_BYTE, data);
        glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    return 0;
}

