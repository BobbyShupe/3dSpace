//									gcc -o opengl opengl.c -lm -lGL -lGLU -lglut -lGLEW
#define _OPEN_SYS_ITOA_EXT
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/freeglut.h>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define TO_RADIANS 3.14/180.0

//width and height of the window ( Aspect ratio 16:9 )
const int width = 16*50;
const int height = 900;

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
    bool Forward,Backward,Left,Right,Up,Down,ctrl,shift,rotLeft,rotRight,rotUp,rotDown;
};

struct cube
{
	float x,y,z,w,h,d,rX,rY,rZ;
	uint16_t image;
	char imageFileName[64];
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


struct Motion motion = {false,false,false,false,false,false,false,false,false,false,false,false};

struct cube* cubes;
struct cube cubeCopy;
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


void saveData();
void loadData();

bool exiting = false;

void setSaveCubes();
void setSaveCfg();
void setSaveCubes();
bool saveCubes = false;
bool saveCfg = false;
float multiplier = 0.0f;

void deleteCube(uint16_t);
void copyCube();
void pasteCube();
bool copyBufferFull = false;

void specialKeyboard(int key,int x,int y);
void specialKeyboardUp(int key,int x,int y);

bool constrainPasteToOriginalPosition = false;
bool constrainToTextureDimensions = false;

void incValue();
void decValue();
void mouseWheel(int,int,int,int);

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
	glutSpecialFunc(specialKeyboard);
	glutSpecialUpFunc(specialKeyboardUp);
	glutMouseWheelFunc(mouseWheel);
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
				if (!strcmp(str, ".jpg") || !strcmp(str, ".JPG") 
				|| !strcmp(str, ".png") || !strcmp(str, ".PNG")
				|| !strcmp(str, ".bmp") || !strcmp(str, ".BMP"))
				{
					filenames[filenamesCount] = (char*)malloc(sizeof(char) * strlen(dir->d_name));
					memset(filenames[filenamesCount], 0, strlen(dir->d_name));
					strcpy(filenames[filenamesCount], dir->d_name);
					//printf("%d %s\n",filenamesCount, filenames[filenamesCount]);
					filenamesCount++;
				}				
			}
		}
		closedir(d);
	}

	char* filenameStr = malloc(sizeof(char*) * 64);
	memset(filenameStr, 0, 64);
	
	for (uint16_t ii =0; ii < filenamesCount - 1; ii ++)
	{
		for (uint16_t i = ii; i < filenamesCount - 1; i++)
		{
			if (strcmp(filenames[i], filenames[i+1]) > 0)
			{
				strcpy(filenameStr, filenames[i+1]);
				strcpy(filenames[i+1], filenames[i]);
				strcpy(filenames[i], (const char*)filenameStr);
			}
		}
	}

	free(filenameStr);
	
	if (filenamesCount > 0)
	{
		struct stat file_status;
		for (uint16_t i = 0; i < (filenamesCount); i ++)
		{
			printf("%d found %s\n",i, filenames[i]);
			stat(filenames[i], &file_status);
			imageData = (unsigned char*) malloc(sizeof(char) * file_status.st_size);
			imageData = stbi_load(filenames[i], &imageParams[textureCount].width, &imageParams[textureCount].height, &imageParams[textureCount].nrChannels, STBI_rgb);
			glGenTextures(1, &textures[textureCount]);
			
			glBindTexture(GL_TEXTURE_2D, textures[textureCount]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			if (!imageData) {printf("Failed to load file %s\n", filenames[i]); exit(1);}
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, imageParams[textureCount].width, imageParams[textureCount].height, GL_RGB, GL_UNSIGNED_BYTE, imageData);
		
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageParams[textureCount].width, imageParams[textureCount].height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
			textureCount ++;

			stbi_image_free(imageData);			

	//glActiveTexture(GL_TEXTURE0);
	//	glGenerateMipmap(GL_TEXTURE_2D);

		}
	}
	loadData();
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
    //draw();

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
//    if (pitch > 360) pitch -= 360.0f;
 //   if (pitch < 0) pitch += 360.0f;
    if (yaw > 360) yaw -= 360.0f;
	if (yaw < 0) yaw += 360.0;
}

