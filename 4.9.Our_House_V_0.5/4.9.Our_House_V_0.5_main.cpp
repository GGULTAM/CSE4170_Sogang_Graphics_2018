#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "Shaders/LoadShaders.h"
#include "My_Shading.h"
GLuint h_ShaderProgram, h_ShaderProgram_simple, h_ShaderProgram_PS, h_ShaderProgram_GS, h_ShaderProgram_SCR; // handle to shader program

//COMMON
GLint loc_ModelViewProjectionMatrix, loc_ModelViewMatrix, loc_ModelViewMatrixInvTrans;
int nowShader = 1; // 0 - simple, 1 - PS, 2 - GS

//SIMPLE
GLint loc_ModelViewProjectionMatrix_simple, loc_primitive_color; // indices of uniform variables

//PONG, GOURAUD
#define NUMBER_OF_LIGHT_SUPPORTED 4 
Light_Parameters light[NUMBER_OF_LIGHT_SUPPORTED];
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED];
GLint loc_global_ambient_color;
loc_Material_Parameters loc_material;

//PONG
GLint loc_ModelViewProjectionMatrix_PS, loc_ModelViewMatrix_PS, loc_ModelViewMatrixInvTrans_PS;
GLint loc_blind_level, loc_effect_enabled, loc_hall_enabled;
bool effect_enabled = false;
int effect_time = 0, effect_added = 1;

//GOURAUD
GLint loc_ModelViewProjectionMatrix_GS, loc_ModelViewMatrix_GS, loc_ModelViewMatrixInvTrans_GS;

//Screen Shader
GLint loc_ModelViewProjectionMatrix_SCR, loc_ModelViewMatrix_SCR, loc_ModelViewMatrixInvTrans_SCR;
GLint loc_scrLevel, loc_scrPos, loc_scrXY;
loc_Material_Parameters loc_scr_material;
loc_light_Parameters loc_scr_light;

// include glm/*.hpp only if necessary
//#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>//translate, rotate, scale, lookAt, perspective, etc.
#include <glm/gtc/matrix_inverse.hpp>
glm::mat3 ModelViewMatrixInvTrans;
glm::mat4 ModelViewProjectionMatrix;
glm::mat4 ModelViewMatrix, ViewMatrix, ProjectionMatrix;
glm::mat4 Main_ViewMatrix;
glm::mat4 CameraViewMatrix[5];
glm::mat4 Map_ViewMatrix[3];
glm::mat4 Map_ProjectionMatrix[3];
int cam_idx = 0;

typedef struct camera {
	glm::vec3 pos, cen, up, v, u, n;
} camera;
camera eye, CCTV[4], map[3];
float zoom_scale[5] = { 2.0f, 5.0f, 5.0f, 5.0f, 5.0f };
float CCTV_angle = 0.0f;
#define NEAR_DISTANCE 1.0f
#define FAR_DISTANCE 1000.0f

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define MOUSE_SENSITIVITY 5.0f

#include "Object_Definitions.h"

void print_vec3(glm::vec3 v) {
	printf("%lf %lf %lf\n", v.x, v.y, v.z);
}
void print_camera(camera cam) {
	print_vec3(cam.pos);
	print_vec3(cam.cen);
	print_vec3(cam.up);
	print_vec3(cam.v);
	print_vec3(cam.u);
	print_vec3(cam.n);
	printf("---------------------\n");
}

float f_mod(float a, float b) {
	while (a >= b) {
		a -= b;
	}
	return a;
}

glm::mat4 look_to_camera(camera cam) {
	glm::mat4 TmpMatrix;
	glm::mat3 TmpXY;
	TmpMatrix = glm::lookAt(cam.pos, cam.cen, cam.v);
	TmpXY = glm::mat3(glm::mat3(TmpMatrix)*glm::vec3(1.0f, 0.0f, 0.0f), glm::mat3(TmpMatrix)*glm::vec3(0.0f, 0.0f, -1.0f), glm::mat3(TmpMatrix)*glm::vec3(0.0f, 1.0f, 0.0f));
	TmpXY = glm::inverse(TmpXY);
	glUniform3fv(loc_scrPos, 1, &glm::vec3(TmpMatrix*glm::vec4(100.0f, 100.0f, 20.0f, 1.0f))[0]);
	glUniformMatrix3fv(loc_scrXY, 1, GL_FALSE, &TmpXY[0][0]);
	//print_vec3(glm::vec3(TmpMatrix*glm::vec4(100.0f, 100.0f, 20.0f, 1.0f)));
	//print_vec3(glm::mat3(TmpMatrix)*glm::vec3(1.0f, 0.0f, 0.0f));
	//print_vec3(glm::mat3(TmpMatrix)*glm::vec3(0.0f, 0.0f, -1.0f));
	return TmpMatrix;
}

