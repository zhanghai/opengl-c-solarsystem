/*
 * Copyright (c) 2016 Zhang Hai <Dreaming.in.Code.ZH@Gmail.com>
 * All Rights Reserved.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <GL/glut.h>

#include "tga.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MOON_ORBIT_RADIUS_SCALE 56
#define SUN_RADIUS_SCALE 0.1

const GLdouble ORBIT_RADIUS_FACTOR = 10;
const GLdouble BODY_ROTATION_FACTOR = 20;
GLdouble g_body_rotation_speed = 1;
const GLdouble BODY_ROTATION_SPEED_FACTOR =0.125;
GLdouble g_body_rotation_phase = 0;
const GLdouble BODY_ROTATION_PHASE_FACTOR = 0.1;

const GLuint ORBIT_COLOR = 0x3FFFFFFF;
const GLdouble ORBIT_INNER_RADIUS = 0.02;

const GLint SPHERE_SUBDIVISION_COUNT = 50;
const GLint TORUS_SIDE_DIVISION_COUNT = 10;
const GLint TORUS_RADIAL_DIVISTION_COUNT = 100;

#define MAKE_TEXTURE_PATH(name) ("texture/" name ".tga")

const char *TEXTURE_PATH_MILKY_WAY = MAKE_TEXTURE_PATH("milky_way");
GLuint TEXTURE_NAME_MILKY_WAY;
GLuint DISPLAY_LIST_MILKY_WAY;

typedef struct body_t {
    char *texture_path;
    GLuint texture_name;
    GLdouble radius; // Mean radius, in earths.
    GLuint display_list;
    GLdouble tilt; // Axis tilt to orbit, in degrees.
    GLdouble z_rotation_inverse[16];
    GLdouble period; // Sidereal rotation period, in days.
    struct {
        GLdouble inclination; // Inclination to ecliptic, in degrees.
        GLdouble radius; // Arithmetic mean of aphelion and perihelion, in AUs.
        GLuint display_list;
        GLdouble period; // Orbital period, in days.
    } orbit;
    struct body_t *planets[];
} body_t;

body_t BODY_MERCURY = { MAKE_TEXTURE_PATH("mercury"), 0, 0.3829, 0, 0.034, {},
                        58.646, { 7.005, 0.387098, 0, 87.9691 }, { NULL } };
body_t BODY_VENUS = { MAKE_TEXTURE_PATH("venus"), 0, 0.9499, 0, 2.64, {},
                      -243.025, { 3.39458, 0.723332, 0, 224.701 }, { NULL } };
body_t BODY_MOON = { MAKE_TEXTURE_PATH("moon"), 0, 0.273, 0, 27.321661, {},
                     6.687,
                     { 5.145, 0.00257 * MOON_ORBIT_RADIUS_SCALE, 0, 27.321661 },
                     { NULL } };
body_t BODY_EARTH = { MAKE_TEXTURE_PATH("earth"), 0, 1, 0, 23.4392811, {},
                      0.99726968, { 0.00005, 1, 0, 365.256363004 },
                      { &BODY_MOON, NULL } };
body_t BODY_MARS = { MAKE_TEXTURE_PATH("mars"), 0, 0.5320, 0, 1.025957, {},
                     25.19, { 1.850, 1.523679, 0, 686.971 }, { NULL } };
body_t BODY_JUPITER = { MAKE_TEXTURE_PATH("jupiter"), 0, 10.97, 0, 9.925 / 24.,
                        {}, 3.13, { 1.303, 5.20260, 0, 4332.59 }, { NULL } };
body_t BODY_SATURN = { MAKE_TEXTURE_PATH("saturn"), 0, 9.140, 0, 10.55 / 24.,
                       {}, 26.73, { 2.485240, 9.554909, 0, 10759.22 },
                       { NULL } };
body_t BODY_URANUS = { MAKE_TEXTURE_PATH("uranus"), 0, 3.981, 0, 0.71833, {},
                       97.77, { 0.773, 19.2184, 0, 30688.5 }, { NULL } };
body_t BODY_NEPTUNE = { MAKE_TEXTURE_PATH("neptune"), 0, 3.865, 0, 0.6713, {},
                        28.32, { 1.767975, 30.110387, 0, 60182 }, { NULL } };
body_t BODY_SUN = { MAKE_TEXTURE_PATH("sun"), 0, 109 * SUN_RADIUS_SCALE, 0,
                    7.25, {
                            1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            0, 0, 0, 1
                    }, 25.05, { 0, 0, 0, 0 }, {
                            &BODY_MERCURY, &BODY_VENUS, &BODY_EARTH, &BODY_MARS,
                            &BODY_JUPITER, &BODY_SATURN, &BODY_URANUS,
                            &BODY_NEPTUNE, NULL
                    } };

#undef MOON_ORBIT_RADIUS_SCALE
#undef SUN_RADIUS_SCALE

#undef MAKE_TEXTURE_PATH

GLdouble transform_body_radius(GLdouble radius) {
    if (radius > 1) {
        radius = sqrt(sqrt(radius));
    }
    return radius;
}

GLdouble transform_orbit_radius(GLdouble radius) {
    if (radius > 1) {
        radius = sqrt(radius);
    }
    return radius * ORBIT_RADIUS_FACTOR;
}

GLdouble g_tick;

void update_tick() {
    g_tick = (GLdouble) clock() / CLOCKS_PER_SEC;
}

GLdouble get_rotation(GLdouble period) {
    return (fmod(g_tick, period) * g_body_rotation_speed
            + g_body_rotation_phase) / period * 360 * BODY_ROTATION_FACTOR;
}

GLdouble g_eye[3] = { 0, 0, 64 };
const GLdouble LOOK_DISTANCE = 64;
GLdouble g_look[3] = { 0, 0, -1 };
GLdouble g_up[3] = { 0, 1, 0 };

const GLdouble MOVE_FACTOR = 1;
const GLdouble ZOOM_FACTOR = 2;

GLdouble vector_length(GLdouble vector[3]) {
    return sqrt(vector[0] * vector[0] + vector[1] * vector[1]
                + vector[2] * vector[2]);
}

void normalize_vector(GLdouble vector[3]) {
    GLdouble length = vector_length(vector);
    vector[0] = vector[0] / length;
    vector[1] = vector[1] / length;
    vector[2] = vector[2] / length;
}

void assign_vector(GLdouble destination[3], GLdouble source[3]) {
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
}

void assign_vector_normalized(GLdouble destination[3], GLdouble source[3]) {
    assign_vector(destination, source);
    normalize_vector(destination);
}

void cross_vector(GLdouble vector[3], GLdouble term[3]) {
    assign_vector(vector, (GLdouble [3]) {
            vector[1] * term[2] - vector[2] * term[1],
            vector[2] * term[0] - vector[0] * term[2],
            vector[0] * term[1] - vector[1] * term[0]
    });
}

void make_rotation_matrix(GLdouble angle, GLdouble axis[3],
                          GLdouble matrix[16]) {
    GLdouble radian = angle / 180 * M_PI;
    GLdouble s = sin(radian);
    GLdouble c = cos(radian);
    GLdouble length = vector_length(axis);
    GLdouble x = axis[0] / length;
    GLdouble y = axis[1] / length;
    GLdouble z = axis[2] / length;
    matrix[0] = x * x * (1 - c) + c;
    matrix[1] = x * y * (1 - c) - z * s;
    matrix[2] = x * z * (1 - c) + y * s;
    matrix[3] = 0;
    matrix[4] = y * x * (1 - c) + z * s;
    matrix[5] = y * y * (1 - c) + c;
    matrix[6] = y * z * (1 - c) - x * s;
    matrix[7] = 0;
    matrix[8] = x * z * (1 - c) - y * s;
    matrix[9] = y * z * (1 - c) + x * s;
    matrix[10] = z * z * (1 - c) + c;
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

void multiply_vector_by_matrix(GLdouble vector[3], GLdouble matrix[16]) {
    GLdouble x = vector[0];
    GLdouble y = vector[1];
    GLdouble z = vector[2];
    vector[0] = x * matrix[0] + y * matrix[1] + z * matrix[2] + matrix[3];
    vector[1] = x * matrix[4] + y * matrix[5] + z * matrix[6] + matrix[7];
    vector[2] = x * matrix[8] + y * matrix[9] + z * matrix[10] + matrix[11];
}

void add_multiplied_vector(GLdouble vector[3], GLdouble factor,
                           GLdouble term[3]) {
    vector[0] += factor * term[0];
    vector[1] += factor * term[1];
    vector[2] += factor * term[2];
}

void multiply_matrix(GLdouble matrix[16], GLdouble term[16]) {

    GLdouble a00 = matrix[0], a01 = matrix[1], a02 = matrix[2], a03 = matrix[3];
    GLdouble a10 = matrix[4], a11 = matrix[5], a12 = matrix[6], a13 = matrix[7];
    GLdouble a20 = matrix[8], a21 = matrix[9], a22 = matrix[10], a23 = matrix[11];
    GLdouble a30 = matrix[12], a31 = matrix[13], a32 = matrix[14], a33 = matrix[15];

    matrix[0] = a00 * term[0] + a01 * term[4] + a02 * term[8] + a03 * term[12];
    matrix[1] = a00 * term[1] + a01 * term[5] + a02 * term[9] + a03 * term[13];
    matrix[2] = a00 * term[2] + a01 * term[6] + a02 * term[10] + a03 * term[14];
    matrix[3] = a00 * term[3] + a01 * term[7] + a02 * term[11] + a03 * term[15];

    matrix[4] = a10 * term[0] + a11 * term[4] + a12 * term[8] + a13 * term[12];
    matrix[5] = a10 * term[1] + a11 * term[5] + a12 * term[9] + a13 * term[13];
    matrix[6] = a10 * term[2] + a11 * term[6] + a12 * term[10] + a13 * term[14];
    matrix[7] = a10 * term[3] + a11 * term[7] + a12 * term[11] + a13 * term[15];

    matrix[8] = a20 * term[0] + a21 * term[4] + a22 * term[8] + a23 * term[12];
    matrix[9] = a20 * term[1] + a21 * term[5] + a22 * term[9] + a23 * term[13];
    matrix[10] = a20 * term[2] + a21 * term[6] + a22 * term[10] + a23 * term[14];
    matrix[11] = a20 * term[3] + a21 * term[7] + a22 * term[11] + a23 * term[15];

    matrix[12] = a30 * term[0] + a31 * term[4] + a32 * term[8] + a33 * term[12];
    matrix[13] = a30 * term[1] + a31 * term[5] + a32 * term[9] + a33 * term[13];
    matrix[14] = a30 * term[2] + a31 * term[6] + a32 * term[10] + a33 * term[14];
    matrix[15] = a30 * term[3] + a31 * term[7] + a32 * term[11] + a33 * term[15];
}

/**
 * @param color AARRGGBB
 */