void camera()
{
	if (motion.ctrl) multiplier = 0.1f;
	if (motion.shift) multiplier = 5.0f;
	if (!motion.ctrl && !motion.shift) multiplier = 1.0f;
    if(motion.Forward)
    {
        camX += (cos((yaw+90)*TO_RADIANS)/50.0) * multiplier;
        camZ -= (sin((yaw+90)*TO_RADIANS)/50.0) * multiplier;
        camY += (tan((pitch)*TO_RADIANS)/50.0) * multiplier;
		setSaveCfg();
    }
    if(motion.Backward)
    {
        camX += (cos((yaw+90+180)*TO_RADIANS)/50.0) * multiplier;
        camZ -= (sin((yaw+90+180)*TO_RADIANS)/50.0) * multiplier;
        camY -= (tan((pitch)*TO_RADIANS)/50.0) * multiplier;
        setSaveCfg();
    }
    if(motion.Left)
    {
        camX += (cos((yaw+90+90)*TO_RADIANS)/50.0) * multiplier;
        camZ -= (sin((yaw+90+90)*TO_RADIANS)/50.0) * multiplier;
        setSaveCfg();
    }
    if(motion.Right)
    {
        camX += (cos((yaw+90-90)*TO_RADIANS)/50.0) * multiplier;
        camZ -= (sin((yaw+90-90)*TO_RADIANS)/50.0) * multiplier;
        setSaveCfg();
    }
	if(motion.Up)
	{
	    camY += (1/50.0) * multiplier; // Move up
	    setSaveCfg();
	}
	if(motion.Down)
	{
	    camY -= (1/50.0) * multiplier; // Move down
	    setSaveCfg();
	}
	if(motion.rotLeft)
	{
		yaw+=1.0f;
	}
	if(motion.rotUp)
	{
		pitch+=1.0f;
	}
	if(motion.rotRight)
	{
		yaw-=1.0f;	
	}
	if(motion.rotDown)
	{
		pitch-=1.0f;	
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
	//if (glutGetModifiers() & GLUT_ACTIVE_ALT) motion.ctrl = true; else motion.ctrl = false;
	if (motion.ctrl) multiplier = 0.1f;
	if (motion.shift) multiplier = 2.0f;
	if (!motion.ctrl && !motion.shift) multiplier = 1.0f;
//''	printf("%d\n", key);
    switch(key)
    {
    case 'W':
    	motion.shift = true;
    case 'w':
        motion.Forward = true;
        break;
    case 'A':
    	motion.shift = true;
    case 'a':
        motion.Left = true;
        break;
    case 'S':
    	motion.shift = true;
    case 's':
        motion.Backward = true;
        break;
    case 'D':
    	motion.shift = true;
    case 'd':
        motion.Right = true;
        break;
    case 'E':
    	motion.shift = true;
    case 'e':
        motion.Up = true;
        break;
    case 'Q':
    	motion.shift = true;
    case 'q':
        motion.Down = true;
        break;
	case 27:
		exiting = true;
	break;
	case ' ':
		makeCube(camX,camY,camZ);
		setSaveCubes();
	break;
	case 9://TAB key down
    	if (glutGetModifiers() & GLUT_ACTIVE_CTRL) { if (selectionIndex > 0) selectionIndex --; else selectionIndex = cubeCount - 1;} 
    	else
			selectionIndex ++;
		if (selectionIndex >= cubeCount) selectionIndex = 0;
		setSaveCfg();
	break;
	case 'x':
	case 'X':
		if (!keys['x'] && !keys['X'])
		{
			keys[key] = true;
			axisX = !axisX;	
		}
		setSaveCfg();
	break;
	case 'y':
	case 'Y':
		if (!keys['y'] && !keys['Y'])
		{
			keys[key] = true;
			axisY = !axisY;
		}
		setSaveCfg();
	break;
	case 'z':
	case 'Z':
		if (!keys['z'] && !keys['Z'])
		{
			keys[key] = true;
			axisZ = !axisZ;
		}
		setSaveCfg();
	break;
		if (cubeCount > 0)
		{
			case '+':
				switch(editMode)
				{
					case MODE_MOVE:
	//					if (glutGetModifiers() & GLUT_ACTIVE_ALT)
	//					{
							if (axisX) cubes[selectionIndex].x += cubes[selectionIndex].w * 0.002;
							if (axisY) cubes[selectionIndex].y += cubes[selectionIndex].h * 0.002;
							if (axisZ) cubes[selectionIndex].z += cubes[selectionIndex].d * 0.002;							
	//					} else
	//					{
							if (axisX) cubes[selectionIndex].x += 0.1f * 0.002;
							if (axisY) cubes[selectionIndex].y += 0.1f * 0.002;
							if (axisZ) cubes[selectionIndex].z += 0.1f * 0.002;							
//						}
						setSaveCubes();						
					break;
				}
			break;
			case '=':
				switch(editMode)
				{
					case MODE_NORMAL:

					break;
					case MODE_SCALE:
						if (axisX) cubes[selectionIndex].w += 0.1f * multiplier;
						if (axisY) cubes[selectionIndex].h += 0.1f * multiplier;
						if (axisZ) cubes[selectionIndex].d += 0.1f * multiplier;
						setSaveCubes();
					break;
					case MODE_MOVE:
						if (glutGetModifiers() & GLUT_ACTIVE_ALT)
						{
							if (axisX) cubes[selectionIndex].x += cubes[selectionIndex].w * multiplier;
							if (axisY) cubes[selectionIndex].y += cubes[selectionIndex].h * multiplier;
							if (axisZ) cubes[selectionIndex].z += cubes[selectionIndex].d * multiplier;							
						} else
						{
							if (axisX) cubes[selectionIndex].x += 0.1f * multiplier;
							if (axisY) cubes[selectionIndex].y += 0.1f * multiplier;
							if (axisZ) cubes[selectionIndex].z += 0.1f * multiplier;							
						}
						setSaveCubes();
					break;
					case MODE_ROTATE:
						if (axisX) 
						{
							cubes[selectionIndex].rX += 0.1f * multiplier;
							if (cubes[selectionIndex].rX > 360.0f) cubes[selectionIndex].rX = 0.0f;
							setSaveCubes();

						}
						if (axisY)
						{
							cubes[selectionIndex].rY += 0.1f * multiplier;
							if (cubes[selectionIndex].rY > 360.0f) cubes[selectionIndex].rY = 0.0f;
							setSaveCubes();
						} 
						if (axisZ)
						{
							cubes[selectionIndex].rZ += 0.1f * multiplier;
							if (cubes[selectionIndex].rZ > 360.0f) cubes[selectionIndex].rZ = 0.0f;
							setSaveCubes();
						}
					break;
					case MODE_TEXTURE:
						cubes[selectionIndex].image ++;
						if (cubes[selectionIndex].image == (textureCount)) 
							cubes[selectionIndex].image = 0;
						strcpy(cubes[selectionIndex].imageFileName, filenames[cubes[selectionIndex].image]);

						if (constrainToTextureDimensions)
						{
							if (axisX) cubes[selectionIndex].w = imageParams[cubes[selectionIndex].image].width * 0.001f;
							if (axisY) cubes[selectionIndex].h = imageParams[cubes[selectionIndex].image].height * 0.001f;
						}
						setSaveCubes();
					break;
					case MODE_TEXTURECOORDS:
						cubes[selectionIndex].textureCoords[editSubMode] += 0.1f;
						if (cubes[selectionIndex].textureCoords[editSubMode] > 1.0f)
							cubes[selectionIndex].textureCoords[editSubMode] = 0.0f;
						setSaveCubes();
					break;
				}
			break;
			case '_':
				switch(editMode)
				{
					case MODE_MOVE:
						//if (glutGetModifiers() & GLUT_ACTIVE_AL)
						//{
							if (axisX) cubes[selectionIndex].x -= cubes[selectionIndex].w * 0.002;
							if (axisY) cubes[selectionIndex].y -= cubes[selectionIndex].h * 0.002;
							if (axisZ) cubes[selectionIndex].z -= cubes[selectionIndex].d * 0.002;							
			//			} else
		//				{
							if (axisX) cubes[selectionIndex].x -= 0.1f * 0.002;
							if (axisY) cubes[selectionIndex].y -= 0.1f * 0.002;
							if (axisZ) cubes[selectionIndex].z -= 0.1f * 0.002;							
	//					}
						setSaveCubes();						
					break;
				}
			break;
			case '-':
				switch(editMode)
				{
					case MODE_NORMAL:

					break;
					case MODE_SCALE:
						if (axisX) cubes[selectionIndex].w -= 0.1f * multiplier;
						if (axisY) cubes[selectionIndex].h -= 0.1f * multiplier;
						if (axisZ) cubes[selectionIndex].d -= 0.1f * multiplier;
						setSaveCubes();
					break;
					case MODE_MOVE:
						if (glutGetModifiers() & GLUT_ACTIVE_ALT)
						{
							if (axisX) cubes[selectionIndex].x -= cubes[selectionIndex].w * multiplier;
							if (axisY) cubes[selectionIndex].y -= cubes[selectionIndex].h * multiplier;
							if (axisZ) cubes[selectionIndex].z -= cubes[selectionIndex].d * multiplier;							
						} else
						{
							if (axisX) cubes[selectionIndex].x -= 0.1f * multiplier;
							if (axisY) cubes[selectionIndex].y -= 0.1f * multiplier;
							if (axisZ) cubes[selectionIndex].z -= 0.1f * multiplier;							
						}
						setSaveCubes();
					break;
					case MODE_ROTATE:
						if (axisX) 
						{
							cubes[selectionIndex].rX -= 0.1f * multiplier;
							if (cubes[selectionIndex].rX < 0.0f) cubes[selectionIndex].rX = 360.0f;
							setSaveCubes();
						}
						if (axisY)
						{
							cubes[selectionIndex].rY -= 0.1f * multiplier;
							if (cubes[selectionIndex].rY < 0.0f) cubes[selectionIndex].rY = 360.0f;
							setSaveCubes();
						} 
						if (axisZ)
						{
							cubes[selectionIndex].rZ -= 0.1f * multiplier;
							if (cubes[selectionIndex].rZ < 0.0f) cubes[selectionIndex].rZ = 360.0f;
							setSaveCubes();
						}
					break;
					case MODE_TEXTURE:

						if (cubes[selectionIndex].image > 0)
							cubes[selectionIndex].image --;
						else
							cubes[selectionIndex].image = textureCount - 1;

						memset(cubes[selectionIndex].imageFileName, 0, 64);
						strcpy(cubes[selectionIndex].imageFileName, filenames[cubes[selectionIndex].image]);
						if (constrainToTextureDimensions)
						{
							if (axisX) cubes[selectionIndex].w = imageParams[cubes[selectionIndex].image].width * 0.001f;
							if (axisY) cubes[selectionIndex].h = imageParams[cubes[selectionIndex].image].height * 0.001f;
						}
						
						setSaveCubes();
					break;
					case MODE_TEXTURECOORDS:
						cubes[selectionIndex].textureCoords[editSubMode] -= 0.1f;
						if (cubes[selectionIndex].textureCoords[editSubMode] < 0.0f)
							cubes[selectionIndex].textureCoords[editSubMode] = 1.0f;
						setSaveCubes();
					break;
				}
			break;
			case 127:
				deleteCube(selectionIndex);
			break;
			case 'C':
			case 'c':
				copyCube();
			break;
			case 'V':
			case 'v':
				pasteCube();
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
			setSaveCfg();
		break;
		case 'i':
		case 'I':
	    	if (glutGetModifiers() & GLUT_ACTIVE_ALT) drawInfo = !drawInfo;
	    	setSaveCfg();
		break;
		case 'R':
		case 'r':
			if (glutGetModifiers() & GLUT_ACTIVE_ALT)
			{
				if (cubeCount > 0)
				{
					if (axisX) cubes[selectionIndex].rX = 0.0f;
					if (axisY) cubes[selectionIndex].rY = 0.0f;
					if (axisZ) cubes[selectionIndex].rZ = 0.0f;
				}
			}
		break;
    }

    if (exiting)
    {
    	saveData();
    	exit(1);
    }
}

void keyboard_up(unsigned char key,int x,int y)
{
    motion.shift = false;
    motion.ctrl = false;
    switch(key)
    {
    case 'W':
    case 'w':
        motion.Forward = false;
        break;
    case 'A':
    	motion.shift = false;
    case 'a':
        motion.Left = false;
        break;
    case 'S':
    	motion.shift = false;
    case 's':
        motion.Backward = false;
        break;
    case 'D':
    	motion.shift = false;
    case 'd':
        motion.Right = false;
        break;
    case 'E':
    	motion.shift = false;
    case 'e':
        motion.Up = false;
        break;
    case 'Q':
    	motion.shift = false;
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

void specialKeyboard(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_LEFT:
		    motion.rotLeft = true;
		break;
		case GLUT_KEY_UP:
		    motion.rotUp = true;
		break;
		case GLUT_KEY_RIGHT:
		    motion.rotRight = true;		
		break;
		case GLUT_KEY_DOWN:
		    motion.rotDown = true;		
		break;
		case GLUT_KEY_F1:
			constrainPasteToOriginalPosition = !constrainPasteToOriginalPosition;
		break;
		case GLUT_KEY_F2:
			constrainToTextureDimensions = !constrainToTextureDimensions;
		break;
	}
}

void specialKeyboardUp(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_LEFT:
			motion.rotLeft = false;
		break;
		case GLUT_KEY_UP:
			motion.rotUp = false;
		break;
		case GLUT_KEY_RIGHT:
			motion.rotRight = false;
		break;
		case GLUT_KEY_DOWN:
			motion.rotDown = false;
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
	cubes[cubeCount].image = 0;
	memset(cubes[cubeCount].imageFileName, 0, 64);
	if (textureCount > 0)
		strcpy(cubes[cubeCount].imageFileName, filenames[cubes[cubeCount].image]);
	cubeCount ++;
	
}

void drawCubes()
{
	glEnable(GL_TEXTURE_2D);

	for (uint16_t i = 0; i < cubeCount; i ++)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glPushMatrix();
		glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
	    glRotatef(cubes[i].rX,1.0,0.0,0.0); // Along X axis
	    glRotatef(cubes[i].rY,0.0,1.0,0.0);    //Along Y axis
	    glRotatef(cubes[i].rZ,0.0,0.0,1.0);    //Along Z axis

    glBindTexture(GL_TEXTURE_2D, textures[cubes[i].image]);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageParams[imageCount].width, imageParams[imageCount].height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);


glDisable(GL_TEXTURE_2D);

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

glEnd();  // End of drawing color-cube


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

glEnable(GL_TEXTURE_2D);
glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads




glColor3f(1.0f, 1.0f, 1.0f);     // Red
glTexCoord2f(cubes[i].textureCoords[0],cubes[i].textureCoords[1]);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(cubes[i].textureCoords[2],cubes[i].textureCoords[3]);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * 1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(cubes[i].textureCoords[4],cubes[i].textureCoords[5]);
glVertex3f((cubes[i].w * 0.5) * -1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);
glTexCoord2f(cubes[i].textureCoords[6],cubes[i].textureCoords[7]);
glVertex3f((cubes[i].w * 0.5) * 1.0f, (cubes[i].h * 0.5) * -1.0f, (cubes[i].d * 0.5) * 1.0f);




glEnd();  // End of drawing color-cube

glDisable(GL_TEXTURE_2D);

glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads






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
		glDisable(GL_LIGHTING);
            glColor3f(0.0f, 1.0f, 0.0f);  // Green color
            glLineWidth(2.0f);  // Set line width for the outline
            
            float w = cubes[i].w / 2;
            float h = cubes[i].h / 2;
            float d = cubes[i].d / 2;
            
            // Draw outline for each face
            glBegin(GL_LINE_LOOP);
            glVertex3f(-w, -h, -d);
            glVertex3f(w, -h, -d);
            glVertex3f(w, h, -d);
            glVertex3f(-w, h, -d);
            glEnd();
            
            glBegin(GL_LINE_LOOP);
            glVertex3f(-w, -h, d);
            glVertex3f(w, -h, d);
            glVertex3f(w, h, d);
            glVertex3f(-w, h, d);
            glEnd();
            
            glBegin(GL_LINE_LOOP);
            glVertex3f(-w, -h, -d);
            glVertex3f(-w, -h, d);
            glVertex3f(-w, h, d);
            glVertex3f(-w, h, -d);
            glEnd();
            
            glBegin(GL_LINE_LOOP);
            glVertex3f(w, -h, -d);
            glVertex3f(w, -h, d);
            glVertex3f(w, h, d);
            glVertex3f(w, h, -d);
            glEnd();
            
            glBegin(GL_LINE_LOOP);
            glVertex3f(-w, h, -d);
            glVertex3f(w, h, -d);
            glVertex3f(w, h, d);
            glVertex3f(-w, h, d);
            glEnd();
            
            glBegin(GL_LINE_LOOP);
            glVertex3f(-w, -h, -d);
            glVertex3f(w, -h, -d);
            glVertex3f(w, -h, d);
            glVertex3f(-w, -h, d);
            glEnd();
            
//            glEnable(GL_LIGHTING);
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
	sprintf(strLine, "Cube Count %d", cubeCount);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "Texture Count %d", textureCount);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "X %f", camX);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "Y %f", camY);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "Z %f", camZ);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "pitch %f", pitch);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "yaw %f", yaw);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "roll %f", roll);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "AxisX %d", axisX);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "AxisY %d", axisY);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "AxisZ %d", axisZ);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "Constrain paste to original position %d", constrainPasteToOriginalPosition);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;

	memset(strLine, 0, 255);
	sprintf(strLine, "Constrain to texture dimensions %d", constrainToTextureDimensions);
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
	//memset(tmpstr, 0, 5);