float vec_size(glm::vec3 v) {
	return glm::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

void calc_axis(camera &cam) {
	cam.n = cam.pos - cam.cen;
	cam.u = glm::cross(cam.up, cam.n);
	cam.v = glm::cross(cam.n, cam.u);
	cam.n = glm::normalize(cam.n);
	cam.v = glm::normalize(cam.v);
	cam.u = glm::normalize(cam.u);
	cam.n *= 2;
	cam.v *= 2;
	cam.u *= 2;
}

void calc_cen(camera &cam) {
	glm::vec3 bak;
	bak = cam.n;
	cam.n = glm::normalize(cam.n);
	cam.cen = cam.pos - cam.n;
	cam.up = cam.v;
	cam.n = bak;
}

void prepare_shader_program(void);
void initialize_lights_and_material(void);
void set_up_scene_lights(void);
void setShader() {
	prepare_shader_program();
	if (nowShader != 0) {
		initialize_lights_and_material();
		set_up_scene_lights();
	}
	else
		glUseProgram(h_ShaderProgram);
}

void changeShader(int n) {
	nowShader = n;
	setShader();
}

void tmp_use_simple(){
	loc_ModelViewProjectionMatrix = loc_ModelViewProjectionMatrix_simple;
	h_ShaderProgram = h_ShaderProgram_simple;
	glUseProgram(h_ShaderProgram);
}

void back_to_prev_shader() {
	if (nowShader == 1) {
		loc_ModelViewProjectionMatrix = loc_ModelViewProjectionMatrix_PS;
		h_ShaderProgram = h_ShaderProgram_PS;
		glUseProgram(h_ShaderProgram);
	}
	else if (nowShader == 2) {
		loc_ModelViewProjectionMatrix = loc_ModelViewProjectionMatrix_GS;
		h_ShaderProgram = h_ShaderProgram_GS;
		glUseProgram(h_ShaderProgram);
	}
}

void display_render(void) {
	tmp_use_simple();
	glLineWidth(2.0f);
	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(WC_AXIS_LENGTH, WC_AXIS_LENGTH, WC_AXIS_LENGTH));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_axes();

	glLineWidth(1.0f);
	back_to_prev_shader();

	draw_static_object(&(static_objects[OBJ_BUILDING]), 0);

	draw_static_object(&(static_objects[OBJ_TABLE]), 0);
	draw_static_object(&(static_objects[OBJ_TABLE]), 1);

	draw_static_object(&(static_objects[OBJ_LIGHT]), 0);
	draw_static_object(&(static_objects[OBJ_LIGHT]), 1);
	draw_static_object(&(static_objects[OBJ_LIGHT]), 2);
	draw_static_object(&(static_objects[OBJ_LIGHT]), 3);
	draw_static_object(&(static_objects[OBJ_LIGHT]), 4);

	draw_static_object(&(static_objects[OBJ_TEAPOT]), 0);
	draw_static_object(&(static_objects[OBJ_NEW_CHAIR]), 0);
	draw_static_object(&(static_objects[OBJ_FRAME]), 0);
	draw_static_object(&(static_objects[OBJ_NEW_PICTURE]), 0);
	draw_static_object(&(static_objects[OBJ_COW]), 0);

	//render_car();
	draw_animated_tiger();

	tmp_use_simple();
	//CAMERA AXIS
	eye.v *= WC_CAM_LENGTH; eye.u *= WC_CAM_LENGTH; eye.n *= WC_CAM_LENGTH;
	vertices_cam[1][0] = eye.v.x; vertices_cam[1][1] = eye.v.y; vertices_cam[1][2] = eye.v.z;
	vertices_cam[3][0] = eye.u.x; vertices_cam[3][1] = eye.u.y; vertices_cam[3][2] = eye.u.z;
	vertices_cam[5][0] = eye.n.x; vertices_cam[5][1] = eye.n.y; vertices_cam[5][2] = eye.n.z;
	eye.v /= WC_CAM_LENGTH; eye.u /= WC_CAM_LENGTH; eye.n /= WC_CAM_LENGTH;

	float FOVY = 15.0f*zoom_scale[0] * TO_RADIAN / 2;
	glm::vec3 fov_vec;
	fov_vec = glm::vec3(glm::vec4(-eye.n, 1) * glm::rotate(glm::mat4(1.0f), FOVY, eye.u));
	fov_vec = glm::vec3(glm::vec4(fov_vec, 1) * glm::rotate(glm::mat4(1.0f), glm::atan(glm::tan(FOVY)*1.5f), eye.v));
	fov_vec = (FAR_DISTANCE + 0.1f) / (glm::dot(fov_vec, -eye.n) / 2) * fov_vec; // divide by 2 -> size of eye.n
	vertices_fov[1][0] = fov_vec.x; vertices_fov[1][1] = fov_vec.y; vertices_fov[1][2] = fov_vec.z;
	viewing_volume[0][0] = fov_vec.x; viewing_volume[0][1] = fov_vec.y; viewing_volume[0][2] = fov_vec.z;

	fov_vec = fov_vec - glm::dot(fov_vec, eye.v)/2 * eye.v;
	vertices_fov[3][0] = fov_vec.x; vertices_fov[3][1] = fov_vec.y; vertices_fov[3][2] = fov_vec.z;
	viewing_volume[1][0] = fov_vec.x; viewing_volume[1][1] = fov_vec.y; viewing_volume[1][2] = fov_vec.z;

	fov_vec = fov_vec - glm::dot(fov_vec, eye.u)/2 * eye.u;
	vertices_fov[5][0] = fov_vec.x; vertices_fov[5][1] = fov_vec.y; vertices_fov[5][2] = fov_vec.z;
	viewing_volume[2][0] = fov_vec.x; viewing_volume[2][1] = fov_vec.y; viewing_volume[2][2] = fov_vec.z;

	fov_vec = fov_vec - glm::dot(fov_vec, eye.v)/2 * eye.v;
	vertices_fov[7][0] = fov_vec.x; vertices_fov[7][1] = fov_vec.y; vertices_fov[7][2] = fov_vec.z;
	viewing_volume[3][0] = fov_vec.x; viewing_volume[3][1] = fov_vec.y; viewing_volume[3][2] = fov_vec.z;

	viewing_volume[4][0] = viewing_volume[0][0]; viewing_volume[4][1] = viewing_volume[0][1]; viewing_volume[4][2] = viewing_volume[0][2];

	for (int i = 5; i <= 9; i++) {
		for (int j = 0; j<3; j++)
			viewing_volume[i][j] = viewing_volume[i - 5][j] * (NEAR_DISTANCE - 0.1f) / FAR_DISTANCE;
	}

	define_cam();
	draw_cam();
	//CAMERA AXIS END
	//draw_floor();
	glUseProgram(h_ShaderProgram_SCR);
	set_material_floor();
	draw_floor();

	back_to_prev_shader();

}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CameraViewMatrix[0] = Main_ViewMatrix;

	// Main Frame
	ViewMatrix = CameraViewMatrix[cam_idx];
	glViewport(400, 0, 1200, 800);
	ProjectionMatrix = glm::perspective(15.0f*zoom_scale[cam_idx] * TO_RADIAN, cam_idx==0?1.5f:2.0f, NEAR_DISTANCE, FAR_DISTANCE);
	display_render();

	glUniform1i(loc_light[0].light_on, 0);
	glUniform1i(loc_light[1].light_on, 0);
	// Map
	ViewMatrix = Map_ViewMatrix[0];
	glViewport(0, 500, 400, 295);
	ProjectionMatrix = Map_ProjectionMatrix[0];
	display_render();

	ViewMatrix = Map_ViewMatrix[1];
	glViewport(0, 400, 400, 70);
	ProjectionMatrix = Map_ProjectionMatrix[1];
	display_render();

	ViewMatrix = Map_ViewMatrix[2];
	glViewport(0, 250, 400, 118);
	ProjectionMatrix = Map_ProjectionMatrix[2];
	display_render();
	// end Map
	glUniform1i(loc_light[0].light_on, light[0].light_on);
	glUniform1i(loc_light[1].light_on, light[1].light_on);
	
	glutSwapBuffers();
}

