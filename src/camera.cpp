#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 mvp_matrix(const Camera &cam, float width, float height)
{
    float aspect_ratio = width / height;
    float near_plane = 0.1f * cam.r;
    float far_plane = 8.0f * cam.r;
    float fovy = glm::radians(cam.FOV);
    glm::vec3 eye = glm::vec3(cam.r * cos(cam.phi) * sin(cam.theta), cam.r * sin(cam.phi), cam.r * cos(cam.phi) * cos(cam.theta));

    glm::mat4 proj = glm::perspective(fovy, aspect_ratio, near_plane, far_plane);
    glm::mat4 view = glm::lookAt(eye, cam.CENTER, cam.UP);
    glm::mat4 mvp = proj * view;
    return mvp;
}