//	sprintf(tmpstr, "AxisZ %d", axisZ);
	//strcat(strLine, tmpstr);
	drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
	textIndex++;
	
	if (cubeCount > 0)
	{
		textIndex++;
		memset(strLine, 0, 255);
		sprintf(strLine, "Selected Cube %d", selectionIndex);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	X %f", cubes[selectionIndex].x);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	Y %f", cubes[selectionIndex].y);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	Z %f", cubes[selectionIndex].z);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	Rotation X %f", cubes[selectionIndex].rX);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	Rotation Y %f", cubes[selectionIndex].rY);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	Rotation Z %f", cubes[selectionIndex].rZ);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;


		memset(strLine, 0, 255);
		sprintf(strLine, "	W %f", cubes[selectionIndex].w);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	H %f", cubes[selectionIndex].h);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	D %f", cubes[selectionIndex].d);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	Texture %d", cubes[selectionIndex].image);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;

		memset(strLine, 0, 255);
		sprintf(strLine, "	image filename %s", cubes[selectionIndex].imageFileName);
		drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
		textIndex++;
		
		if (editMode == MODE_TEXTURECOORDS)
		{

			for (uint8_t i = 0; i < 8; i ++)
			{
				memset(strLine, 0, 255);
				if (i == editSubMode)
					sprintf(strLine, "->Texture Coordinate %d: %.1f", i, cubes[selectionIndex].textureCoords[i]);
				else
					sprintf(strLine, " 	   Texture Coordinate %d: %.1f", i, cubes[selectionIndex].textureCoords[i]);
				drawText(10, glutGet(GLUT_WINDOW_HEIGHT) - 18 * textIndex, strLine);
				textIndex++;			
			}			
		}
	}
	glColor3f(1.0f, 1.0f, 1.0f);	
}