void specialkey(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_CTRL_L:
		eye.pos -= eye.v; eye.cen -= eye.v;
		Main_ViewMatrix = look_to_camera(eye);
		glutPostRedisplay();
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	static int flag_cull_face = 0, polygon_fill_on = 1, depth_test_on = 0;
	static float blind_level = 0.0f;
	static bool hall_enabled = false;
	switch (key) {
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	case 'b':
		blind_level += 1.0f;
		if (blind_level >= 5.0f) blind_level = 0.0f;
		glUniform1f(loc_blind_level, blind_level);
		printf("Blind : %.0lf\n", blind_level);
		glutPostRedisplay();
		break;
	case 'n':
		effect_enabled = 1 - effect_enabled;
		printf("Effect enabled : %s\n", effect_enabled ? "true" : "false");
		if (effect_enabled)
			effect_time = 10;
		else
			glUniform1i(loc_effect_enabled, 100);
		glutPostRedisplay();
		break;
	case 'm':
		hall_enabled = 1 - hall_enabled;
		printf("Hall enabled : %s\n", hall_enabled ? "true" : "false");
		glUniform1i(loc_hall_enabled, hall_enabled);
		glutPostRedisplay();
		break;
	case 'g':
		cam_idx++;
		cam_idx %= 5;
		glutPostRedisplay();
		/*
		printf("CAM\n");
		printf("%lf, %lf, %lf\n", eye.pos.x, eye.pos.y, eye.pos.z);
		printf("%lf, %lf, %lf\n", eye.cen.x, eye.cen.y, eye.cen.z);
		printf("%lf, %lf, %lf\n", eye.up.x, eye.up.y, eye.up.z);
		*/
		break;
	case 'z':
		if (cam_idx != 0) break;
		eye.pos = glm::vec3(500.0f, 90.0f, 25.0f);
		eye.cen = glm::vec3(0.0f, 90.0f, 25.0f);
		eye.up = glm::vec3(0.0f, 0.0f, 1.0f);
		calc_axis(eye);
		zoom_scale[0] = 1.0f;
		Main_ViewMatrix = look_to_camera(eye);
		glutPostRedisplay();
		break;
	case 'c':
		flag_cull_face = (flag_cull_face + 1) % 3;
		switch (flag_cull_face) {
		case 0:
			glDisable(GL_CULL_FACE);
			glutPostRedisplay();
			fprintf(stdout, "^^^ No faces are culled.\n");
			break;
		case 1: // cull back faces;
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			fprintf(stdout, "^^^ Back faces are culled.\n");
			break;
		case 2: // cull front faces;
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			fprintf(stdout, "^^^ Front faces are culled.\n");
			break;
		}
		break;
	case 'f':
		polygon_fill_on = 1 - polygon_fill_on;
		if (polygon_fill_on) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			fprintf(stdout, "^^^ Polygon filling enabled.\n");
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			fprintf(stdout, "^^^ Line drawing enabled.\n");
		}
		glutPostRedisplay();
		break;
	/*
	case 'd':
		depth_test_on = 1 - depth_test_on;
		if (depth_test_on) {
			glEnable(GL_DEPTH_TEST);
			fprintf(stdout, "^^^ Depth test enabled.\n");
		}
		else {
			glDisable(GL_DEPTH_TEST);
			fprintf(stdout, "^^^ Depth test disabled.\n");
		}
		glutPostRedisplay();
		break;
	*/
	case 'w':
		eye.pos -= eye.n; eye.cen -= eye.n;
		Main_ViewMatrix = look_to_camera(eye);
		glutPostRedisplay();
		break;
	case 's':
		eye.pos += eye.n; eye.cen += eye.n;
		Main_ViewMatrix = look_to_camera(eye);
		glutPostRedisplay();
		break;
	case 'd':
		eye.pos += eye.u; eye.cen += eye.u;
		Main_ViewMatrix = look_to_camera(eye);
		glutPostRedisplay();
		break;
	case 'a':
		eye.pos -= eye.u; eye.cen -= eye.u;
		Main_ViewMatrix = look_to_camera(eye);
		glutPostRedisplay();
		break;
	case ' ':
		eye.pos += eye.v; eye.cen += eye.v;
		Main_ViewMatrix = look_to_camera(eye);
		glutPostRedisplay();
		break;
	case 'q':
		if (cam_idx == 4) {
			CCTV[3].u = glm::vec3((glm::vec4(CCTV[3].u, 1) * glm::rotate(glm::mat4(1.0f), -0.02f, glm::vec3(0, 0, 1))));
			CCTV[3].n = glm::vec3((glm::vec4(CCTV[3].n, 1) * glm::rotate(glm::mat4(1.0f), -0.02f, glm::vec3(0, 0, 1))));
			CCTV[3].v = glm::vec3((glm::vec4(CCTV[3].v, 1) * glm::rotate(glm::mat4(1.0f), -0.02f, glm::vec3(0, 0, 1))));
			calc_cen(CCTV[3]);
			CameraViewMatrix[4] = look_to_camera(CCTV[3]);
		}
		else {
			eye.u = glm::vec3((glm::vec4(eye.u, 1) * glm::rotate(glm::mat4(1.0f), 0.02f, -eye.n)));
			eye.v = glm::vec3((glm::vec4(eye.v, 1) * glm::rotate(glm::mat4(1.0f), 0.02f, -eye.n)));
			calc_cen(eye);
			Main_ViewMatrix = look_to_camera(eye);
		}
		glutPostRedisplay();
		break;
	case 'e':
		if (cam_idx == 4) {
			CCTV[3].u = glm::vec3((glm::vec4(CCTV[3].u, 1) * glm::rotate(glm::mat4(1.0f), 0.02f, glm::vec3(0, 0, 1))));
			CCTV[3].n = glm::vec3((glm::vec4(CCTV[3].n, 1) * glm::rotate(glm::mat4(1.0f), 0.02f, glm::vec3(0, 0, 1))));
			CCTV[3].v = glm::vec3((glm::vec4(CCTV[3].v, 1) * glm::rotate(glm::mat4(1.0f), 0.02f, glm::vec3(0, 0, 1))));
			calc_cen(CCTV[3]);
			CameraViewMatrix[4] = look_to_camera(CCTV[3]);
		}
		else {
			eye.u = glm::vec3((glm::vec4(eye.u, 1) * glm::rotate(glm::mat4(1.0f), -0.02f, -eye.n)));
			eye.v = glm::vec3((glm::vec4(eye.v, 1) * glm::rotate(glm::mat4(1.0f), -0.02f, -eye.n)));
			calc_cen(eye);
			Main_ViewMatrix = look_to_camera(eye);
		}
		glutPostRedisplay();
		break;
	}
	
}

