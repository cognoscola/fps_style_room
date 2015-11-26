//TODO change project name in CMakeLists.txt

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <utils/maths_funcs.h>

static float cam_yaw = 0.0f;

struct Camera{

    float speed = 1.0f; // 1 unit per second
    float yaw_speed = 10.0f; // 10 degrees per second
    float pos[3]; // don't start at zero, or we will be too close
    float yaw = 0.0f; // y-rotation in degrees
    float pitch = 0.0f;

    float heading_speed = 100.0f;
    float heading = 0.0f;

    mat4 T;
    mat4 R;
    mat4 viewMatrix;

    GLint view_mat_location;
    GLint proj_mat_location;

};

static Camera camera;

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);

/* create a unit quaternion q from an angle in degrees a, and an axis x,y,z */
void create_versor (float* q, float a, float x, float y, float z) {
    float rad = ONE_DEG_IN_RAD * a;
    q[0] = cosf (rad / 2.0f);
    q[1] = sinf (rad / 2.0f) * x;
    q[2] = sinf (rad / 2.0f) * y;
    q[3] = sinf (rad / 2.0f) * z;
}

/* convert a unit quaternion q to a 4x4 matrix m */
void quat_to_mat4 (float* m, float* q) {
    float w = q[0];
    float x = q[1];
    float y = q[2];
    float z = q[3];
    m[0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
    m[1] = 2.0f * x * y + 2.0f * w * z;
    m[2] = 2.0f * x * z - 2.0f * w * y;
    m[3] = 0.0f;
    m[4] = 2.0f * x * y - 2.0f * w * z;
    m[5] = 1.0f - 2.0f * x * x - 2.0f * z * z;
    m[6] = 2.0f * y * z + 2.0f * w * x;
    m[7] = 0.0f;
    m[8] = 2.0f * x * z + 2.0f * w * y;
    m[9] = 2.0f * y * z - 2.0f * w * x;
    m[10] = 1.0f - 2.0f * x * x - 2.0f * y * y;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
}

/* normalise a quaternion in case it got a bit mangled */
void normalise_quat (float* q) {
    // norm(q) = q / magnitude (q)
    // magnitude (q) = sqrt (w*w + x*x...)
    // only compute sqrt if interior sum != 1.0
    float sum = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    // NB: floats have min 6 digits of precision
    const float thresh = 0.0001f;
    if (fabs (1.0f - sum) < thresh) {
        return;
    }
    float mag = sqrt (sum);
    for (int i = 0; i < 4; i++) {
        q[i] = q[i] / mag;
    }
}

/* multiply quaternions to get another one. result=R*S */
void mult_quat_quat (float* result, float* r, float* s) {
    result[0] = s[0] * r[0] - s[1] * r[1] -
                s[2] * r[2] - s[3] * r[3];
    result[1] = s[0] * r[1] + s[1] * r[0] -
                s[2] * r[3] + s[3] * r[2];
    result[2] = s[0] * r[2] + s[1] * r[3] +
                s[2] * r[0] - s[3] * r[1];
    result[3] = s[0] * r[3] - s[1] * r[2] +
                s[2] * r[1] + s[3] * r[0];
    // re-normalise in case of mangling
    normalise_quat (result);
}

int main () {
    GLFWwindow* window = NULL;
    const GLubyte* renderer;
    const GLubyte* version;
    GLuint vao;
    GLuint vbo;

    /**Triangle Coordinates*/
    GLfloat points[] = {
            0.0f,	0.5f,	0.0f,
            0.5f, -0.5f,	0.0f,
            -0.5f, -0.5f,	0.0f
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

    window = glfwCreateWindow (640, 480, "Hello World", NULL, NULL);
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


    /* get version info */
    renderer = glGetString (GL_RENDERER); /* get renderer string */
    version = glGetString (GL_VERSION); /* version as a string */
    printf ("Renderer: %s\n", renderer);
    printf ("OpenGL version supported %s\n", version);

    glEnable (GL_DEPTH_TEST); /* enable depth-testing */
    glDepthFunc (GL_LESS);


    glGenBuffers (1, &vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glBufferData (GL_ARRAY_BUFFER, 9 * sizeof (GLfloat), points,
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
    float aspect = (float)640 /(float)480; //todo make a struct holding window information

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
    camera.speed = 1.0f; // 1 unit per second
    camera.yaw_speed = 10.0f; // 10 degrees per second
    camera.pos[0] = 0.0f; // don't start at zero, or we will be too close
    camera.pos[1] = 0.0f; // don't start at zero, or we will be too close
    camera.pos[2] = 2.0f; // don't start at zero, or we will be too close
    camera.T = translate (identity_mat4 (), vec3 (-camera.pos[0], -camera.pos[1], -camera.pos[2]));
    camera.R = rotate_y_deg (identity_mat4 (), -cam_yaw);
    camera.viewMatrix = camera.R * camera.T;

    glUseProgram(shader_programme);

    camera.view_mat_location = glGetUniformLocation(shader_programme, "view");
    camera.proj_mat_location = glGetUniformLocation(shader_programme, "proj");

    glUniformMatrix4fv(camera.view_mat_location, 1, GL_FALSE, camera.viewMatrix.m);
    glUniformMatrix4fv(camera.proj_mat_location, 1, GL_FALSE, proj_mat);


    while (!glfwWindowShouldClose (window)) {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport (0, 0, 640, 480);
        glUseProgram (shader_programme);
        glBindVertexArray (vao);
        glDrawArrays (GL_TRIANGLES, 0, 3);
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

    camera.yaw += position_x_difference;
    camera.pitch += position_y_difference;

//    camera.R = rotate_y_deg (identity_mat4 (), -camera.yaw); //

    float resultQuat[4];
    float quatYaw[4];
    float quatPitch[4];
    create_versor(quatYaw, -camera.yaw, 0.0f, 1.0f, 0.0f);
    create_versor(quatPitch, -camera.pitch, 1.0f, 0.0f, 0.0f);
    mult_quat_quat(resultQuat, quatYaw, quatPitch);
    quat_to_mat4(camera.R.m, resultQuat);



    camera.viewMatrix = camera.R * camera.T;
    glUniformMatrix4fv (camera.view_mat_location, 1, GL_FALSE, camera.viewMatrix.m);

}