void saveData()
{
	FILE* file;

	if (saveCubes)
	{	
		file = fopen("cubes.dat", "wb");
		if (file == NULL)
		{
			printf("Error opening file %s to save data.", "cubes.dat");
		}
		size_t numWritten;
		fwrite(&cubeCount, sizeof(uint16_t), 1, file);

		for (uint16_t i = 0; i < cubeCount; i ++)
		{
	
			numWritten = fwrite(&cubes[i].x, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].y, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].z, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].w, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].h, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].d, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].rX, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].rY, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].rZ, sizeof(float), 1, file);
			numWritten = fwrite(&cubes[i].image, sizeof(uint16_t), 1, file);
			numWritten = fwrite(cubes[i].imageFileName, 64, 1, file);
			for (uint8_t ii = 0; ii < 8; ii ++)
			{
				numWritten = fwrite(&cubes[i].textureCoords[ii], sizeof(float), 1, file);
			}
		}
	//	fwrite(&cubes, sizeof(struct cube), cubeCount, file);
		fclose(file);
	}

	if (saveCfg)
	{
		file = fopen("config.dat", "wb");
		if (file == NULL)
		{
			printf("Error opening file %s to save data.", "config.dat");
		}
		fwrite(&axisX, sizeof(uint8_t), 1, file);
		fwrite(&axisY, sizeof(uint8_t), 1, file);
		fwrite(&axisZ, sizeof(uint8_t), 1, file);
		fwrite(&pitch, sizeof(float), 1, file);
		fwrite(&yaw, sizeof(float), 1, file);
		fwrite(&roll, sizeof(float), 1, file);
		fwrite(&camX, sizeof(float), 1, file);
		fwrite(&camY, sizeof(float), 1, file);
		fwrite(&camZ, sizeof(float), 1, file);
		fwrite(&selectionIndex, sizeof(uint16_t), 1, file);
		fwrite(&constrainPasteToOriginalPosition, sizeof(uint8_t), 1, file);
		fwrite(&editMode, sizeof(uint8_t), 1, file);
		fwrite(&editSubMode, sizeof(uint8_t), 1, file);
		fwrite(&constrainToTextureDimensions, sizeof(uint8_t), 1, file);		
		fclose(file);
	}
}