void reshape(int width, int height) {
	glutReshapeWindow(1600, 800);
}


float TigerPath[52][2] = { { 25.0f, 50.0f },{ 45.0f, 50.0f },{ 45.0f, 25.0f },{ 65.0f, 25.0f },{ 65.0f, 70.0f },{ 25.0f, 70.0f },{ 25.0f, 145.0f },{ 55.0f, 145.0f },{ 55.0f, 125.0f },{ 25.0f, 125.0f },{ 25.0f, 105.0f },{ 55.0f, 105.0f },{ 55.0f, 90.0f },{ 80.0f, 90.0f },{ 80.0f, 110.0f },{ 155.0f, 110.0f },{ 155.0f, 145.0f },{ 140.0f, 145.0f },{ 140.0f, 135.0f },{ 75.0f, 135.0f },{ 75.0f, 145.0f },{ 170.0f, 145.0f },{ 170.0f, 100.0f },{ 190.0f, 100.0f },{ 190.0f, 120.0f },{ 215.0f, 120.0f },{ 215.0f, 140.0f },{ 195.0f, 140.0f },{ 195.0f, 145.0f },{ 215.0f, 145.0f },{ 215.0f, 50.0f },{ 200.0f, 50.0f },{ 200.0f, 30.0f },{ 215.0f, 30.0f },{ 215.0f, 25.0f },{ 180.0f, 25.0f },{ 180.0f, 75.0f },{ 200.0f, 75.0f },{ 200.0f, 95.0f },{ 130.0f, 95.0f },{ 130.0f, 75.0f },{ 155.0f, 75.0f },{ 155.0f, 65.0f },{ 115.0f, 65.0f },{ 115.0f, 25.0f },{ 110.0f, 25.0f },{ 110.0f, 65.0f },{ 120.0f, 65.0f },{ 120.0f, 85.0f },{ 90.0f, 85.0f },{ 90.0f, 25.0f },{ 25.0f, 25.0f } };
int TigerPathIdx = 0;
bool TigerPathStop = false;

