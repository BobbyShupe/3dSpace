//							gcc -o opengl opengl.c -lm -lGL -lGLU -lglut -lGLEW
#define _OPEN_SYS_ITOA_EXT
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/freeglut.h>

#define FPS 60
#define TO_RADIANS 3.14/180.0

//width and height of the window ( Aspect ratio 16:9 )
const int width = 16*50;
const int height = 9*50;

float pitch = 0.0, yaw= 0.0, roll=0.0;
float camX=0.0,camZ=0.0, camY=0.0;

void display();
void reshape(int w,int h);
void timer(int);
void passive_motion(int,int);
void camera();
void keyboard(unsigned char key,int x,int y);
void keyboard_up(unsigned char key,int x,int y);
void drawText(float, float, char*);
uint8_t editMode = 0;
uint8_t editSubMode = 0;

#define MODE_NORMAL			0x00
#define MODE_SCALE			0x01
#define MODE_MOVE			0x02
#define MODE_ROTATE			0x03
#define MODE_TEXTURE    	0x04
#define MODE_TEXTURECOORDS  0X05
#define MODE_MAX			0x05

#define MODE_SUBMODE_01		0x00
#define MODE_SUBMODE_02		0x01
#define MODE_SUBMODE_03		0x02
#define MODE_SUBMODE_04		0x03
#define MODE_SUBMODE_05		0x04
#define MODE_SUBMODE_06		0x05
#define MODE_SUBMODE_07		0x06
#define MODE_SUBMODE_08		0x07
#define MODE_SUBMODE_MAX	0x07

void drawStatus();
struct Motion
{
    bool Forward,Backward,Left,Right,Up,Down;
};

struct cube
{
	float x,y,z,w,h,d,rX,rY,rZ;
	uint16_t image;
	char imageFileName[32];
	float textureCoords[9];
};

struct imageParameters
{
	int width;
	int height;
	int nrChannels;	
};

unsigned char* imageData;
struct imageParameters imageParams[9999];


struct Motion motion = {false,false,false,false,false,false};

struct cube* cubes;
uint16_t cubeCount = 0;
struct cube newCube;

void makeCube(float,float,float);
void drawCubes();
void init()
{
    glutSetCursor(GLUT_CURSOR_NONE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glutWarpPointer(width/2,height/2);
}

unsigned int textures[9999];
uint16_t textureCount = 0;

uint16_t selectionIndex = 0;

bool keys[255];

bool axisX = false;
bool axisY = false;
bool axisZ = false;

bool scale = false;
bool move = false;
bool rotate = false;

bool drawInfo = true;

char* filenames[9999];
uint16_t filenamesCount = 0;
char* str;
char* tmpstr;
char* strLine;

int main(int argc,char**argv)
{
	tmpstr = (char*)malloc(sizeof(char) * 5);
	strLine = (char*)malloc(sizeof(char) * 255);
	str = (char*) malloc(sizeof(char) * 4);
	cubes = (struct cube*) malloc(sizeof(struct cube) * 10000);
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutCreateWindow("Projectile Motion - 3D Simulation");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutPassiveMotionFunc(passive_motion);
    glutTimerFunc(0,timer,0);    //more info about this is given below at definition of timer()
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboard_up);

	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (dir->d_type == DT_REG && strlen(dir->d_name) > 4)
			{
				memset(str, 0, 4);
				memcpy(str, &dir->d_name[strlen(dir->d_name) - 4], 4);
				if (!strcmp(str, ".jpg") || !strcmp(str, ".JPG") || !strcmp(str, ".png") || !strcmp(str, ".PNG"))
				{
					filenames[filenamesCount] = (char*)malloc(sizeof(char) * strlen(dir->d_name));
					memset(filenames[filenamesCount], 0, strlen(dir->d_name));
					strcpy(filenames[filenamesCount], dir->d_name);
//					printf("%s\n", filenames[filenamesCount]);
					filenamesCount++;
				}				
			}
		}
		closedir(d);
	}
	if (filenamesCount > 0)
	{
		for (uint16_t i = 0; i < (filenamesCount); i ++)
		{
			printf("%d %s\n",i, filenames[i]);
			textureCount ++;
			imageData = stbi_load(filenames[i], &imageParams[textureCount].width, &imageParams[textureCount].height, &imageParams[textureCount].nrChannels, 0);
			glGenTextures(1, &textures[textureCount]);
			
			glBindTexture(GL_TEXTURE_2D, textures[textureCount]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			if (!imageData) {printf("Failed to load file %s\n", filenames[i]); exit(1);}
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, imageParams[textureCount].width, imageParams[textureCount].height, GL_RGB, GL_UNSIGNED_BYTE, imageData);
		
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageParams[textureCount].width, imageParams[textureCount].height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

			stbi_image_free(imageData);			

	//glActiveTexture(GL_TEXTURE0);
	//	glGenerateMipmap(GL_TEXTURE_2D);

		}
	}

    glutMainLoop();
    return 0;
}