void loadData()
{
	FILE* file = fopen("cubes.dat", "rb");
		if (file == NULL)
	{
		printf("Error opening file to save data.");
	}
	if (file != NULL)
	{
		//fread(&cubes[0], sizeof(struct cube), cubeCount, file);	 
		fread(&cubeCount, sizeof(uint16_t), 1, file);
		for (uint16_t i = 0; i < cubeCount; i ++)
		{
			fread(&cubes[i].x, sizeof(float), 1, file);
			fread(&cubes[i].y, sizeof(float), 1, file);
			fread(&cubes[i].z, sizeof(float), 1, file);
			fread(&cubes[i].w, sizeof(float), 1, file);
			fread(&cubes[i].h, sizeof(float), 1, file);
			fread(&cubes[i].d, sizeof(float), 1, file);
			fread(&cubes[i].rX, sizeof(float), 1, file);
			fread(&cubes[i].rY, sizeof(float), 1, file);
			fread(&cubes[i].rZ, sizeof(float), 1, file);

			fread(&cubes[i].image, sizeof(uint16_t), 1, file);
			fread(cubes[i].imageFileName, 64, 1, file);
			for (uint8_t ii = 0; ii < 8; ii ++)
			{
				fread(&cubes[i].textureCoords[ii], sizeof(float), 1, file);
			}
			for (uint16_t ii = 0; ii < filenamesCount; ii ++)
			{
				if (!strcmp(filenames[ii], cubes[i].imageFileName))
					cubes[i].image = ii;
			}

		}
/*		for (uint16_t i = 0; i < cubeCount; i ++)
		{
			printf("%d\n", i);
			printf("x %f\n", cubes[i].x);
			printf("y %f\n", cubes[i].y);
			printf("z %f\n", cubes[i].z);
			printf("w %f\n", cubes[i].w);
			printf("h %f\n", cubes[i].h);
			printf("d %f\n", cubes[i].d);
			printf("rX %f\n", cubes[i].rX);
			printf("rY %f\n", cubes[i].rY);
			printf("rZ %f\n", cubes[i].rZ);

			printf("%s\n", cubes[i].imageFileName);
			for (uint8_t ii = 0; ii < 8; ii++)
			{
				printf(" texCoord[%d] %f\n", ii, cubes[i].textureCoords[ii]);
			}	
		}	
*/
		fclose(file);

		}
	file = fopen("config.dat", "rb");
	if (file == NULL)
	{
		printf("Error opening file %s to save data.", "config.dat");
	} else
	{
		fread(&axisX, sizeof(uint8_t), 1, file);
		fread(&axisY, sizeof(uint8_t), 1, file);
		fread(&axisZ, sizeof(uint8_t), 1, file);
		fread(&pitch, sizeof(float), 1, file);
		fread(&yaw, sizeof(float), 1, file);
		fread(&roll, sizeof(float), 1, file);
		fread(&camX, sizeof(float), 1, file);
		fread(&camY, sizeof(float), 1, file);
		fread(&camZ, sizeof(float), 1, file);
		fread(&selectionIndex, sizeof(uint16_t), 1, file);
		fread(&constrainPasteToOriginalPosition, sizeof(uint8_t), 1, file);
		fread(&editMode, sizeof(uint8_t), 1, file);
		fread(&editSubMode, sizeof(uint8_t), 1, file);
		fread(&constrainToTextureDimensions, sizeof(uint8_t), 1, file);
		fclose(file);		
	}
}