int angle = 180, prev_angle;
void timer_scene(int timestamp_scene) {
	int goal_angle;
	glm::vec3 nowPath;
	nowPath = glm::vec3(TigerPath[TigerPathIdx][0] - tiger_data.pos[0], TigerPath[TigerPathIdx][1] - tiger_data.pos[1], 0);
	//printf("%lf %lf\n", nowPath.x, nowPath.y);
	if (TigerPathStop == true) {
		if (abs(nowPath.x) > 0.01f) {
			if (nowPath.x > 0) {
				goal_angle = 90;
			}
			else
				goal_angle = 270;
			}
		else {
			if (nowPath.y > 0)
				goal_angle = 180;
			else
				goal_angle = 0;
		}
		if (prev_angle == 0 && goal_angle == 270)
			angle -= 1;
		else if (prev_angle == 270 && goal_angle == 0)
			angle += 1;
		else
			angle += (((goal_angle - angle)>0) ? 1 : -1);

		//printf("%d %d %d\n", angle, goal_angle, prev_angle);
		if (angle < 0) angle = 360+angle;
		if (angle >= 360) angle = angle-360;
		if (angle == goal_angle) TigerPathStop = false;
	}
	else {
		if (abs(nowPath.x) > 0.01f) {
			if (nowPath.x > 0) {
				tiger_data.pos[0] += 0.2f;
			}
			else if (nowPath.x < 0) {
				tiger_data.pos[0] -= 0.2f;
			}
		}
		else {
			if (nowPath.y > 0)
				tiger_data.pos[1] += 0.2f;
			else
				tiger_data.pos[1] -= 0.2f;
		}
		//printf("%lf %lf %d\n", tiger_data.pos[0], tiger_data.pos[1], angle);
		if ((abs(tiger_data.pos[0] - TigerPath[TigerPathIdx][0]) < 0.01f) && (abs(tiger_data.pos[1] - TigerPath[TigerPathIdx][1]) < 0.01f)) {
			TigerPathIdx++;
			TigerPathStop = true;
		}
		prev_angle = angle;
	}
	TigerPathIdx %= 52;
	tiger_data.cur_frame = timestamp_scene / 5 % N_TIGER_FRAMES;
	tiger_data.rotation_angle = (float)angle;

	//Shader Effect
	if (effect_enabled) {
		if (effect_time == 10) effect_added = -1;
		if (effect_time ==  0) effect_added = +1;
		effect_time += effect_added;
		glUniform1i(loc_effect_enabled, effect_time);
	}

	glutPostRedisplay();
	glutTimerFunc(20, timer_scene, (timestamp_scene + 1) % INT_MAX);
}

void mouse_wheel(int button, int dir, int x, int y) {
	if ((1 <= cam_idx) && (cam_idx <= 3)) return;
	if (dir > 0) {
		if (zoom_scale[cam_idx] > 0.1f) zoom_scale[cam_idx] /= 1.2f;
	}
	else {
		if (zoom_scale[cam_idx] < 5.0f) zoom_scale[cam_idx] *= 1.2f;
	}
	reshape(1200, 800);
}

void mouse_move(int x, int y) {
	float sens = MOUSE_SENSITIVITY * 0.0001f;
	x -= 600; y -= 400;
	eye.u = glm::vec3((glm::vec4(eye.u, 1) * glm::rotate(glm::mat4(1.0f), x*sens, eye.v)));
	eye.n = glm::vec3((glm::vec4(eye.n, 1) * glm::rotate(glm::mat4(1.0f), x*sens, eye.v)));
	eye.n = glm::vec3((glm::vec4(eye.n, 1) * glm::rotate(glm::mat4(1.0f), y*sens, eye.u)));
	eye.v = glm::vec3((glm::vec4(eye.v, 1) * glm::rotate(glm::mat4(1.0f), y*sens, eye.u)));
	
	calc_cen(eye);
	Main_ViewMatrix = look_to_camera(eye);
	glutWarpPointer(600, 400);
}

void prepare_shader_program(void);
void initialize_lights_and_material(void);
void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_RIGHT_BUTTON)){
		if (state == (GLUT_DOWN) && glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
			nowShader = 2;
			fprintf(stdout, "GS\n");
		}
		else {
			nowShader = 1;
			fprintf(stdout, "PS\n");
		}
		setShader();
	}
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialkey);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup_OpenGL_stuffs);
	glutPassiveMotionFunc(mouse_move);	
	glutMouseWheelFunc(mouse_wheel);
	glutMouseFunc(mouse);
}