/* This function just draws the scene. I used Texture mapping to draw
   a chessboard like surface. If this is too complicated for you ,
   you can just use a simple quadrilateral */

void draw()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_TEXTURE_2D);
	GLuint floorTexture;
    glGenTextures(1,&floorTexture);

    unsigned char texture_data[2][2][4] =
                    {
                        0,0,0,255,  255,255,255,255,
                        255,255,255,255,    0,0,0,255
                    };

    glBindTexture(GL_TEXTURE_2D,floorTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,texture_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);

    glBegin(GL_QUADS);

    glTexCoord2f(0.0,0.0);  glVertex3f(-50.0,-5.0,-50.0);
    glTexCoord2f(25.0,0.0);  glVertex3f(50.0,-5.0,-50.0);
    glTexCoord2f(25.0,25.0);  glVertex3f(50.0,-5.0,50.0);
    glTexCoord2f(0.0,25.0);  glVertex3f(-50.0,-5.0,50.0);

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    camera();
    draw();

    drawCubes();
    if (drawInfo) drawStatus();

    glutSwapBuffers();
}

void reshape(int w,int h)
{
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60,16.0/9.0,.01,7500);
    glMatrixMode(GL_MODELVIEW);

}


/*this funtion is used to keep calling the display function periodically
  at a rate of FPS times in one second. The constant FPS is defined above and
  has the value of 60
*/
void timer(int)
{
    glutPostRedisplay();
    glutWarpPointer(width/2,height/2);
    glutTimerFunc(1000/FPS,timer,0);
}

void passive_motion(int x,int y)
{
    /* two variables to store X and Y coordinates, as observed from the center
      of the window
    */
    int dev_x,dev_y;
    dev_x = (width/2)-x;
    dev_y = (height/2)-y;

    /* apply the changes to pitch and yaw*/
    yaw+=(float)dev_x/20.0;
    pitch+=(float)dev_y/20.0;
}

void camera()
{

    if(motion.Forward)
    {
        camX += cos((yaw+90)*TO_RADIANS)/50.0;
        camZ -= sin((yaw+90)*TO_RADIANS)/50.0;
    }
    if(motion.Backward)
    {
        camX += cos((yaw+90+180)*TO_RADIANS)/50.0;
        camZ -= sin((yaw+90+180)*TO_RADIANS)/50.0;
    }
    if(motion.Left)
    {
        camX += cos((yaw+90+90)*TO_RADIANS)/50.0;
        camZ -= sin((yaw+90+90)*TO_RADIANS)/50.0;
    }
    if(motion.Right)
    {
        camX += cos((yaw+90-90)*TO_RADIANS)/50.0;
        camZ -= sin((yaw+90-90)*TO_RADIANS)/50.0;
    }
	if(motion.Up)
	{
	    camY += 1/50.0; // Move up
	}
	if(motion.Down)
	{
	    camY -= 1/50.0; // Move down
	}
    /*limit the values of pitch
      between -60 and 70
    */
    if(pitch>=70)
        pitch = 70;
    if(pitch<=-60)
        pitch=-60;

    glRotatef(-pitch,1.0,0.0,0.0); // Along X axis
    glRotatef(-yaw,0.0,1.0,0.0);    //Along Y axis
    glRotatef(-roll,0.0,0.0,1.0);    //Along Z axis
    glTranslatef(-camX,-camY,-camZ);
}

