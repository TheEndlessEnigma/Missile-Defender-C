/*
Missile Defender by Chris Carpenter

BSD LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#define E_RAD 0.065
#define RAD_TRIS 36
#define WIDTH 1000
#define HEIGHT 1000
#define M_SPD_INIT 8600
#define A_SPD 225
#define SLP_TIME 1800
#define LAG_LOOP 1500
#define M_SPD_MAX 1000
#define M_SPD_DECR 200
#include <stdio.h>
#include <stdlib.h>
#include <GL/freeglut.h>
#include <math.h>
#include <time.h>
#include <unistd.h> // UNIX library


typedef struct {
	double x, y, t;
	int inc, s;
} explosion;

typedef struct {
	double x, y, dx, dy, ox, oy, deltax, deltay;
} missile;

typedef struct {
	double x, y, dx, dy, ox, oy, deltax, deltay;
} hyp;

typedef struct {
	double x, y, dx, dy, ox, oy, deltax, deltay;
} abm;

typedef struct {
	double x, y, dx, dy, ox, oy, deltax, deltay;
	int shot;
} mirv;

typedef struct {
	double x, y, ox, dropx, deltax;
	int shot;
} bomber;

typedef struct {
	double x[9];
	double y[9];
	int t;
} targs;

int miscount = 0;
int mismax = 15;
int m_spd = M_SPD_INIT;
int m_screen = 3;
int pause_game = 0;
int end = 0;
int lvl = 0;
int ec=0, mc=0, ac=0, mic=0, hc=0, boc=0, bc=0;
int city[6] = {1, 1, 1, 1, 1, 1};
int silo[3] = {10, 10, 10};
double gameover_ticks = 1;
unsigned int score = 0;
double mx, my;//, bgr, bgg, bgb;
double current_mult = 1;
long lag = LAG_LOOP;
targs trg;
explosion e[128];
missile m[128];
abm a[32];
mirv mi[32];
hyp h[32];
bomber bo[8];

void shootTarg(int w, int h);

// sets the Target coordinates
void setTargs()
{
	int i;
	for(i=0; i<3; ++i) {
		trg.x[i] = -0.85 + i*0.85;
		trg.y[i] = -0.93;
		trg.x[i+3] = -0.6 + i*0.17;
		trg.y[i+3] = -0.96;
		trg.x[i+6] = 0.26 + i*0.17;
		trg.y[i+6] = -0.96;
	}
	trg.t = 9;
}

// removes a Target from the Target array
void remTarg(int index)
{
	int i;
	--trg.t;
	for(i=index; i<trg.t; ++i) {
		trg.x[i] = trg.x[i+1];
		trg.y[i] = trg.y[i+1];
	}
}

// adds an Explosion to the Explosion array
void addExplosion(double xval, double yval, int is_score)
{
	e[ec++] = (explosion){xval, yval, 1, 0, is_score};	
}

// adds a Bomber to the Bomber array
void addBomber(double origx, double yval, double drop_p)
{
	double delx = 0.00014*WIDTH;
	bo[boc++] = (bomber){origx, yval, origx, drop_p, delx, 0};	
}

// adds a Missile to the Missile array
void addMissile(double destx, double desty, double origx, double origy)
{
	double multiplier = sqrt((desty-origy)*(desty-origy)+(destx-origx)*(destx-origx))/(origy-desty);
	double delx = (destx-origx)/((origy-desty)/HEIGHT*m_spd)/multiplier;
	double dely = (desty-origy)/((origy-desty)/HEIGHT*m_spd)/multiplier;
	m[mc++] = (missile){origx, origy, destx, desty, origx, origy, delx, dely};
}

// adds a Missile to the Missile array
void addHyp(double destx, double desty, double origx, double origy)
{
	double multiplier = sqrt((desty-origy)*(desty-origy)+(destx-origx)*(destx-origx))/(origy-desty);
	double delx = 1.667*(destx-origx)/((origy-desty)/HEIGHT*m_spd)/multiplier;
	double dely = 1.667*(desty-origy)/((origy-desty)/HEIGHT*m_spd)/multiplier;
	h[hc++] = (hyp){origx, origy, destx, desty, origx, origy, delx, dely};
}

// adds a MIRV to the MIRV array
void addMirv(double destx, double desty, double origx, double origy)
{
	double multiplier = sqrt((desty-origy)*(desty-origy)+(destx-origx)*(destx-origx))/(origy-desty);
	double delx = (destx-origx)/((origy-desty)/HEIGHT*m_spd)/multiplier;
	double dely = (desty-origy)/((origy-desty)/HEIGHT*m_spd)/multiplier;
	mi[mic++] = (mirv){origx, origy, destx, desty, origx, origy, delx, dely, 0};
}

// adds an ABM to the ABM array
void addAbm(double destx, double desty, double origx, double origy)
{
	double multiplier = sqrt((desty-origy)*(desty-origy)+(destx-origx)*(destx-origx))/(origy-desty);
	double delx = (destx-origx)/((origy-desty)/HEIGHT*A_SPD)/multiplier;
	double dely = (desty-origy)/((origy-desty)/HEIGHT*A_SPD)/multiplier;
	a[ac++] = (abm){origx, origy, destx, desty, origx, origy, delx, dely};
}

// removes a Bomber from the Bomber array
void remBomber(int index)
{
	int i;
	--boc;
	for(i=index; i<boc; ++i)
		bo[i] = bo[i+1];
}

// removes an Explosion from the Explosion array
void remExplosion(int index)
{
	int i;
	--ec;
	for(i=index; i<ec; ++i)
		e[i] = e[i+1];
}

// removes a Missile from the Missile array
void remMissile(int index)
{
	int i;
	--mc;
	for(i=index; i<mc; ++i)
		m[i] = m[i+1];
}

// removes a MIRV from the MIRV array
void remMirv(int index)
{
	int i;
	--mic;
	for(i=index; i<mic; ++i)
		mi[i] = mi[i+1];
}

// removes an ABM from the ABM array
void remAbm(int index)
{
	int i;
	--ac;
	for(i=index; i<ac; ++i)
		a[i] = a[i+1];
}

// removes an ABM from the ABM array
void remHyp(int index)
{
	int i;
	--hc;
	for(i=index; i<hc; ++i)
		h[i] = h[i+1];
}

// updates the movement of objects
void update()
{
	int i, j;
	for(i=0; i<boc; ++i) {
		if(bo[i].x >= WIDTH+(0.05*WIDTH)) {
			remBomber(i--);
			continue;
		}
		if(!bo[i].shot && bo[i].x >= bo[i].dropx) {
			bo[i].shot = 1;
			addMissile(rand()%((int)WIDTH-(int)bo[i].x)+bo[i].x, (1 + -0.94)*HEIGHT/2, bo[i].x, bo[i].y);
		}
		bo[i].x += bo[i].deltax*current_mult;
	}
	// update ABM positions
	for(i=0; i<ac; ++i){
		if(a[i].dy <= a[i].y) {
			addExplosion(a[i].x, a[i].y, 1);
			remAbm(i--);
			continue;
		}
		a[i].x += a[i].deltax;
		a[i].y += a[i].deltay;
	}
	// update missile positions
	for(i=0; i<mc; ++i){
		if(m[i].dy >= m[i].y) {
			addExplosion(m[i].x, m[i].y, 0);
			remMissile(i--);
			continue;
		}
		m[i].x += m[i].deltax*current_mult;
		m[i].y += m[i].deltay*current_mult;
	}
	// update missile positions
	for(i=0; i<hc; ++i){
		if(h[i].dy >= h[i].y) {
			addExplosion(h[i].x, h[i].y, 0);
			remHyp(i--);
			continue;
		}
		h[i].x += h[i].deltax*current_mult;
		h[i].y += h[i].deltay*current_mult;
	}
	// update MIRV positions
	// fix slowdown when mirv appears
	for(i=0; i<mic; ++i){
		int j;
		if(mi[i].dy >= mi[i].y && !mi[i].shot) {
			mi[i].shot = 1;
			mi[i].x = mi[i].dx;
			mi[i].y = mi[i].dy;
			for(j=0; j<3; ++j)
				shootTarg(mi[i].dx, mi[i].dy);
			continue;
		}
		if(mi[i].shot) {
			remMirv(i--);
		} else {
			mi[i].x += mi[i].deltax*current_mult;
			mi[i].y += mi[i].deltay*current_mult;
		}
	}
	// update explosions, explode things caught in explosions
	for(i=0; i<ec; ++i) {
		if(e[i].t >= 5) {
			remExplosion(i--);
			continue;
		}
		double x2 = 2*e[i].x/WIDTH-1;
		double y2 = 2*e[i].y/HEIGHT-1;

		for(j=0; j<boc; ++j) {
			double x1 = (2*bo[j].x/WIDTH-1)-0.01;
			double y1 = 2*bo[j].y/HEIGHT-1;
			double x1_23 = (2*bo[j].x/WIDTH-1)-0.04;
			double y1_2 = (2*bo[j].y/HEIGHT-1)+0.04;
			double y1_3 = (2*bo[j].y/HEIGHT-1)-0.04;
			double dist1 = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
			double dist2 = sqrt((x2-x1_23)*(x2-x1_23)+(y2-y1_2)*(y2-y1_2));
			double dist3 = sqrt((x2-x1_23)*(x2-x1_23)+(y2-y1_3)*(y2-y1_3));
			if(dist1 <= E_RAD/e[i].t || dist2 <= E_RAD/e[i].t || dist3 <= E_RAD/e[i].t) {
				if(e[i].s) {
					if(bo[j].shot)
						score += (e[i].s+1)*100;
					else
						score += (e[i].s+1)*300;
					addExplosion(bo[j].x, bo[j].y, e[i].s+1);
				} else {
					addExplosion(bo[j].x, bo[j].y, 0);
				}
				remBomber(j--);
			}
		}
		for(j=0; j<mic; ++j) {
			double x1 = 2*mi[j].x/WIDTH-1;
			double y1 = 2*mi[j].y/HEIGHT-1;
			double dist = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
			if(dist <= E_RAD/e[i].t) {
				if(e[i].s) {
					score += (e[i].s+1)*500;
					addExplosion(mi[j].x, mi[j].y, e[i].s+1);
				} else {
					addExplosion(mi[j].x, mi[j].y, 0);
				}
				remMirv(j--);
			}
		}
		for(j=0; j<mc; ++j) {
			double x1 = 2*m[j].x/WIDTH-1;
			double y1 = 2*m[j].y/HEIGHT-1;
			double dist = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
			if(dist <= E_RAD/e[i].t) {
				if(e[i].s) {
					score += (e[i].s+1)*100;
					addExplosion(m[j].x, m[j].y, e[i].s+1);
				} else {
					addExplosion(m[j].x, m[j].y, 0);
				}
				remMissile(j--);
			}
		}
		for(j=0; j<hc; ++j) {
			double x1 = 2*h[j].x/WIDTH-1;
			double y1 = 2*h[j].y/HEIGHT-1;
			double dist = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
			if(dist <= E_RAD/e[i].t) {
				if(e[i].s) {
					score += (e[i].s+1)*300;
					addExplosion(h[j].x, h[j].y, e[i].s+1);
				} else {
					addExplosion(h[j].x, h[j].y, 0);
				}
				remHyp(j--);
			}
		}
		for(j=0; j<trg.t; ++j) {
			double dist = sqrt((x2-trg.x[j])*(x2-trg.x[j])+(y2-trg.y[j])*(y2-trg.y[j]));
			if(dist <= E_RAD/e[i].t) {
				int k = (int)(100*trg.x[j]);
				if(k == -85)
					silo[0] = 0;
				else if(k == 0)
					silo[1] = 0;
				else if(k == 85)
					silo[2] = 0;
				else if(k == -60)
					city[0] = 0;
				else if(k == -42)
					city[1] = 0;
				else if(k == -25)
					city[2] = 0;
				else if(k == 26)
					city[3] = 0;
				else if(k == 43)
					city[4] = 0;
				else if(k == 60)
					city[5] = 0;
				remTarg(j--);
			}
		}
		if(!e[i].inc && e[i].t < 0.7)
			e[i].inc = 1;
		if(!e[i].inc && e[i].t <= 1)
			e[i].t -= 0.0015;
		else if(e[i].t < 1.1)
			e[i].t += 0.0015;
		else if(e[i].t < 2)
			e[i].t += 0.025;
		else
			e[i].t += 0.06;
	}
}

// draws a triangle
void drawTri(double x1, double y1, double x2, double y2, double x3, double y3)
{
	glBegin(GL_TRIANGLES);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glVertex2f(x3, y3);
	glEnd();
}

// draws a bomber
void drawBomber(double x1, double y1)
{
	glColor3f(0, 0, 1);
	drawTri(x1, y1, x1-0.05, y1-0.05, x1-0.05, y1+0.05);
	glColor3f(0, 0, 0);
	drawTri(x1-0.05, y1, x1-0.05, y1-0.012, x1-0.036, y1-0.006);
	drawTri(x1-0.05, y1, x1-0.05, y1+0.012, x1-0.036, y1+0.006);
	drawTri(x1-0.05, y1+0.012, x1-0.05, y1+0.040, x1-0.034, y1+0.026);
	drawTri(x1-0.05, y1-0.012, x1-0.05, y1-0.040, x1-0.034, y1-0.026);
	drawTri(x1-0.05, y1+0.04, x1-0.05, y1+0.05, x1-0.0395, y1+0.045);
	drawTri(x1-0.05, y1-0.04, x1-0.05, y1-0.05, x1-0.0395, y1-0.045);
}

// draws a rectangle
void drawRect(double x, double y, double xlength, double ylength)
{
	drawTri(x, y, x+xlength, y, x, y+ylength);
	drawTri(x+xlength, y, x, y+ylength, x+xlength, y+ylength);
}

// draw game over
void drawGameOver(double x, double y)
{

	double two_pi = 2.0 * 3.14159265;

	int color = rand()%3;
	if(color == 0)
		glColor3f(1.0, 0.0, 0.0);
	else if(color == 1)
		glColor3f(0.0, 1.0, 0.0);
	else
		glColor3f(0.0, 0.0, 1.0);
	double x1 = 0;
	double y1 = 0;
	if(gameover_ticks>=0.0015)
		gameover_ticks -= 0.0015;
	else
		glColor3f(1.0, 1.0, 1.0);
	double rad = E_RAD/gameover_ticks;
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(x1, y1);
	int j;
	for(j=0; j<=RAD_TRIS; ++j)
		glVertex2f((x1+(rad*cos(j*two_pi/RAD_TRIS))), (y1+(rad*sin(j*two_pi/RAD_TRIS))));
	glEnd();

	glColor3f(1.0, 0.0, 0.0);

	// T
	drawRect(x, y+0.2-0.025, 0.1, 0.025);
	drawRect(x+0.05-0.0125, y, 0.025, 0.2);
	
	// H
	drawRect(x+0.1+0.025, y, 0.025, 0.2);
	drawRect(x+0.2, y, 0.025, 0.2);
	drawRect(x+0.1+0.025, y+0.1-0.0125, 0.1, 0.025);

	// E
	drawRect(x+0.2+0.05, y, 0.025, 0.2);
	drawRect(x+0.2+0.05, y+0.2-0.025, 0.1, 0.025);
	drawRect(x+0.2+0.05, y+0.1-0.0125, 0.1, 0.025);
	drawRect(x+0.2+0.05, y, 0.1, 0.025);

	// E
	drawRect(x+0.4+0.075, y, 0.025, 0.2);
	drawRect(x+0.4+0.075, y+0.2-0.025, 0.1, 0.025);
	drawRect(x+0.4+0.075, y+0.1-0.0125, 0.1, 0.025);
	drawRect(x+0.4+0.075, y, 0.1, 0.025);

	// n
	drawRect(x+0.6, y, 0.025, 0.1);
	drawRect(x+0.6, y+0.1-0.0125, 0.1, 0.025);
	drawRect(x+0.7-0.025, y, 0.025, 0.1);

	// d
	drawRect(x+0.7+0.025, y, 0.025, 0.1);
	drawRect(x+0.8, y, 0.025, 0.2);
	drawRect(x+0.7+0.025, y, 0.1, 0.025);
	drawRect(x+0.7+0.025, y+0.1-0.0125, 0.1, 0.025);

}

// draws an on-ground ABM at float (-1.0,1.0) coordinates
void drawAbm(double x, double y)
{
	drawTri(0+x, 0+y, -0.005+x, -0.005+y, -0.005+x, 0.01+y);
	drawTri(0+x, 0+y, 0.005+x, -0.005+y, 0.005+x, 0.01+y);
	drawTri(0+x, 0+y, -0.005+x, 0.01+y, 0.005+x, 0.01+y);
	drawTri(0.005+x, 0.01+y, -0.005+x, 0.01+y, 0+x, 0.02+y);
}

// draws a destroyed city at float (-1.0,1.0) coordinates
void drawRubble(double x, double y)
{
	glColor3f(1.0, 0, 0);
	drawTri(-0.075/2+x, 0+y, -0.025/2+x, 0+y, -0.05/2+x, 0.05/2+y);
	drawTri(-0.025/2+x, 0+y, 0.025/2+x, 0+y, 0+x, 0.15/2+y);
	drawTri(0.025/2+x, 0+y, 0.05/2+x, 0.1/2+y, 0.075/2+x, 0+y);
	glEnd();
}

// draws a city at float (-1.0,1.0) coordinates
void drawCity(double x, double y)
{
	glColor3f(0, 0, 1.0);
	drawTri(-0.075/2+x, 0+y, -0.075/2+x, 0.1/2+y, -0.025/2+x, 0+y);
	drawTri(-0.075/2+x, 0.1/2+y, -0.025/2+x, 0+y, -0.025/2+x, 0.1/2+y);
	drawTri(-0.025/2+x, 0+y, 0.025/2+x, 0+y, -0.025/2+x, 0.15/2+y);
	drawTri(-0.025/2+x, 0.15/2+y, 0.025/2+x, 0+y, 0.025/2+x, 0.15/2+y);
	drawTri(0.025/2+x, 0+y, 0.025/2+x, 0.05/2+y, 0.075/2+x, 0+y);
	drawTri(0.025/2+x, 0.05/2+y, 0.075/2+x, 0.05/2+y, 0.075/2+x, 0+y);
}

// draws a hill, called by draw function
void drawHill(int hill)
{
	glColor3f(0, 0, 1.0);
	if(silo[hill] >= 1)
		drawAbm(-0.85+hill*0.85, -0.88);
	if(silo[hill] >= 2)
		drawAbm(-0.87+hill*0.85, -0.9);
	if(silo[hill] >= 3)
		drawAbm(-0.83+hill*0.85, -0.9);
	int i;
	for(i=0; i<3; ++i)
		if(silo[hill] >= 4+i)
			drawAbm(-0.89+(hill*0.85)+(0.04*i), -0.92);
	for(i=0; i<4; ++i)
		if(silo[hill] >= 7+i)
			drawAbm(-0.91+(hill*0.85)+(0.04*i), -0.94);
}

// draws all cities, called by draw function
void drawCities()
{
	glColor3f(0, 0, 1.0);
	int i;
	for(i=0; i<3; ++i) {
		if(city[i])
			drawCity(-0.6+0.17*i, -0.97);
		else
			drawRubble(-0.6+0.17*i, -0.97);
		if(city[i+3])
			drawCity(0.26+0.17*i, -0.97);
		else
			drawRubble(0.26+0.17*i, -0.97);
	}
}

// prints text
void printText(char *txt, double x, double y)
{
	char buf[64];
	snprintf(buf, sizeof buf, "%s", txt);
	glRasterPos2f(x, y);
	glutBitmapString(GLUT_BITMAP_9_BY_15, buf);
}

// draw function, called by idle function
void draw()
{
	double x1, x2, y1, y2;
	glClearColor(0, 0, 0, 0.0);
 glClear(GL_COLOR_BUFFER_BIT);
 glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	int i;
	glColor3f(0, 1.0, 0);
	drawTri(-1, -1, 1, -1, 1, -0.96);
	drawTri(-1, -1, -1, -0.96, 1, -0.96);
	drawTri(-1, -0.96, -0.85, -0.85, -0.7, -0.96);
	drawTri(-0.15, -0.96, 0, -0.85, 0.15, -0.96);
	drawTri(0.7, -0.96, 0.85, -0.85, 1, -0.96);
	for(i=0; i<3; ++i)
		drawHill(i);
	drawCities();
	char buf[32];
    glColor3f(1.0, 0, 0);
	if(lag > 0 && lvl > 0) {
		--lag;
		snprintf(buf, sizeof buf, "%s%7d", "LEVEL ", lvl);
		glRasterPos2f(-0.14,0);
		glutBitmapString(GLUT_BITMAP_9_BY_15, buf);
		if(lvl == 3) {
			printText("MIRV ICBMS INBOUND!", -0.16, -0.1);
		} else if(lvl == 4) {
			printText("HYPERSONIC CRUISE MISSILES INBOUND!",-0.3,-0.1);
		} else if(lvl == 5) {
			printText("BOMBERS INBOUND!",-0.17,-0.1);
		}
	} else if(lvl == 0) {
		printText("MISSILE COMMAND",-0.145,-0.05);
	}
	snprintf(buf, sizeof buf, "%s%d", "LEVEL ", lvl);
	glRasterPos2f(0.75,-0.995);
	glutBitmapString(GLUT_BITMAP_9_BY_15, buf);
	snprintf(buf, sizeof buf, "%s%7d", "SCORE: ", score);
	glRasterPos2f(-0.15,-0.995);
	glutBitmapString(GLUT_BITMAP_9_BY_15, buf);
	if(end)
		drawGameOver(-0.45, 0);
	for(i=0; i<mc; ++i) { // draw missile lines
		glColor3f(1.0, 0, 0);
		x1 = 2*m[i].ox / WIDTH - 1;
		y1 = 2*m[i].oy / HEIGHT - 1;
		x2 = 2*m[i].x / WIDTH - 1;
		y2 = 2*m[i].y / HEIGHT - 1;
		glBegin(GL_LINES);
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
		glEnd();
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_POINTS);
		glVertex2f(x2, y2);
		glEnd();
	}
	for(i=0; i<boc; ++i) {
		x1 = 2*bo[i].x / WIDTH - 1;
		y1 = 2*bo[i].y / HEIGHT - 1;
		drawBomber(x1, y1);
	}
	for(i=0; i<mic; ++i) { // draw MIRV lines
		glColor3f(0, 1, 0);
		x1 = 2*mi[i].ox / WIDTH - 1;
		y1 = 2*mi[i].oy / HEIGHT - 1;
		x2 = 2*mi[i].x / WIDTH - 1;
		y2 = 2*mi[i].y / HEIGHT - 1;
		glBegin(GL_LINES);
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
		glEnd();
		glColor3f(1.0, 0, 0);
		glBegin(GL_POINTS);
		glVertex2f(x2, y2);
		glEnd();
	}
	for(i=0; i<ac; ++i) { // draw ABM lines
		x1 = 2*a[i].ox / WIDTH - 1;
		y1 = 2*a[i].oy / HEIGHT - 1;
		x2 = 2*a[i].x / WIDTH - 1;
		y2 = 2*a[i].y / HEIGHT - 1;
		double tmpx = 2*a[i].dx / WIDTH - 1;;
		double tmpy = 2*a[i].dy / HEIGHT - 1;
		double x3 = tmpx + 0.01;
		double y3 = tmpy + 0.01;
		double x4 = tmpx - 0.01;
		double y4 = tmpy + 0.01;
		double x5 = tmpx - 0.01;
		double y5 = tmpy - 0.01;
		double x6 = tmpx + 0.01;
		double y6 = tmpy - 0.01;
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_LINES);
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
		int store = rand()%3;
		if(store == 2)
			glColor3f(1.0, 0, 0);
		else if(store == 1)
			glColor3f(0,1,0);
		else
			glColor3f(0,0,1);
		glBegin(GL_LINES);
		glVertex2f(x3, y3);
		glVertex2f(x5, y5);
		glEnd();
		glBegin(GL_LINES);
		glVertex2f(x4, y4);
		glVertex2f(x6, y6);
		glEnd();
		glColor3f(0, 0, 1.0);
		glBegin(GL_POINTS);
		glVertex2f(x2, y2);
		glEnd();
	}
	for(i=0; i<hc; ++i) { // draw ABM lines
		glColor3f(0, 0, 1.0);
		x1 = 2*h[i].ox / WIDTH - 1;
		y1 = 2*h[i].oy / HEIGHT - 1;
		x2 = 2*h[i].x / WIDTH - 1;
		y2 = 2*h[i].y / HEIGHT - 1;
		glBegin(GL_LINES);
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
		glEnd();
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_POINTS);
		glVertex2f(x2, y2);
		glEnd();
	}
	double two_pi = 2.0 * 3.14159265;
	for(i=0; i<ec; ++i) { // draw explosions
		int color = rand()%3;
		if(color == 0)
			glColor3f(1.0, 0.0, 0.0);
		else if(color == 1)
			glColor3f(0.0, 1.0, 0.0);
		else
			glColor3f(0.0, 0.0, 1.0);
		x1 = 2*e[i].x / WIDTH - 1;
		y1 = 2*e[i].y / HEIGHT -1;
		double rad = E_RAD/e[i].t;
		glBegin(GL_TRIANGLE_FAN);
		glVertex2f(x1, y1);
		int j;
		for(j=0; j<=RAD_TRIS; ++j)
			glVertex2f((x1+(rad*cos(j*two_pi/RAD_TRIS))), (y1+(rad*sin(j*two_pi/RAD_TRIS))));
		glEnd();
	}
 glFlush();
}

// shoots missiles at player targets
void shootTarg(int w, int h)
{
	if(trg.t > 0) {
		int randn = rand() % trg.t;
		addMissile((1 + trg.x[randn])*WIDTH/2, (1 + trg.y[randn])*HEIGHT/2, w, h);
	}
}

// shoots hypersonic missile
void shootHyp(int w, int h)
{
	if(trg.t > 0) {
		int randn = rand() % trg.t;
		addHyp((1 + trg.x[randn])*WIDTH/2, (1 + trg.y[randn])*HEIGHT/2, w, h);
	}
}

// shoots mirv
void shootMirv() 
{
	int top_h = (3*HEIGHT)/5;
	int bot_h = (2*HEIGHT)/5;
	addMirv(((0.15*WIDTH)-1)+0.7*(rand()%WIDTH), bot_h + rand()%(top_h-bot_h), rand()%WIDTH, HEIGHT-1);
}

// reset game
void reset()
{
	score = 0;
	end = 0;
	lvl = 1;
	m_screen = 3;
	m_spd = M_SPD_INIT;
	miscount = 0;
	mismax = 15;
	current_mult = 1;
	gameover_ticks = 1;
	int i;
	for(i=0; i<3; ++i)
		silo[i] = 10;
	for(i=0; i<6; ++i)
		city[i] = 1;
	setTargs();
}

// function that controls AI
void ai()
{
	int cityval = city[0]+city[1]+city[2]+city[3]+city[4]+city[5];
	if(mc<m_screen && miscount<mismax && (cityval)>0) {
		if(lvl>4 && miscount % 14 == 0)
			addBomber(-(0.05*WIDTH), rand()%(int)((HEIGHT-(0.4*HEIGHT)))+0.3*HEIGHT, rand()%(int)(0.9*WIDTH)+0.1*WIDTH);
		else if(lvl>2 && mic == 0)
			shootMirv();
		else if(lvl>3 && miscount % 15 == 0)
			shootHyp(rand()%WIDTH, HEIGHT-1);
		else
			shootTarg(rand()%WIDTH, HEIGHT-1);
		++miscount;
	} else if(miscount==mismax && (cityval)>0 && (mc+ec+hc+mic+boc)==0) {
		glClearColor(0.0, 0.0, 0.0, 0.0);
    	glClear(GL_COLOR_BUFFER_BIT);
		if(m_spd > M_SPD_MAX) {
			if(lvl<5 || lvl%3==0)
				++m_screen;
			m_spd -= M_SPD_DECR;
			if(lvl<10 || lvl%2==0)
				++mismax;
		}
		++lvl;
		current_mult = ((double)M_SPD_INIT/m_spd);
		miscount = 0;
		int i;
		for(i=0; i<3; ++i)
			silo[i] = 10;
		setTargs();
		int j=0;
		for(i=0; i<6; ++i)
			if(city[i]==0) {
				remTarg(i-j+3);
				++j;
			}

		lag = LAG_LOOP;
	} else if((cityval)==0) {
		current_mult = ((double)M_SPD_INIT/600);
		end = 1;
	}
}

// get mouse input, launch ABMs from the proper silos
void handleMouse(int button, int state, int x, int y)
{
	glutGetModifiers();
	if(silo[0]+silo[1]+silo[2] > 0) {
		if(button == GLUT_LEFT_BUTTON) {
			if(state == GLUT_DOWN) {
				if(lvl == 0) {
					++lvl;
				} else if(pause_game) {
					pause_game = 0;
				} else {
					mx = x;
					my = HEIGHT-y;
					double silos[3] = {(1 + -0.85)*WIDTH/2, (1 + 0)*WIDTH/2, (1 + 0.85)*WIDTH/2};
					double val[3] = {silos[0]-x, silos[1]-x, silos[2]-x};
					int i;
					for(i=0; i<3; ++i)
						if(val[i] < 0)
							val[i]*=-1;
					int choice[3];
					if(val[0] <= val[1] && val[1] <= val[2])
						choice[0]=0, choice[1]=1, choice[2]=2;
					else if(val[2] <= val[1] && val[1] <= val[0])
						choice[0]=2, choice[1]=1, choice[2]=0;
					else if(val[1] <= val[0] && val[0] <= val[2])
						choice[0]=1, choice[1]=0, choice[2]=2;
					else if(val[2] <= val[0] && val[0] <= val[1])
						choice[0]=2, choice[1]=0, choice[2]=1;
					else if(val[1] <= val[2] && val[2] <= val[0])
						choice[0]=1, choice[1]=2, choice[2]=0;
					else if(val[0] <= val[2] && val[2] <= val[1])
						choice[0]=0, choice[1]=2, choice[2]=1;
					i=0;
					while(silo[choice[i]] == 0 && i<3)
						++i;
					if(silo[choice[i]]>0) {
						addAbm(mx, my, silos[choice[i]], (1 + -0.85)*HEIGHT/2);
						--silo[choice[i]];
					}
				}	
			}
		} else if(button == GLUT_RIGHT_BUTTON) {
			if(state == GLUT_DOWN) {
				if(pause_game == 0)
					pause_game = 1;
				else
					pause_game = 0;
			}
		}
	} else if(end && ec==0 && mc ==0) {
		if(button == GLUT_LEFT_BUTTON) {
			if(state == GLUT_DOWN) {
				reset();
			}
		}
	}
}

// idle function for AI, update, and draw 
void idle()
{
	if(!pause_game && lag==0)
		ai();
	int i;
	for(i=0; i<180; ++i){
		if(!pause_game)
			update();
		draw();
		usleep(SLP_TIME);
	}
}

// main
int main(int argc, char** argv) 
{
	srand(time(NULL));
	trg.t = 0;
	mx = WIDTH/2.0;
	my = HEIGHT/2.0;
	setTargs();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Missile Defender");
	glutDisplayFunc(idle);
	glutMouseFunc(handleMouse);
	glutIdleFunc(idle);
	glutMainLoop();
	return 1;
}