void prepare_shader_program(void) {
	//fprintf(stdout, "%d prepare\n", nowShader);
	int i;
	char string[256];
	ShaderInfo shader_info_simple[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
	{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
	{ GL_NONE, NULL }
	};
	ShaderInfo shader_info_PS[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Phong.vert" },
	{ GL_FRAGMENT_SHADER, "Shaders/Phong.frag" },
	{ GL_NONE, NULL }
	};
	ShaderInfo shader_info_GS[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Gouraud.vert" },
	{ GL_FRAGMENT_SHADER, "Shaders/Gouraud.frag" },
	{ GL_NONE, NULL }
	};
	ShaderInfo shader_info_SCR[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Screen.vert" },
	{ GL_FRAGMENT_SHADER, "Shaders/Screen.frag" },
	{ GL_NONE, NULL }
	};

	h_ShaderProgram_SCR = LoadShaders(shader_info_SCR);
	loc_ModelViewProjectionMatrix_SCR = glGetUniformLocation(h_ShaderProgram_SCR, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_SCR = glGetUniformLocation(h_ShaderProgram_SCR, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_SCR = glGetUniformLocation(h_ShaderProgram_SCR, "u_ModelViewMatrixInvTrans");
	loc_scr_material.ambient_color = glGetUniformLocation(h_ShaderProgram_SCR, "u_material.ambient_color");
	loc_scr_material.diffuse_color = glGetUniformLocation(h_ShaderProgram_SCR, "u_material.diffuse_color");
	loc_scr_material.specular_color = glGetUniformLocation(h_ShaderProgram_SCR, "u_material.specular_color");
	loc_scr_material.emissive_color = glGetUniformLocation(h_ShaderProgram_SCR, "u_material.emissive_color");
	loc_scr_material.specular_exponent = glGetUniformLocation(h_ShaderProgram_SCR, "u_material.specular_exponent");
	loc_scr_light.light_on = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].light_on");
	loc_scr_light.position = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].position");
	loc_scr_light.ambient_color = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].ambient_color");
	loc_scr_light.diffuse_color = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].diffuse_color");
	loc_scr_light.specular_color = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].specular_color");
	loc_scr_light.spot_direction = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].spot_direction");
	loc_scr_light.spot_exponent = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].spot_exponent");
	loc_scr_light.spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].spot_cutoff_angle");
	loc_scr_light.light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_SCR, "u_light[0].light_attenuation_factors");
	loc_scrLevel = glGetUniformLocation(h_ShaderProgram_SCR, "u_scrLevel");
	loc_scrPos = glGetUniformLocation(h_ShaderProgram_SCR, "u_scrPos");
	loc_scrXY = glGetUniformLocation(h_ShaderProgram_SCR, "u_scrXY");

	h_ShaderProgram_simple = LoadShaders(shader_info_simple);
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");
	loc_ModelViewProjectionMatrix_simple = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");
	if (nowShader == 0) {
		loc_ModelViewProjectionMatrix = loc_ModelViewProjectionMatrix_simple;
		h_ShaderProgram = h_ShaderProgram_simple;
		return;
	}
	h_ShaderProgram_PS = LoadShaders(shader_info_PS);
	loc_ModelViewProjectionMatrix_PS = glGetUniformLocation(h_ShaderProgram_PS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_PS = glGetUniformLocation(h_ShaderProgram_PS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_PS = glGetUniformLocation(h_ShaderProgram_PS, "u_ModelViewMatrixInvTrans");

	h_ShaderProgram_GS = LoadShaders(shader_info_GS);
	loc_ModelViewProjectionMatrix_GS = glGetUniformLocation(h_ShaderProgram_GS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_GS = glGetUniformLocation(h_ShaderProgram_GS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_GS = glGetUniformLocation(h_ShaderProgram_GS, "u_ModelViewMatrixInvTrans");

	if (nowShader == 1) {
		h_ShaderProgram = h_ShaderProgram_PS;
		loc_ModelViewProjectionMatrix = loc_ModelViewProjectionMatrix_PS;
		loc_ModelViewMatrix = loc_ModelViewMatrix_PS;
		loc_ModelViewMatrixInvTrans = loc_ModelViewMatrixInvTrans_PS;
	}
	else if (nowShader == 2){
		h_ShaderProgram = h_ShaderProgram_GS;
		loc_ModelViewProjectionMatrix = loc_ModelViewProjectionMatrix_GS;
		loc_ModelViewMatrix = loc_ModelViewMatrix_GS;
		loc_ModelViewMatrixInvTrans = loc_ModelViewMatrixInvTrans_GS;
	}

	loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram, "u_global_ambient_color");
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		sprintf(string, "u_light[%d].light_on", i);
		loc_light[i].light_on = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].position", i);
		loc_light[i].position = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].ambient_color", i);
		loc_light[i].ambient_color = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].diffuse_color", i);
		loc_light[i].diffuse_color = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].specular_color", i);
		loc_light[i].specular_color = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].spot_direction", i);
		loc_light[i].spot_direction = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].spot_exponent", i);
		loc_light[i].spot_exponent = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].spot_cutoff_angle", i);
		loc_light[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram, string);
		sprintf(string, "u_light[%d].light_attenuation_factors", i);
		loc_light[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram, string);
	}

	loc_material.ambient_color = glGetUniformLocation(h_ShaderProgram, "u_material.ambient_color");
	loc_material.diffuse_color = glGetUniformLocation(h_ShaderProgram, "u_material.diffuse_color");
	loc_material.specular_color = glGetUniformLocation(h_ShaderProgram, "u_material.specular_color");
	loc_material.emissive_color = glGetUniformLocation(h_ShaderProgram, "u_material.emissive_color");
	loc_material.specular_exponent = glGetUniformLocation(h_ShaderProgram, "u_material.specular_exponent");

	if (nowShader == 1) {
		loc_blind_level = glGetUniformLocation(h_ShaderProgram_PS, "u_blind_level");
		loc_effect_enabled = glGetUniformLocation(h_ShaderProgram_PS, "u_effect_enabled");
		loc_hall_enabled = glGetUniformLocation(h_ShaderProgram_PS, "u_hall_enabled");
	}
}