void keyboard(unsigned char key,int x,int y)
{
	printf("%d\n", key);
    switch(key)
    {
    case 'W':
    case 'w':
        motion.Forward = true;
        break;
    case 'A':
    case 'a':
        motion.Left = true;
        break;
    case 'S':
    case 's':
        motion.Backward = true;
        break;
    case 'D':
    case 'd':
        motion.Right = true;
        break;
    case 'E':
    case 'e':
        motion.Up = true;
        break;
    case 'Q':
    case 'q':
        motion.Down = true;
        break;
	case 27:
		exit(1);
	break;
	case ' ':
		makeCube(camX,camY,camZ);
	break;
	case 9://TAB key down
    	if (glutGetModifiers() & GLUT_ACTIVE_CTRL) { if (selectionIndex > 0) selectionIndex --; else selectionIndex = cubeCount - 1;} 
    	else
			selectionIndex ++;
		if (selectionIndex >= cubeCount) selectionIndex = 0;
	break;
	case 'x':
	case 'X':
		if (!keys['x'] && !keys['X'])
		{
			keys[key] = true;
			axisX = !axisX;	
		}
	break;
	case 'y':
	case 'Y':
		if (!keys['y'] && !keys['Y'])
		{
			keys[key] = true;
			axisY = !axisY;
		}
	break;
	case 'z':
	case 'Z':
		if (!keys['z'] && !keys['Z'])
		{
			keys[key] = true;
			axisZ = !axisZ;
		}
	break;
		if (cubeCount > 0)
		{
			case '+':
			case '=':
				switch(editMode)
				{
					case MODE_NORMAL:

					break;
					case MODE_SCALE:
						if (axisX) cubes[selectionIndex].w += 0.1f;
						if (axisY) cubes[selectionIndex].h += 0.1f;
						if (axisZ) cubes[selectionIndex].d += 0.1f;
					break;
					case MODE_MOVE:
						if (axisX) cubes[selectionIndex].x += 0.1f;
						if (axisY) cubes[selectionIndex].y += 0.1f;
						if (axisZ) cubes[selectionIndex].z += 0.1f;
					break;
					case MODE_ROTATE:
						if (axisX) 
						{
							cubes[selectionIndex].rX += 0.1f;
							if (cubes[selectionIndex].rX > 360.0f) cubes[selectionIndex].rX = 0.0f;
						}
						if (axisY)
						{
							cubes[selectionIndex].rY += 0.1f;
							if (cubes[selectionIndex].rY > 360.0f) cubes[selectionIndex].rY = 0.0f;
							
						} 
						if (axisZ)
						{
							cubes[selectionIndex].rZ += 0.1f;
							if (cubes[selectionIndex].rZ > 360.0f) cubes[selectionIndex].rZ = 0.0f;
						}
					break;
					case MODE_TEXTURE:
						cubes[selectionIndex].image ++;
						if (cubes[selectionIndex].image == (textureCount + 1)) 
							cubes[selectionIndex].image = 0;
					break;
					case MODE_TEXTURECOORDS:
						cubes[selectionIndex].textureCoords[editSubMode] += 0.1f;
						if (cubes[selectionIndex].textureCoords[editSubMode] > 1.0f)
							cubes[selectionIndex].textureCoords[editSubMode] = 0.0f;
					break;
				}
			break;
			case '-':
				switch(editMode)
				{
					case MODE_NORMAL:

					break;
					case MODE_SCALE:
						if (axisX) cubes[selectionIndex].w -= 0.1f;
						if (axisY) cubes[selectionIndex].h -= 0.1f;
						if (axisZ) cubes[selectionIndex].d -= 0.1f;
					break;
					case MODE_MOVE:
						if (axisX) cubes[selectionIndex].x -= 0.1f;
						if (axisY) cubes[selectionIndex].y -= 0.1f;
						if (axisZ) cubes[selectionIndex].z -= 0.1f;
					break;
					case MODE_ROTATE:
						if (axisX) 
						{
							cubes[selectionIndex].rX -= 0.1f;
							if (cubes[selectionIndex].rX < 0.0f) cubes[selectionIndex].rX = 360.0f;
						}
						if (axisY)
						{
							cubes[selectionIndex].rY -= 0.1f;
							if (cubes[selectionIndex].rY < 0.0f) cubes[selectionIndex].rY = 360.0f;
							
						} 
						if (axisZ)
						{
							cubes[selectionIndex].rZ -= 0.1f;
							if (cubes[selectionIndex].rZ < 0.0f) cubes[selectionIndex].rZ = 360.0f;
						}
					break;
					case MODE_TEXTURE:
						if (cubes[selectionIndex].image > 0) 
							cubes[selectionIndex].image --; 
						else 
							cubes[selectionIndex].image = textureCount;
					break;
					case MODE_TEXTURECOORDS:
						cubes[selectionIndex].textureCoords[editSubMode] -= 0.1f;
						if (cubes[selectionIndex].textureCoords[editSubMode] < 0.0f)
							cubes[selectionIndex].textureCoords[editSubMode] = 1.0f;
					break;
				}
			break;
		}
		case 'm':
		case 'M':
	    	if (glutGetModifiers() & GLUT_ACTIVE_ALT)
	    	{
	    		if (editMode == MODE_TEXTURECOORDS)
	    		{
		    		editSubMode ++;
		    		if (editSubMode > MODE_SUBMODE_MAX) editSubMode = 0;
	    		}
	    	} else 
			{
		    	editMode ++;
				if (editMode > MODE_MAX) editMode = MODE_NORMAL;
			}
		break;
		case 'i':
		case 'I':
	    	if (glutGetModifiers() & GLUT_ACTIVE_ALT) drawInfo = !drawInfo;
		break;
    }
}