void _glClearColorui(GLuint color) {
    glClearColor((GLfloat) ((color >> 16) & 0xFF) / 0xFF,
                 (GLfloat) ((color >> 8) & 0xFF) / 0xFF,
                 (GLfloat) (color & 0xFF) / 0xFF,
                 (GLfloat) ((color >> 24) & 0xFF) / 0xFF);
}

/**
 * @param color AARRGGBB
 */
void _glColorui(GLuint color) {
    glColor4ub((GLubyte) (color >> 16),
               (GLubyte) (color >> 8),
               (GLubyte) color,
               (GLubyte) (color >> 24));
}

GLdouble _glutGetWindowAspect() {
    return (GLdouble) glutGet(GLUT_WINDOW_WIDTH) / glutGet(GLUT_WINDOW_HEIGHT);
}

void check_gl_error() {
    GLenum error = glGetError();
    if (error) {
        fprintf(stderr, "draw: %d\n", error);
        exit(1);
    }
}

GLuint load_texture(const char *path) {
    texture_t texture;
    if (!load_tga(path, &texture)) {
        exit(1);
    }
    GLuint name;
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width, texture.height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, texture.data);
    free(texture.data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return name;
}

void initialize_body_texture(body_t *body) {

    body->texture_name = load_texture(body->texture_path);

    for (body_t **iter = body->planets; *iter; ++iter) {
        initialize_body_texture(*iter);
    }
}