void setSaveCfg()
{
	if (!saveCfg) saveCfg = true;
}

void setSaveCubes()
{
	if (!saveCubes) saveCubes = true;
}

void deleteCube(uint16_t d_)
{
	if (d_ < cubeCount)
	{
		for (uint16_t i = d_; i < cubeCount; i ++)
		{
			cubes[i].x = cubes[i+1].x;
			cubes[i].y = cubes[i+1].y;
			cubes[i].z = cubes[i+1].z;
			cubes[i].w = cubes[i+1].w;
			cubes[i].h = cubes[i+1].h;
			cubes[i].d = cubes[i+1].d;
			cubes[i].rX = cubes[i+1].rX;
			cubes[i].rY = cubes[i+1].rY;
			cubes[i].rZ = cubes[i+1].rZ;
			cubes[i].image = cubes[i+1].image;
			memset(cubes[i].imageFileName, 0, 64);
			strcpy(cubes[i].imageFileName, cubes[i+1].imageFileName);
			for (uint8_t ii = 0; ii < 7; ii ++)
				cubes[i].textureCoords[ii] = cubes[i+1].textureCoords[ii];
		}
		cubeCount --;
	}
	if (selectionIndex >= cubeCount) 
	{
 		if (cubeCount > 0) selectionIndex = cubeCount - 1;
	}
}

