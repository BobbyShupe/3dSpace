#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>


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

struct Motion
{
    bool Forward,Backward,Left,Right,Up,Down;
};

struct cube
{
	float x,y,z,w,h,d;
	uint16_t image;	
};

struct Motion motion = {false,false,false,false,false,false};

struct cube* cubes;
uint16_t cubeCount = 0;
struct cube newCube;

void makeCube(int16_t,int16_t,int16_t);
void drawCubes();
void init()
{
    glutSetCursor(GLUT_CURSOR_NONE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glutWarpPointer(width/2,height/2);
}

int main(int argc,char**argv)
{
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

    glutMainLoop();
    return 0;
}

/* This function just draws the scene. I used Texture mapping to draw
   a chessboard like surface. If this is too complicated for you ,
   you can just use a simple quadrilateral */

void draw()
{
    glEnable(GL_TEXTURE_2D);
    GLuint texture;
    glGenTextures(1,&texture);

    unsigned char texture_data[2][2][4] =
                    {
                        0,0,0,255,  255,255,255,255,
                        255,255,255,255,    0,0,0,255
                    };

    glBindTexture(GL_TEXTURE_2D,texture);
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

    glutSwapBuffers();
}

void reshape(int w,int h)
{
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60,16.0/9.0,1,7500);
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
    yaw+=(float)dev_x/10.0;
    pitch+=(float)dev_y/10.0;
}

void camera()
{

    if(motion.Forward)
    {
        camX += cos((yaw+90)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90)*TO_RADIANS)/5.0;
    }
    if(motion.Backward)
    {
        camX += cos((yaw+90+180)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90+180)*TO_RADIANS)/5.0;
    }
    if(motion.Left)
    {
        camX += cos((yaw+90+90)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90+90)*TO_RADIANS)/5.0;
    }
    if(motion.Right)
    {
        camX += cos((yaw+90-90)*TO_RADIANS)/5.0;
        camZ -= sin((yaw+90-90)*TO_RADIANS)/5.0;
    }
	if(motion.Up)
	{
	    camY += 1/5.0; // Move up
	}
	if(motion.Down)
	{
	    camY -= 1/5.0; // Move down
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
    }
}

void makeCube(int16_t _x, int16_t _y, int16_t _z)
{
	newCube.x = _x; newCube.y = _y; newCube.z = _z;
	cubes[cubeCount].x = newCube.x;
	cubes[cubeCount].y = newCube.y;
	cubes[cubeCount].z = newCube.z;	
	cubeCount ++;
}

void drawCubes()
{
	for (uint16_t i = 0; i < cubeCount; i ++)
	{
		glPushMatrix();
		glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
                                  // Top face (y = 1.0f)
                                  // Define vertices in counter-clockwise (CCW) order with normal pointing out
glColor3f(0.0f, 1.0f, 0.0f);     // Green
glVertex3f(1.0f, 1.0f, -1.0f);
glVertex3f(-1.0f, 1.0f, -1.0f);
glVertex3f(-1.0f, 1.0f, 1.0f);
glVertex3f(1.0f, 1.0f, 1.0f);

// Bottom face (y = -1.0f)
glColor3f(1.0f, 0.5f, 0.0f);     // Orange
glVertex3f(1.0f, -1.0f, 1.0f);
glVertex3f(-1.0f, -1.0f, 1.0f);
glVertex3f(-1.0f, -1.0f, -1.0f);
glVertex3f(1.0f, -1.0f, -1.0f);

// Front face  (z = 1.0f)
glColor3f(1.0f, 0.0f, 0.0f);     // Red
glVertex3f(1.0f, 1.0f, 1.0f);
glVertex3f(-1.0f, 1.0f, 1.0f);
glVertex3f(-1.0f, -1.0f, 1.0f);
glVertex3f(1.0f, -1.0f, 1.0f);

// Back face (z = -1.0f)
glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
glVertex3f(1.0f, -1.0f, -1.0f);
glVertex3f(-1.0f, -1.0f, -1.0f);
glVertex3f(-1.0f, 1.0f, -1.0f);
glVertex3f(1.0f, 1.0f, -1.0f);

// Left face (x = -1.0f)
glColor3f(0.0f, 0.0f, 1.0f);     // Blue
glVertex3f(-1.0f, 1.0f, 1.0f);
glVertex3f(-1.0f, 1.0f, -1.0f);
glVertex3f(-1.0f, -1.0f, -1.0f);
glVertex3f(-1.0f, -1.0f, 1.0f);

// Right face (x = 1.0f)
glColor3f(1.0f, 0.0f, 1.0f);     // Magenta
glVertex3f(1.0f, 1.0f, -1.0f);
glVertex3f(1.0f, 1.0f, 1.0f);
glVertex3f(1.0f, -1.0f, 1.0f);
glVertex3f(1.0f, -1.0f, -1.0f);
glEnd();  // End of drawing color-cube

	glPopMatrix();
	}
	glColor3f(1.0f, 1.0f, 1.0f);
}