void initialize_textures() {
    TEXTURE_NAME_MILKY_WAY = load_texture(TEXTURE_PATH_MILKY_WAY);
    initialize_body_texture(&BODY_SUN);
}

void initialize_milky_way_display_list() {

    DISPLAY_LIST_MILKY_WAY = glGenLists(1);
    glNewList(DISPLAY_LIST_MILKY_WAY, GL_COMPILE);

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TEXTURE_NAME_MILKY_WAY);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(1, 0);
    glVertex2f(1, 0);
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    glTexCoord2f(0, 1);
    glVertex2f(0, 1);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib();

    glEndList();
}

void initialize_body_display_list(body_t *body, GLUquadricObj *quadric) {

    body->display_list = glGenLists(1);
    glNewList(body->display_list, GL_COMPILE);

    gluQuadricTexture(quadric, GLU_TRUE);
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, body->texture_name);
    gluSphere(quadric, transform_body_radius(body->radius),
              SPHERE_SUBDIVISION_COUNT, SPHERE_SUBDIVISION_COUNT);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib();

    glEndList();

    for (body_t **iter = body->planets; *iter; ++iter) {

        body_t *planet = *iter;

        planet->orbit.display_list = glGenLists(1);
        glNewList(planet->orbit.display_list, GL_COMPILE);

        gluQuadricTexture(quadric, GLU_FALSE);
        glPushAttrib(GL_CURRENT_BIT);
        _glColorui(ORBIT_COLOR);
        glutSolidTorus(ORBIT_INNER_RADIUS,
                       transform_orbit_radius(planet->orbit.radius),
                       TORUS_SIDE_DIVISION_COUNT, TORUS_RADIAL_DIVISTION_COUNT);
        glPopAttrib();

        glEndList();

        initialize_body_display_list(*iter, quadric);
    }
}