void copyCube()
{
	if (selectionIndex >= 0 && selectionIndex < cubeCount)
	{
		cubeCopy.x = cubes[selectionIndex].x;
		cubeCopy.y = cubes[selectionIndex].y;
		cubeCopy.z = cubes[selectionIndex].z;
		cubeCopy.w = cubes[selectionIndex].w;
		cubeCopy.h = cubes[selectionIndex].h;
		cubeCopy.d = cubes[selectionIndex].d;
		cubeCopy.rX = cubes[selectionIndex].rX;
		cubeCopy.rY = cubes[selectionIndex].rY;
		cubeCopy.rZ = cubes[selectionIndex].rZ;
		cubeCopy.image = cubes[selectionIndex].image;
		memset(cubeCopy.imageFileName, 0, 64);
		strcpy(cubeCopy.imageFileName, cubes[selectionIndex].imageFileName);
		for (uint8_t i = 0; i < 7; i++)
			cubeCopy.textureCoords[i] = cubes[selectionIndex].textureCoords[i];
		copyBufferFull = true;
	}
}

void pasteCube()
{
	if (copyBufferFull)
	{
		if (constrainPasteToOriginalPosition) makeCube(cubeCopy.x,cubeCopy.y,cubeCopy.z); else
			makeCube(camX, camY, camZ);
		cubes[cubeCount-1].w = cubeCopy.w;
		cubes[cubeCount-1].h = cubeCopy.h;
		cubes[cubeCount-1].d = cubeCopy.d;
		cubes[cubeCount-1].rX = cubeCopy.rX;
		cubes[cubeCount-1].rY = cubeCopy.rY;
		cubes[cubeCount-1].rZ = cubeCopy.rZ;
		cubes[cubeCount-1].image = cubeCopy.image;
		memset(cubes[cubeCount-1].imageFileName, 0, 64);
		strcpy(cubes[cubeCount-1].imageFileName, cubeCopy.imageFileName);
		for (uint8_t i = 0; i < 7; i ++)
			cubes[cubeCount-1].textureCoords[i] = cubeCopy.textureCoords[i];
	}
}