void initialize_lights_and_material(void) {
	int i;

	glUseProgram(h_ShaderProgram);

	glUniform4f(loc_global_ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		glUniform1i(loc_light[i].light_on, 0); // turn off all lights initially
		glUniform4f(loc_light[i].position, 0.0f, 0.0f, 1.0f, 0.0f);
		glUniform4f(loc_light[i].ambient_color, 0.0f, 0.0f, 0.0f, 1.0f);
		if (i == 0) {
			glUniform4f(loc_light[i].diffuse_color, 1.0f, 1.0f, 1.0f, 1.0f);
			glUniform4f(loc_light[i].specular_color, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			glUniform4f(loc_light[i].diffuse_color, 0.0f, 0.0f, 0.0f, 1.0f);
			glUniform4f(loc_light[i].specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		glUniform3f(loc_light[i].spot_direction, 0.0f, 0.0f, -1.0f);
		glUniform1f(loc_light[i].spot_exponent, 0.0f); // [0.0, 128.0]
		glUniform1f(loc_light[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
		glUniform4f(loc_light[i].light_attenuation_factors, 1.0f, 0.0f, 0.0f, 10.0f); // .w != 0.0f for no ligth attenuation
	}

	glUniform4f(loc_material.ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
	glUniform4f(loc_material.diffuse_color, 0.8f, 0.8f, 0.8f, 1.0f);
	glUniform4f(loc_material.specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform4f(loc_material.emissive_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform1f(loc_material.specular_exponent, 0.0f); // [0.0, 128.0]
	if (nowShader == 1){
		glUniform1f(loc_blind_level, 0);
		glUniform1i(loc_effect_enabled, 100);
		glUniform1i(loc_hall_enabled, 0);
	}
	glUniform1i(loc_scrLevel, 10);

	glUseProgram(0);
}

void initialize_OpenGL(void) {
	glEnable(GL_DEPTH_TEST); // Default state
	 
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.12f, 0.18f, 0.12f, 1.0f);

	eye.pos = glm::vec3(60.0f, 25.0f, 5.0f);
	eye.cen = glm::vec3(0.0f, 25.0f, 5.0f);
	eye.up = glm::vec3(0.0f, 0.0f, 1.0f);
	calc_axis(eye);
	Main_ViewMatrix = look_to_camera(eye);

	CCTV[0].pos = glm::vec3(192.841568f, 129.907623f, 49.039806f);
	CCTV[0].cen = glm::vec3(192.947067f, 129.507782f, 48.129314f);
	CCTV[0].up = glm::vec3(3.408169f, -1.742600f, 1.160055f);
		
	CCTV[1].pos = glm::vec3(160.424347f, 84.567108f, 38.933216f);
	CCTV[1].cen = glm::vec3(160.156708f, 84.169998f, 38.055340f);
	CCTV[1].up = glm::vec3(-3.546929f, -1.018898f, 1.542134f);

	CCTV[2].pos = glm::vec3(15.529955f, 71.240005f, 52.596512f);
	CCTV[2].cen = glm::vec3(15.992289f, 71.533043f, 51.759628f);
	CCTV[2].up = glm::vec3(1.267686f, 3.306948f, 1.858406f);

	CCTV[3].pos = glm::vec3(75.399796, 118.156868, 3.236091);
	CCTV[3].cen = glm::vec3(76.150238, 117.514671, 3.392353);
	CCTV[3].up = glm::vec3(-0.441037, 0.445893, 3.950567);
	CCTV[3].pos = glm::vec3(70.0f, 120.0f, 3.0f);
	CCTV[3].cen = glm::vec3(71.0f, 119.0f, 3.5f);
	CCTV[3].up = glm::vec3(0.0f, 0.0f, 1.0f);
	
	for (int i = 0; i < 4; i++) {
		calc_axis(CCTV[i]);
		CameraViewMatrix[i + 1] = look_to_camera(CCTV[i]);
	}

	map[0].pos = glm::vec3(0.0f, 0.0f, 50.0f);
	map[0].cen = glm::vec3(0.0f);
	map[0].up = glm::vec3(0.0f, 1.0f, 0.0f);
	Map_ProjectionMatrix[0] = glm::ortho(0.0f, 240.0f, 0.0f, 170.0f, 1.0f, 10000.0f);

	map[1].pos = glm::vec3(0.0f, 0.0f, 0.0f);
	map[1].cen = glm::vec3(0.0f, 1.0f, 0.0f);
	map[1].up = glm::vec3(0.0f, 0.0f, 1.0f);
	Map_ProjectionMatrix[1] = glm::ortho(0.0f, 240.0f, 0.0f, 60.0f, 1.0f, 10000.0f);

	map[2].pos = glm::vec3(0.0f, 0.0f, 0.0f);
	map[2].cen = glm::vec3(1.0f, 0.0f, 0.0f);
	map[2].up = glm::vec3(0.0f, 0.0f, 1.0f);
	Map_ProjectionMatrix[2] = glm::ortho(-170.0f, 0.0f, 0.0f, 60.0f, 1.0f, 10000.0f);

	for (int i = 0; i < 3; i++) {
		calc_axis(map[i]);
		Map_ViewMatrix[i] = look_to_camera(map[i]);
	}

	initialize_lights_and_material();

	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(600, 400);
}

void set_up_scene_lights(void) {
	//fprintf(stdout, "%d set_up\n", nowShader);
	// point_light_EC: use light 0
	light[0].light_on = 1;
	light[0].position[0] = 0.0f; light[0].position[1] = 0.0f; 	// point light position in EC
	light[0].position[2] = 0.0f; light[0].position[3] = 1.0f;

	light[0].ambient_color[0] = 0.3f; light[0].ambient_color[1] = 0.3f;
	light[0].ambient_color[2] = 0.3f; light[0].ambient_color[3] = 1.0f;

	light[0].diffuse_color[0] = 0.7f; light[0].diffuse_color[1] = 0.7f;
	light[0].diffuse_color[2] = 0.7f; light[0].diffuse_color[3] = 1.0f;

	light[0].specular_color[0] = 0.9f; light[0].specular_color[1] = 0.9f;
	light[0].specular_color[2] = 0.9f; light[0].specular_color[3] = 1.0f;

	// spot_light_WC: use light 1
	light[1].light_on = 1;
	light[1].position[0] = 0.0f; light[1].position[1] = 0.0f; // spot light position in WC
	light[1].position[2] = 0.0f; light[1].position[3] = 1.0f;

	light[1].ambient_color[0] = 0.0f; light[1].ambient_color[1] = 0.0f;
	light[1].ambient_color[2] = 0.0f; light[1].ambient_color[3] = 1.0f;

	light[1].diffuse_color[0] = 1.0f; light[1].diffuse_color[1] = 0.0f;
	light[1].diffuse_color[2] = 0.0f; light[1].diffuse_color[3] = 1.0f;

	light[1].specular_color[0] = 1.0f; light[1].specular_color[1] = 0.0f;
	light[1].specular_color[2] = 0.0f; light[1].specular_color[3] = 1.0f;

	light[1].spot_direction[0] = 0.0f; light[1].spot_direction[1] = 0.0f; // spot light direction in WC
	light[1].spot_direction[2] = -1.0f;
	light[1].spot_cutoff_angle = 10.0f;
	light[1].spot_exponent = 0.0f;

	glUseProgram(h_ShaderProgram_SCR);
	glUniform1i(loc_scr_light.light_on, light[0].light_on);
	glUniform4fv(loc_scr_light.position, 1, light[0].position);
	glUniform4fv(loc_scr_light.ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_scr_light.diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_scr_light.specular_color, 1, light[0].specular_color);

	glUseProgram(h_ShaderProgram);
	glUniform1i(loc_light[0].light_on, light[0].light_on);
	glUniform4fv(loc_light[0].position, 1, light[0].position);
	glUniform4fv(loc_light[0].ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_light[0].diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_light[0].specular_color, 1, light[0].specular_color);

	glUniform1i(loc_light[1].light_on, light[1].light_on);
	glUniform4fv(loc_light[1].position, 1, light[1].position);
	glUniform4fv(loc_light[1].ambient_color, 1, light[1].ambient_color);
	glUniform4fv(loc_light[1].diffuse_color, 1, light[1].diffuse_color);
	glUniform4fv(loc_light[1].specular_color, 1, light[1].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glUniform3fv(loc_light[1].spot_direction, 1, light[1].spot_direction);
	glUniform1f(loc_light[1].spot_cutoff_angle, light[1].spot_cutoff_angle);
	glUniform1f(loc_light[1].spot_exponent, light[1].spot_exponent);
}

void prepare_scene(void) {
	define_axes();
	define_static_objects();
	define_animated_tiger();
	prepare_floor();
	set_up_scene_lights();
	prepare_geom_obj(GEOM_OBJ_ID_CAR_BODY, "Data/car_body_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_CAR_WHEEL, "Data/car_wheel_triangles_v.txt", GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_CAR_NUT, "Data/car_nut_triangles_v.txt", GEOM_OBJ_TYPE_V);
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void print_message(const char * m) {
	fprintf(stdout, "%s\n\n", m);
}

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 1
void main(int argc, char *argv[]) { 
	char program_name[256] = "Sogang CSE4170 Our_House_GLSL_V_0.5";
	char messages[N_MESSAGE_LINES][256] = { "    - Keys used: 'c', 'f', 'd', 'ESC'" };

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(1600, 800);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}