void initialize_solar_system_display_lists() {
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    initialize_body_display_list(&BODY_SUN, quadric);
    gluDeleteQuadric(quadric);
}

void initialize_display_lists() {
    initialize_milky_way_display_list();
    initialize_solar_system_display_lists();
}

void initialize() {
    initialize_textures();
    initialize_display_lists();
}

void draw_milky_way() {

    glPushAttrib(GL_TRANSFORM_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glCallList(DISPLAY_LIST_MILKY_WAY);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void draw_body(body_t *body) {

    glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if (body != &BODY_SUN) {
        glEnable(GL_LIGHTING);
    }

    double_t axis[3] = { 0, 1, 0 };
    multiply_vector_by_matrix(axis, body->z_rotation_inverse);
    glRotated(body->tilt, axis[0], axis[1], axis[2]);
    glRotated(get_rotation(body->period), 0, 0, 1);
    glCallList(body->display_list);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    for (body_t **iter = body->planets; *iter; ++iter) {

        body_t *planet = *iter;

        glPushAttrib(GL_TRANSFORM_BIT);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        glRotated(planet->orbit.inclination, 0, -1, 0);

        glCallList(planet->orbit.display_list);

        GLdouble rotation = get_rotation(planet->orbit.period);
        glRotated(rotation, 0, 0, 1);
        make_rotation_matrix(rotation, (GLdouble []) { 0, 0, -1 },
                             planet->z_rotation_inverse);
        multiply_matrix(planet->z_rotation_inverse, body->z_rotation_inverse);
        glTranslated(transform_orbit_radius(planet->orbit.radius), 0, 0);
        draw_body(planet);

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glPopAttrib();
    }
}

void draw_solar_system() {

    update_tick();

    glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Fovy:
    // https://www.opengl.org/discussion_boards/archive/index.php/t-186151.html
    gluPerspective(45, _glutGetWindowAspect(), 1, 200);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    GLdouble center[3];
    assign_vector(center, g_eye);
    add_multiplied_vector(center, LOOK_DISTANCE, g_look);
    gluLookAt(g_eye[0], g_eye[1], g_eye[2], center[0], center[1], center[2],
              g_up[0], g_up[1], g_up[2]);

    glEnable(GL_DEPTH_TEST);

    glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat []) { 0, 0, 0, 1 });
    draw_body(&BODY_SUN);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}