void incValue()
{
	switch(editMode)
	{
		case MODE_NORMAL:

		break;
		case MODE_SCALE:
			if (axisX) cubes[selectionIndex].w += 0.1f * multiplier;
			if (axisY) cubes[selectionIndex].h += 0.1f * multiplier;
			if (axisZ) cubes[selectionIndex].d += 0.1f * multiplier;
			setSaveCubes();
		break;
		case MODE_MOVE:
			if (glutGetModifiers() & GLUT_ACTIVE_ALT)
			{
				if (axisX) cubes[selectionIndex].x += cubes[selectionIndex].w * multiplier;
				if (axisY) cubes[selectionIndex].y += cubes[selectionIndex].h * multiplier;
				if (axisZ) cubes[selectionIndex].z += cubes[selectionIndex].d * multiplier;							
			} else
			{
				if (axisX) cubes[selectionIndex].x += 0.1f * multiplier;
				if (axisY) cubes[selectionIndex].y += 0.1f * multiplier;
				if (axisZ) cubes[selectionIndex].z += 0.1f * multiplier;							
			}
			setSaveCubes();
		break;
		case MODE_ROTATE:
			if (axisX) 
			{
				cubes[selectionIndex].rX += 0.1f * multiplier;
				if (cubes[selectionIndex].rX > 360.0f) cubes[selectionIndex].rX = 0.0f;
					setSaveCubes();
			}
			if (axisY)
			{
				cubes[selectionIndex].rY += 0.1f * multiplier;
				if (cubes[selectionIndex].rY > 360.0f) cubes[selectionIndex].rY = 0.0f;
					setSaveCubes();
			} 
			if (axisZ)
			{
				cubes[selectionIndex].rZ += 0.1f * multiplier;
				if (cubes[selectionIndex].rZ > 360.0f) cubes[selectionIndex].rZ = 0.0f;
					setSaveCubes();
			}
		break;
		case MODE_TEXTURE:
			cubes[selectionIndex].image ++;
			if (cubes[selectionIndex].image == (textureCount)) 
				cubes[selectionIndex].image = 0;
			strcpy(cubes[selectionIndex].imageFileName, filenames[cubes[selectionIndex].image]);

			if (constrainToTextureDimensions)
			{
				if (axisX) cubes[selectionIndex].w = imageParams[cubes[selectionIndex].image].width * 0.001f;
				if (axisY) cubes[selectionIndex].h = imageParams[cubes[selectionIndex].image].height * 0.001f;
			}
			setSaveCubes();
		break;
		case MODE_TEXTURECOORDS:
			cubes[selectionIndex].textureCoords[editSubMode] += 0.1f;
			if (cubes[selectionIndex].textureCoords[editSubMode] > 1.0f)
				cubes[selectionIndex].textureCoords[editSubMode] = 0.0f;
			setSaveCubes();
		break;
	}
}

void decValue()
{
	switch(editMode)
	{
		case MODE_NORMAL:

		break;
		case MODE_SCALE:
			if (axisX) cubes[selectionIndex].w -= 0.1f * multiplier;
			if (axisY) cubes[selectionIndex].h -= 0.1f * multiplier;
			if (axisZ) cubes[selectionIndex].d -= 0.1f * multiplier;
				setSaveCubes();
		break;
		case MODE_MOVE:
			if (glutGetModifiers() & GLUT_ACTIVE_ALT)
			{
				if (axisX) cubes[selectionIndex].x -= cubes[selectionIndex].w * multiplier;
				if (axisY) cubes[selectionIndex].y -= cubes[selectionIndex].h * multiplier;
				if (axisZ) cubes[selectionIndex].z -= cubes[selectionIndex].d * multiplier;							
			} else
			{
				if (axisX) cubes[selectionIndex].x -= 0.1f * multiplier;
				if (axisY) cubes[selectionIndex].y -= 0.1f * multiplier;
				if (axisZ) cubes[selectionIndex].z -= 0.1f * multiplier;							
			}
			setSaveCubes();
		break;
		case MODE_ROTATE:
			if (axisX) 
			{
				cubes[selectionIndex].rX -= 0.1f * multiplier;
				if (cubes[selectionIndex].rX < 0.0f) cubes[selectionIndex].rX = 360.0f;
				setSaveCubes();
			}
			if (axisY)
			{
				cubes[selectionIndex].rY -= 0.1f * multiplier;
				if (cubes[selectionIndex].rY < 0.0f) cubes[selectionIndex].rY = 360.0f;
				setSaveCubes();
			} 
			if (axisZ)
			{
				cubes[selectionIndex].rZ -= 0.1f * multiplier;
				if (cubes[selectionIndex].rZ < 0.0f) cubes[selectionIndex].rZ = 360.0f;
				setSaveCubes();
			}
		break;
		case MODE_TEXTURE:

			if (cubes[selectionIndex].image > 0)
				cubes[selectionIndex].image --;
			else
				cubes[selectionIndex].image = textureCount - 1;

			memset(cubes[selectionIndex].imageFileName, 0, 64);
			strcpy(cubes[selectionIndex].imageFileName, filenames[cubes[selectionIndex].image]);
			if (constrainToTextureDimensions)
			{
				if (axisX) cubes[selectionIndex].w = imageParams[cubes[selectionIndex].image].width * 0.001f;
				if (axisY) cubes[selectionIndex].h = imageParams[cubes[selectionIndex].image].height * 0.001f;
			}
					
			setSaveCubes();
		break;
		case MODE_TEXTURECOORDS:
			cubes[selectionIndex].textureCoords[editSubMode] -= 0.1f;
			if (cubes[selectionIndex].textureCoords[editSubMode] < 0.0f)
				cubes[selectionIndex].textureCoords[editSubMode] = 1.0f;
			setSaveCubes();
		break;
	}

}

void mouseWheel(int button, int dir, int x, int y)
{
	if (dir > 0) incValue();
	if (dir < 0) decValue();
}
