/* ARVoxelEditor - Augmented Reality Voxel Editor
 * Copyright (C) 2015 Santiago Sánchez Sobrino
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "functions.h"

#include <GL/glut.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include "colours.h"
#include "structs.h"

ARMultiMarkerInfoT *mMarker;
int dim[2];
int voxel_size = 16;
int grid_height = PAPER_HEIGHT / 16;
int grid_width = PAPER_WIDTH / 16;
struct TObject *objects = NULL;
int n_objects = 0;
struct TVoxel *voxels = NULL;
int n_voxels = 0;
int n_voxels_non_dirty = 0;
struct TBrush brush;
unsigned char colour_index = BLACK;
int n_colours = 0;
int is_input = 0;
char character[2] = " \0";

int roundNum(float num) {
  return num < 0 ? num - 0.5 : num + 0.5;
}

void menu() {
  char buff[200];

  int num_real_voxels = n_voxels - n_voxels_non_dirty;
  sprintf(buff,
          "Colour: %s (%u, %u, %u)\n"
          "Num. of voxels: %d\n"
          "Num. of colours: %d\n",
          brush.colour->name, brush.colour->r, brush.colour->g, brush.colour->b,
          num_real_voxels < 0 ? 0 : num_real_voxels, n_colours);
  printText(1.0f, 1.0f, 1.0f,  10, 14,  GLUT_BITMAP_HELVETICA_12,  buff,  1);

  sprintf(buff,
          "Q: Quit\n"
          "-/+: Change colour\n"
          "R: Reset\n"
          "U: Undo\n"
          "ENTER: Command line\n"
          "SPACEBAR: Put voxel\n"
          "X: Remove voxel\n");
  printText(1.0f, 1.0f, 1.0f,  10, 14,  GLUT_BITMAP_HELVETICA_12, buff, 0);

  if(is_input)
    input();
}

void keyboard(unsigned char key, int x, int y) {
  if(is_input) {
    character[0] = key;
    return;
  }

  switch(key) {
  case 0x1B: case 'Q': case 'q': cleanup(); break;
  case '-':
    colour_index = (colour_index>0)?colour_index-1:COLOURS_LENGTH-1;
    brush.colour = &colours[colour_index];
    break;
  case '+':
    colour_index = (colour_index<COLOURS_LENGTH-1)?colour_index+1:0;
    brush.colour = &colours[colour_index];
    break;
  case 'R': case 'r': cleanCanvas(); break;
  case 'U': case 'u': removeLastVoxel(); break;
  case 'X': case 'x': brush.remove_voxel = 1; break;
  case ' ': brush.put_voxel = 1; break;
  case 0xD: is_input = 1; break;
  }
}

void input() {
  static char buff[1024];
  static int first_enter = 1;

  printText(0.0f, 1.0f, 0.0f, 164, 14, GLUT_BITMAP_HELVETICA_10, ">", 0);

  // CWD
  if(character[0] == '^') {
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) != NULL) {
      strcat(buff, cwd);
      character[0] = '\0';
    }
    else
      fprintf(stderr, "getcwd() error.\n");
  }

  // BACKSPACE
  if(character[0] == '\b') {
    buff[strlen(buff)-1] = '\0';
  }
  else if(character[0] != '\0') {
    strcat(buff, character);
  }

  printText(0.0f, 1.0f, 0.0f, 174, 14, GLUT_BITMAP_HELVETICA_10, buff, 0);

  // NULL
  if(character[0] == '\0')
    return;

  // ENTER
  if(first_enter) {
    character[0] = '\0';
    first_enter = 0;
    return;
  }
  if(character[0] == 0xD) {
    is_input = 0;
    first_enter = 1;

    char command[24];
    sscanf(buff, "%s", command);
    printf("%s\n", command);
    if(strcmp(command, "load") == 0) {
      char arg1[64];
      sscanf(buff, "%*s %s", arg1);
      printf("%s\n", arg1);
      loadModel(arg1);
    }
    else if(strcmp(command, "save") == 0) {
      char arg1[64];
      sscanf(buff, "%*s %s", arg1);
      printf("%s\n", arg1);
      saveModel(arg1);
    }

    buff[0] = '\0';
    return;
  }

  character[0] = '\0';
}

void addObject(char *p, int patt_id, double w, double c[2], void (*draw)(void)) {
  if((patt_id=arLoadPatt(p)) < 0)
    ERROR("Error loading pattern");

  n_objects++;
  objects = (struct TObject*)realloc(objects, sizeof(struct TObject)*n_objects);

  objects[n_objects-1].id = patt_id;
  objects[n_objects-1].width = w;
  objects[n_objects-1].center[0] = c[0];
  objects[n_objects-1].center[1] = c[1];
  objects[n_objects-1].draw = draw;
}

void addVoxel(struct TColour* colour, int x, int y, int z) {
  int half_voxel_size = voxel_size / 2;
  int cx = x * voxel_size + half_voxel_size;
  int cy = y * voxel_size - half_voxel_size;
  int cz = z * voxel_size + half_voxel_size;

  int populated = isPopulated(cx, cy, cz, changeColour);
  switch(populated) {
  case -1:
    // Populated location! Ignore it and don't paint;
    // just change its colour in case
    return;
  case -2:
    // Empty location! Draw the voxel in it
    n_voxels++;
    voxels = (struct TVoxel*)realloc(voxels, sizeof(struct TVoxel)*n_voxels);
    populated = n_voxels - 1;
    break;
  default:
    // Non dirty position, i.e., old voxel location but now it's free
    n_voxels_non_dirty--;
    break;
  }

  voxels[populated].colour = colour;
  voxels[populated].x = cx;
  voxels[populated].y = cy;
  voxels[populated].z = cz;
  voxels[populated].dirty = 1;

  n_colours = countColours();
}

void removeLastVoxel() {
  if(voxels && (n_voxels > 0)) {
    voxels = (struct TVoxel*)realloc(voxels, sizeof(struct TVoxel)*n_voxels-1);
    n_voxels--;
    n_colours = countColours();
  }
}

void removeVoxel(struct TVoxel* voxel) {
  voxel->dirty = 0;
  n_colours = countColours();
  n_voxels_non_dirty++;
}

void loadModel(char *filename) {
  FILE *f;
  char line[64];
  int colour, x, y, z;

  if(!(f=fopen(filename, "r"))) {
    fprintf(stderr, "Error opening the requested model.\n");
    return;
  }

  cleanCanvas();

  while(fgets(line, 64, f) != NULL) {
    switch(line[0]){
    case 'v':
      sscanf(&line[2], "%d %d %d %d", &colour, &x, &y, &z);
      addVoxel(&colours[colour], x, y * (-1), z);
      break;
    case '#':
      // Comment or something; ignore it
    default:
      break;
    }
  }

  fclose(f);
}

void saveModel(char *filename) {
  FILE *f;
  int i;
  int half_voxel_size = voxel_size / 2;

  if(!(f=fopen(filename, "w"))) {
    fprintf(stderr, "Error opening the requested model.\n");
    return;
  }

  fprintf(f,
          "# +-----------------------------------------------------------+\n"
          "# | Augmented Reality Voxel Model exported from ARVoxelEditor |\n"
          "# +-----------------------------------------------------------+\n"
          "# * Number of voxels: %d\n"
          "# * Number of colours: %d\n\n",
          n_voxels, n_colours);

  for(i = 0; i < n_voxels; ++i) {
    struct TVoxel* v = &voxels[i];
    if(!v->dirty)
      continue;

    fprintf(f, "v %d %d %d %d\n",
            v->colour->index,
            (v->x - half_voxel_size) / voxel_size,
            (v->y + half_voxel_size) / voxel_size * (-1),
            (v->z - half_voxel_size) / voxel_size);
  }

  fclose(f);
}

int isPopulated(int x, int y, int z, void (*cb)(struct TVoxel *voxel)) {
  int i;

  for(i = 0; i < n_voxels; ++i) {
    struct TVoxel *v = &voxels[i];
    if((v->x == x) && (v->y == y) && (v->z == z)) {
      if(v->dirty) {
        if(cb)
          (*cb)(v);
        return -1;
      }
      else {
        return i;
      }
    }
  }

  return -2;
}

int countColours() {
  enum EColour unique[n_voxels];
  int total = 0;
  int i;

  total = 0;
  for(i = 0; i < n_voxels; ++i) {
    if(!voxels[i].dirty)
      continue;

    enum EColour sv = voxels[i].colour->index;
    int j;
    for(j = 0; j < total; ++j)
      if(sv == unique[j])
        break;

    if(total == j) {
      unique[total] = sv;
      ++total;
    }
  }

  return total;
}

void changeColour(struct TVoxel* voxel) {
  voxel->colour = brush.colour;
}

void printText(float r, float g, float b, int x, int y, void *font, char *string, int top) {
  int i, len;

  argDrawMode2D();

  if(top) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, dim[0], dim[1], 0.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
  }

  glColor3f(r, g, b);
  glRasterPos2i(x, y);
  for (i = 0, len = (int)strlen(string); i < len; ++i) {
    if(string[i] != '\n' ) {
      glutBitmapCharacter(font, string[i]);
    }
    else {
      y += 14;
      glRasterPos2i(x, y);
    }
  }

  if(top) {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
  }
}

void drawPlane(struct TColour* colour,
               float x1, float y1, float x2, float y2,
               float z) {
  glColor3ub(colour->r, colour->g, colour->b);
  glBegin(GL_QUADS);
  glVertex3f(x1, y1, z);
  glVertex3f(x2, y1, z);
  glVertex3f(x2, y2, z);
  glVertex3f(x1, y2, z);
  glEnd();
}

void drawSquare(float size,
                struct TColour* colour,
                float x, float y, float z) {
  glColor3ub(colour->r, colour->g, colour->b);
  glBegin(GL_QUADS);
  glVertex3f(x, y, z);
  glVertex3f(x+size, y, z);
  glVertex3f(x+size, y+size, z);
  glVertex3f(x, y+size, z);
  glEnd();
}

void drawCircle(float size,
                struct TColour* colour,
                float x, float y, float z,
                int xy, int xz, int yz) {
  int i;
  int triangleAmount = 20;
  float radius = size / 2.0f;

  float twicePi_divided_by_triangles = 2.0f * 3.1416 / triangleAmount;
  glColor3ub(colour->r, colour->g, colour->b);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(x, y, z);
  for(i = 0; i <= triangleAmount; ++i) {
    float cached = i * twicePi_divided_by_triangles;
    glVertex3f(x + (radius * cos(cached) * xy) + (radius * cos(cached) * xz),
               y + (radius * sin(cached) * xy) + (radius * cos(cached) * yz),
               z + (radius * sin(cached) * xz) + (radius * sin(cached) * yz));
  }
  glEnd();
}

void drawLine(float size,
              struct TColour* colour,
              float x1, float y1, float z1,
              float x2, float y2, float z2) {
  glLineWidth(size);
  glColor3ub(colour->r, colour->g, colour->b);
  glBegin(GL_LINES);
  glVertex3f(x1, y1, z1);
  glVertex3f(x2, y2, z2);
  glEnd();
}

void drawCube(float size,
              struct TColour* colour,
              float x, float y, float z,
              int wired) {
  glLineWidth(1);
  glPushMatrix();
  glTranslatef(x, y, z);
  glColor3ub(colour->r, colour->g, colour->b);
  (wired)?glutWireCube(size):glutSolidCube(size);
  glPopMatrix();
}

void drawReference() {
  int i;

  // Plane
  drawPlane(&colours[WHITE],  0.0f, 0.0f, PAPER_HEIGHT, -PAPER_WIDTH, -0.004f);

  // Axis
  drawLine(5.0f,  &colours[RED],   0.0f, 0.0f, 0.0f,  PAPER_HEIGHT, 0.0f, 0.0f);
  drawLine(5.0f,  &colours[LIME],  0.0f, 0.0f, 0.0f,  0.0f, -PAPER_WIDTH, 0.0f);
  drawLine(5.0f,  &colours[BLUE],  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, PAPER_WIDTH);

  // Grid
  for(i = 0; i <= grid_height; ++i)
    drawLine(2.0f,  &colours[GRAY],   (i * voxel_size), 0.0f, 0.0f,  (i * voxel_size), -PAPER_WIDTH, 0.0f);
  for(i = 0; i <= grid_width; ++i)
    drawLine(2.0f,  &colours[GRAY],   0.0f, (-i * voxel_size), 0.0f,  PAPER_HEIGHT, (-i * voxel_size), 0.0f);
}

void drawBrush() {
  // Distances between the plane and the brush
  double m[3][4], m2[3][4];
  float x, y, z;
  arUtilMatInv(mMarker->trans, m);
  arUtilMatMul(m, brush.parent->patt_trans, m2);
  x = m2[0][3]; y = m2[1][3]; z = m2[2][3];

  int half_voxel_size = voxel_size / 2;
  int ix = roundNum(x / voxel_size) * voxel_size + half_voxel_size;
  int iy = roundNum(y / voxel_size) * voxel_size - half_voxel_size;
  int iz = roundNum(z / voxel_size) * voxel_size + half_voxel_size;

  // Wired cube following the marker...
  (*brush.draw)(voxel_size, brush.colour, ix, iy, iz, 1);
  // ... and the shadow
  drawSquare(voxel_size, brush.colour, ix-half_voxel_size, iy-half_voxel_size, 0.0f);

  // Projected lines
  drawLine(1.0f,  &colours[RED],   ix, iy, iz,  0.0f, iy, iz);
  drawLine(1.0f,  &colours[LIME],  ix, iy, iz,  ix, 0.0f, iz);
  drawLine(1.0f,  &colours[BLUE],  ix, iy, iz,  ix, iy, 0.0f);

  // Projected circles
  drawCircle(5.0f,  &colours[RED],   0.0f, iy, iz,  0, 0, 1);
  drawCircle(5.0f,  &colours[LIME],  ix, 0.0f, iz,  0, 1, 0);
  drawCircle(5.0f,  &colours[BLUE],  ix, iy, 0.0f,  1, 0, 0);

  if(brush.put_voxel) {
    brush.put_voxel = 0;
    addVoxel(brush.colour, roundNum(x/voxel_size), roundNum(y/voxel_size), roundNum(z/voxel_size));
  }
  else if(brush.remove_voxel) {
    brush.remove_voxel = 0;
    isPopulated(ix, iy, iz, removeVoxel);
  }
}

void draw() {
  double gl_para[16];
  GLfloat mat_ambient[]     = {1.0, 1.0, 1.0, 1.0};
  GLfloat mat_diffuse[]     = {0.0, 0.0, 0.0, 1.0};
  GLfloat light_position[]  = {100.0, -200.0, 200.0, 0.0};
  int i;

  argDrawMode3D();
  argDraw3dCamera(0, 0);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  argConvGlpara(mMarker->trans, gl_para);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixd(gl_para);

  // Draw the canvas and the axis
  drawReference();

  // Draw attached models of the patterns
  // Just wired cubes at the moment, but ready to scale to
  // other figures
  for(i = 0; i < n_objects; ++i) {
    struct TObject* obj = &objects[i];

    if(obj->visible && obj->draw) {
      if(obj->id == BRUSH_PATT) {
        brush.parent = obj;
      }

      obj->draw();
    }
  }

  // Draw stored voxels with illumination; disable it
  // to draw shadows and other stuff
  int half_voxel_size = voxel_size / 2;
  for(i = 0; i < n_voxels; ++i) {
    struct TVoxel* v = &voxels[i];

    if(v->dirty) {
      // The voxel
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      glLightfv(GL_LIGHT0, GL_POSITION, light_position);

      glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
      mat_diffuse[0] = v->colour->r / 255.0f;
      mat_diffuse[1] = v->colour->g / 255.0f;
      mat_diffuse[2] = v->colour->b / 255.0f;
      glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
      drawCube(voxel_size, v->colour, v->x, v->y, v->z, 0);

      glDisable(GL_LIGHT0);
      glDisable(GL_LIGHTING);

      // The "shadow" over the canvas
      drawSquare(voxel_size, &colours[LIGHT_GRAY], v->x-half_voxel_size, v->y-half_voxel_size, 0.002f);
    }
  }

  glDisable(GL_DEPTH_TEST);
}

void cleanCanvas() {
  if(voxels) {
    free(voxels);
    voxels = NULL;
    n_voxels = 0;
    n_voxels_non_dirty = 0;
    n_colours = 0;
  }
}

void cleanup() {
  arVideoCapStop();
  arVideoClose();
  argCleanup();
  free(objects);
  free(voxels);
  exit(0);
}