void draw() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_milky_way();
    draw_solar_system();

    glutSwapBuffers();
}

void display() {
    draw();
    check_gl_error();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
}

void keyboard(unsigned char key, int x, int y) {
    GLdouble direction_factor = 1;
    switch (key) {
        case '1':
            assign_vector(g_eye, (GLdouble[3]) { 0, 0, 64 });
            assign_vector(g_look, (GLdouble[3]) { 0, 0, -1 });
            assign_vector(g_up, (GLdouble[3]) { 0, 1, 0 });
            break;
        case '2':
            assign_vector(g_eye, (GLdouble[3]) { 0, 0, 24 });
            assign_vector(g_look, (GLdouble[3]) { 0, 0, -1 });
            assign_vector(g_up, (GLdouble[3]) { 0, 1, 0 });
            break;
        case '3':
            assign_vector(g_eye, (GLdouble[3]) { 0, -64, 16 });
            assign_vector_normalized(g_look, (GLdouble[3]) { 0, 64, -16 });
            assign_vector_normalized(g_up, (GLdouble[3]) { 0, 16, 64 });
            break;
        case '4':
            assign_vector(g_eye, (GLdouble[3]) { 0, -24, 3 });
            assign_vector_normalized(g_look, (GLdouble[3]) { 0, 24, -3 });
            assign_vector_normalized(g_up, (GLdouble[3]) { 0, 3, 24 });
            break;
        case '5':
            assign_vector(g_eye, (GLdouble[3]) { 0, -64, 0 });
            assign_vector_normalized(g_look, (GLdouble[3]) { 0, 64, 0 });
            assign_vector_normalized(g_up, (GLdouble[3]) { 0, 0, 64 });
            break;
        case '6':
            assign_vector(g_eye, (GLdouble[3]) { 0, -24, 0 });
            assign_vector_normalized(g_look, (GLdouble[3]) { 0, 24, 0 });
            assign_vector_normalized(g_up, (GLdouble[3]) { 0, 0, 24 });
            break;
        case '7':
            assign_vector(g_eye, (GLdouble[3]) { -64, 0, 16 });
            assign_vector_normalized(g_look, (GLdouble[3]) {64, 0, -16 });
            assign_vector_normalized(g_up, (GLdouble[3]) {16, 0, 64 });
            break;
        case '8':
            assign_vector(g_eye, (GLdouble[3]) { -24, 0, 3 });
            assign_vector_normalized(g_look, (GLdouble[3]) {24, 0, -3 });
            assign_vector_normalized(g_up, (GLdouble[3]) {3, 0, 24 });
            break;
        case '-':
            direction_factor = -1;
            // Fall through!
        case '+':
        case '=':
            g_body_rotation_phase += direction_factor
                                     * BODY_ROTATION_PHASE_FACTOR;
            break;
        case '[':
            direction_factor = -1;
            // Fall through!
        case ']':
            g_body_rotation_speed += direction_factor
                                            * BODY_ROTATION_SPEED_FACTOR;
            break;
        case 'a':
        case 'A':
            direction_factor = -1;
            // Fall through!
        case 'd':
        case 'D': {
            GLdouble direction[3];
            assign_vector(direction, g_look);
            cross_vector(direction, g_up);
            add_multiplied_vector(g_eye, direction_factor * MOVE_FACTOR,
                                  direction);
            break;
        }
        case 's':
        case 'S':
            direction_factor = -1;
            // Fall through!
        case 'w':
        case 'W':
            add_multiplied_vector(g_eye, direction_factor * MOVE_FACTOR,
                                  g_look);
            break;
        case 'e':
        case 'E':
            direction_factor = -1;
            // Fall through!
        case 'q':
        case 'Q':
            add_multiplied_vector(g_eye, direction_factor * MOVE_FACTOR, g_up);
            break;
        case 'p':
            fprintf(stderr,
                    "g_eye = (%lf, %lf, %lf), g_look = (%lf, %lf, %lf), g_up = (%lf, %lf, %lf)\n",
                    g_eye[0], g_eye[1], g_eye[2], g_look[0], g_look[1],
                    g_look[2], g_up[0], g_up[1], g_up[2]);
            break;
        default:
            // Do nothing.
            break;
    }
}

