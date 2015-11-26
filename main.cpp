#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <utils/maths_funcs.h>
#include <utils/quat_funcs.h>

struct Hardware{

    GLFWmonitor *mon;
    const GLFWvidmode* vmode;
};
struct Camera{

    float pos[3]; // don't start at zero, or we will be too close
    float yaw = 0.0f; // y-rotation in degrees
    float pitch = 0.0f;

    float signal_amplifier = 0.1f;

    mat4 T;
    mat4 R;
    mat4 R2;
    mat4 viewMatrix;

    GLint view_mat_location;
    GLint proj_mat_location;

    float resultQuat[4];
    float quatYaw[4];
    float quatPitch[4];

};

static Camera camera;
static Hardware hardware;

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);


int main () {
    GLFWwindow* window = NULL;
    const GLubyte* renderer;
    const GLubyte* version;
    GLuint vao;
    GLuint vbo;

    /**Triangle Coordinates*/
    GLfloat points[] = {
            0.0f, 0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,

            0.5f, -0.5f, 0.0f,
            0.5, -0.5f, 1.0,
            0.5f, 0.5f, 0.5f,

            -0.5f, -0.5f, 1.0f,
            -0.5f,-0.5f, 0.0f,
            -0.5f,0.5f, 0.5f,

            0.0f, 0.5f, 1.0f,
            0.5f, -0.5f, 1.0f,
            -0.5f, -0.5f, 1.0f,

    };

    /*Shader Stuff*/
    const char* vertex_shader =
            "#version 410\n"
                    "uniform mat4 view, proj;"
                    "in vec3 vertex_points;"

                    "void main () {"
                    "	gl_Position = proj * view * vec4 (vertex_points, 1.0);"
                    "}";
    const char* fragment_shader =
            "#version 410\n"
                    "out vec4 fragment_colour;"
                    "void main () {"
                    "	fragment_colour = vec4 (0.5, 0.0, 0.5, 1.0);"
                    "}";

    GLuint vs, fs;
    GLuint shader_programme;

    /* start GL context and O/S window using the GLFW helper library */
    if (!glfwInit ()) {
        fprintf (stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    hardware = {};

    hardware.mon = glfwGetPrimaryMonitor();
    hardware.vmode = glfwGetVideoMode(hardware.mon);

    window = glfwCreateWindow (hardware.vmode->width, hardware.vmode->height, "Hello World", hardware.mon, NULL);
    if (!window) {
        fprintf (stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent (window);

    /* start GLEW extension handler */
    glewExperimental = GL_TRUE;
    glewInit ();

    glfwSetCursorPosCallback(window,cursor_position_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /* get version info */
    renderer = glGetString (GL_RENDERER); /* get renderer string */
    version = glGetString (GL_VERSION); /* version as a string */
    printf ("Renderer: %s\n", renderer);
    printf ("OpenGL version supported %s\n", version);

    glEnable (GL_DEPTH_TEST); /* enable depth-testing */
    glDepthFunc (GL_LESS);


    glGenBuffers (1, &vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glBufferData (GL_ARRAY_BUFFER, 36 * sizeof (GLfloat), points,
                  GL_STATIC_DRAW);

    glGenVertexArrays (1, &vao);
    glBindVertexArray (vao);
    glEnableVertexAttribArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);


    vs = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (vs, 1, &vertex_shader, NULL);
    glCompileShader (vs);
    fs = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (fs, 1, &fragment_shader, NULL);
    glCompileShader (fs);
    shader_programme = glCreateProgram ();
    glAttachShader (shader_programme, fs);
    glAttachShader (shader_programme, vs);
    glLinkProgram (shader_programme);


    // camera stuff
#define PI 3.14159265359
#define DEG_TO_RAD (2.0 * PI) / 360.0

    float near = 0.1f;
    float far = 100.0f;
    float fov = 67.0f * DEG_TO_RAD;
    float aspect = (float)hardware.vmode->width /(float)hardware.vmode->height;

    // matrix components
    float range = tan (fov * 0.5f) * near;
    float Sx = (2.0f * near) / (range * aspect + range * aspect);
    float Sy = near / range;
    float Sz = -(far + near) / (far - near);
    float Pz = -(2.0f * far * near) / (far - near);
    GLfloat proj_mat[] = {
            Sx, 0.0f, 0.0f, 0.0f,
            0.0f, Sy, 0.0f, 0.0f,
            0.0f, 0.0f, Sz, -1.0f,
            0.0f, 0.0f, Pz, 0.0f
    };

    camera = {};

    //create view matrix
    camera.pos[0] = 0.0f; // don't start at zero, or we will be too close
    camera.pos[1] = 0.0f; // don't start at zero, or we will be too close
    camera.pos[2] = 0.5f; // don't start at zero, or we will be too close
    camera.T = translate (identity_mat4 (), vec3 (-camera.pos[0], -camera.pos[1], -camera.pos[2]));
    camera.R = rotate_y_deg (identity_mat4 (), -camera.yaw);
    camera.R2 = rotate_y_deg (identity_mat4 (), -camera.yaw);
    camera.viewMatrix = camera.R * camera.T;

    glUseProgram(shader_programme);

    camera.view_mat_location = glGetUniformLocation(shader_programme, "view");
    camera.proj_mat_location = glGetUniformLocation(shader_programme, "proj");

    glUniformMatrix4fv(camera.view_mat_location, 1, GL_FALSE, camera.viewMatrix.m);
    glUniformMatrix4fv(camera.proj_mat_location, 1, GL_FALSE, proj_mat);


    while (!glfwWindowShouldClose (window)) {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport (0, 0, hardware.vmode->width,hardware.vmode->height);
        glUseProgram (shader_programme);
        glBindVertexArray (vao);
        glDrawArrays (GL_TRIANGLES, 0, 12);
        glfwPollEvents ();

        if (GLFW_PRESS == glfwGetKey (window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose (window, 1);
        }
        glfwSwapBuffers (window);
    }

    /* close GL context and any other GLFW resources */
    glfwTerminate();
    return 0;
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {

    printf("Mouse moved at X: %f Y:%f\n", xpos, ypos);

    //calculate pitch
    static double  previous_ypos = ypos;
    double position_y_difference = ypos - previous_ypos;
    previous_ypos = ypos;

    //calculate yaw
    static double previous_xpos = xpos;
    double position_x_difference = xpos - previous_xpos;
    previous_xpos = xpos;

    camera.yaw += position_x_difference *camera.signal_amplifier;
    camera.pitch += position_y_difference *camera.signal_amplifier;

    create_versor(camera.quatPitch, -camera.pitch, 1.0f, 0.0f, 0.0f);
    create_versor(camera.quatYaw, -camera.yaw, 0.0f, 1.0f, 0.0f);

    quat_to_mat4(camera.R.m, camera.quatPitch);
    quat_to_mat4(camera.R2.m, camera.quatYaw);


//    mult_quat_quat(camera.resultQuat, camera.quatYaw, camera.quatPitch);
//    mult_quat_quat(camera.resultQuat, camera.quatYaw, camera.quatPitch);
//    quat_to_mat4(camera.R.m, camera.resultQuat);

    //first find Translation, then pitch, then yaw
    camera.viewMatrix = camera.R *camera.R2 * camera.T;
    glUniformMatrix4fv (camera.view_mat_location, 1, GL_FALSE, camera.viewMatrix.m);

}