void keyboard_up(unsigned char key,int x,int y)
{
    switch(key)
    {
    case 'W':
    case 'w':
        motion.Forward = false;
        break;
    case 'A':
    case 'a':
        motion.Left = false;
        break;
    case 'S':
    case 's':
        motion.Backward = false;
        break;
    case 'D':
    case 'd':
        motion.Right = false;
        break;
    case 'E':
    case 'e':
        motion.Up = false;
        break;
    case 'Q':
    case 'q':
        motion.Down = false;
        break;
	case 'x':
	case 'X':
	    keys[key] = false;
	break;
	case 'y':
	case 'Y':
	    keys[key] = false;
	break;
	case 'z':
	case 'Z':
	    keys[key] = false;
	break;
    }
}

void makeCube(float _x, float _y, float _z)
{
	newCube.x = _x; newCube.y = _y; newCube.z = _z;
	cubes[cubeCount].x = newCube.x;
	cubes[cubeCount].y = newCube.y;
	cubes[cubeCount].z = newCube.z;	
	cubes[cubeCount].w = 0.75f;
	cubes[cubeCount].h = 2.0f;
	cubes[cubeCount].d = 0.02f;
	cubes[cubeCount].textureCoords[0] = 1.0f;
	cubes[cubeCount].textureCoords[1] = 0.0f;
	cubes[cubeCount].textureCoords[2] = 0.0f;
	cubes[cubeCount].textureCoords[3] = 0.0f;
	cubes[cubeCount].textureCoords[4] = 0.0f;
	cubes[cubeCount].textureCoords[5] = 1.0f;
	cubes[cubeCount].textureCoords[6] = 1.0f;
	cubes[cubeCount].textureCoords[7] = 1.0f;

	cubeCount ++;
}

void drawCubes()
{
	for (uint16_t i = 0; i < cubeCount; i ++)
	{
		glEnable(GL_TEXTURE_2D);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glPushMatrix();
		glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);

    glBindTexture(GL_TEXTURE_2D, textures[cubes[i].image]);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageParams[imageCount].width, imageParams[imageCount].height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);

glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
                                  // Top face (y = 1.0f)
                                  // Define vertices in counter-clockwise (CCW) order with normal pointing out
glColor3ub(255, 255, 255);     // Green
//glTexCoord2f(0.0,0.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(1.0,0.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(1.0,1.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
//glTexCoord2f(0.0,1.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
// Bottom face (y = -1.0f)
glColor3ub(255, 255, 255);     // Orange
//glTexCoord2f(1.0,0.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
//glTexCoord2f(0.0,0.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
//glTexCoord2f(0.0,1.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(1.0,1.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);

// Front face  (z = 1.0f)
/*
glColor3f(1.0f, 1.0f, 1.0f);     // Red
glTexCoord2f(1.0,0.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(0.0,0.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(0.0,1.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(1.0,1.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
*/

glColor3f(1.0f, 1.0f, 1.0f);     // Red
glTexCoord2f(cubes[i].textureCoords[0],cubes[i].textureCoords[1]);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(cubes[i].textureCoords[2],cubes[i].textureCoords[3]);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(cubes[i].textureCoords[4],cubes[i].textureCoords[5]);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(cubes[i].textureCoords[6],cubes[i].textureCoords[7]);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);


// Back face (z = -1.0f)
glColor3ub(255, 255, 255);     // Yellow
//glTexCoord2f(0.0,0.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(1.0,0.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(1.0,1.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(0.0,1.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);

// Left face (x = -1.0f)
glColor3ub(255, 255, 255);    // Blue
//glTexCoord2f(0.0,0.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
//glTexCoord2f(1.0,0.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(1.0,1.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(0.0,1.0);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);