int g_last_x = -1;
int g_last_y = -1;

void passive_motion(int x, int y) {
    g_last_x = x;
    g_last_y = y;
}

void motion(int x, int y) {

    if (g_last_x >= 0 && g_last_y >= 0) {

        // MAGIC: 16 is something that should be derived from fovy which is 45.
        GLdouble radius = 16 * LOOK_DISTANCE;

        int delta_x = x - g_last_x;
        GLdouble angle_x = (GLdouble) delta_x / radius / M_PI * 180;
        // Rotate camera right by angle_x.
        GLdouble rotation_x[16];
        make_rotation_matrix(angle_x, g_up, rotation_x);
        multiply_vector_by_matrix(g_look, rotation_x);

        int delta_y = y - g_last_y;
        GLdouble angle_y = (GLdouble) delta_y / radius / M_PI * 180;
        // Rotate camera up by angle_y.
        GLdouble axis_y[3];
        assign_vector(axis_y, g_look);
        cross_vector(axis_y, g_up);
        GLdouble rotation_y[16];
        make_rotation_matrix(angle_y, axis_y, rotation_y);
        multiply_vector_by_matrix(g_look, rotation_y);
        multiply_vector_by_matrix(g_up, rotation_y);
    }

    g_last_x = x;
    g_last_y = y;
}

void mouse(int button, int state, int x, int y) {
    switch (button) {
        case 3:
        case 4:
            if (state == GLUT_UP) {
                return;
            }
            GLdouble direction_factor = button == 3 ? 1 : -1;
            add_multiplied_vector(g_eye, direction_factor * ZOOM_FACTOR,
                                  g_look);
            break;
        default:
            // Do nothing.
            break;
    }
}

int main(int argc, char **argv) {

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH
                        | GLUT_MULTISAMPLE);
    glutInitWindowSize(1200, 640);
    glutCreateWindow("Solar System");

    _glClearColorui(0xFFFFFFFF);
    glClearDepth(1);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,
                 (GLfloat []) { 0.5, 0.5, 0.5, 1 });
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                 (GLfloat []) { 0.8, 0.8, 0.8, 1 });
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 7);
    glEnable(GL_LIGHT0);

    initialize();

    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(passive_motion);
    glutMotionFunc(motion);
    glutMouseFunc(mouse);
    glutDisplayFunc(display);
    glutIdleFunc(display);

    glutMainLoop();

    return EXIT_SUCCESS;
}