// Right face (x = 1.0f)
glColor3ub(255, 255, 255);     // Magenta
//glTexCoord2f(0.0,0.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
//glTexCoord2f(1.0,0.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
//glTexCoord2f(1.0,1.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
//glTexCoord2f(0.0,1.0);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);

glEnd();  // End of drawing color-cube



		if (selectionIndex == i)
		{
			glDisable(GL_TEXTURE_2D);
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);


			glBegin(GL_QUADS);                // Begin draw	ing the color cube with 6 quads
                                  // Top face (y = 1.0f)
                                  // Define vertices in counter-clockwise (CCW) order with normal pointing out
			glColor3f(1.0f, 1.0f, 1.0f);     // Green
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);

// Bottom face (y = -1.0f)
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);

// Front face  (z = 1.0f)
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);

// Back face (z = -1.0f)
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);

// Left face (x = -1.0f)
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);

// Right face (x = 1.0f)
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * -1.0f);
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
			glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * -1.0f);
	
			glEnd();  // End of drawing color-cube

		}
		glPopMatrix();

	}
	glColor3f(1.0f, 1.0f, 1.0f);
}

void drawText(float _x, float _y, char* text)
{
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, glutGet(GLUT_WINDOW_WIDTH), 0.0, glutGet(GLUT_WINDOW_HEIGHT));
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRasterPos2f(_x, _y);
	glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)text);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
}

uint8_t textIndex;
void drawStatus()
{
	textIndex = 1;
	glColor3f(0.2f, 0.8f,0.2f);
	memset(strLine, 0, 255);
	strcpy(strLine, "Cube Count ");
	memset(tmpstr, 0, 5);
	sprintf(tmpstr, "%d", cubeCount);
	strcat(strLine, tmpstr);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	strcpy(strLine, "Texture Count ");
	memset(tmpstr, 0, 5);
	sprintf(tmpstr, "%d", textureCount);
	strcat(strLine, tmpstr);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	memset(tmpstr, 0, 5);
	sprintf(tmpstr, "AxisX %d", axisX);
	strcat(strLine, tmpstr);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	memset(tmpstr, 0, 5);
	sprintf(tmpstr, "AxisY %d", axisY);
	strcat(strLine, tmpstr);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	memset(tmpstr, 0, 5);
	sprintf(tmpstr, "AxisZ %d", axisZ);
	strcat(strLine, tmpstr);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	switch (editMode)
	{
		case MODE_NORMAL:
			strcpy(strLine, "MODE Normal");
		break;
		case MODE_SCALE:
			strcpy(strLine, "MODE Scale");
		break;
		case MODE_MOVE:
			strcpy(strLine, "MODE Move");
		break;
		case MODE_ROTATE:
			strcpy(strLine, "MODE Rotate");
		break;
		case MODE_TEXTURE:
			strcpy(strLine, "MODE Texture");
		break;
		case MODE_TEXTURECOORDS:
			strcpy(strLine, "MODE Texture Coords");
		break;
	}
	memset(tmpstr, 0, 5);
//	sprintf(tmpstr, "AxisZ %d", axisZ);
	strcat(strLine, tmpstr);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;
	
	if (cubeCount > 0)
	{
		textIndex++;
		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "Selected Cube %d", selectionIndex);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "	X %f", cubes[selectionIndex].x);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "	Y %f", cubes[selectionIndex].y);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "	Z %f", cubes[selectionIndex].z);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "	W %f", cubes[selectionIndex].w);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "	H %f", cubes[selectionIndex].h);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "	D %f", cubes[selectionIndex].d);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		memset(tmpstr, 0, 5);
		sprintf(tmpstr, "	Texture %d", cubes[selectionIndex].image);
		strcpy(strLine, tmpstr);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		if (editMode == MODE_TEXTURECOORDS)
		{

			for (uint8_t i = 0; i < 8; i ++)
			{
				memset(strLine, 0, 255);
				memset(tmpstr, 0, 5);
				if (i == editSubMode)
					sprintf(tmpstr, "->Texture Coordinate %d: %.1f", i, cubes[selectionIndex].textureCoords[i]);
				else
					sprintf(tmpstr, " 	   Texture Coordinate %d: %.1f", i, cubes[selectionIndex].textureCoords[i]);
				strcpy(strLine, tmpstr);
				drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
				textIndex++;			
			}			
		}
	}
	glColor3f(1.0f, 1.0f, 1.0f);	
}
