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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <AR/arMulti.h>

#define PAPER_HEIGHT 297
#define PAPER_WIDTH 210
#define ERROR(msg, args...) { fprintf(stderr, msg, ##args); exit(1); }

struct TVoxel;
struct TColour;

// Multimarker data structure used for reference displaying
extern ARMultiMarkerInfoT *mMarker;
// Resolution of the camera. Usually 640x480
extern int dim[2];
// Size of the voxel. Usually 8, 16, 32, ...
extern int voxel_size;
// Size of the grid of the canvas: PAPER_{WIDTH,HEIGHT} / voxel_size
extern int grid_height, grid_width;
// "List" of markers
extern struct TObject *objects;
// Number of markers
extern int n_objects;
// "List" of voxels
extern struct TVoxel *voxels;
// Number of voxels
extern int n_voxels;
// Number of disty voxels, i.e., old voxel locations which are now free
extern int n_voxels_non_dirty;
// The brush
extern struct TBrush brush;
// The current colour we have selected
extern unsigned char colour_index ;
// The number of colours in the canvas
extern int n_colours;
// Control variable to check whether command line is active
extern int is_input;
// Stores the last characted pressed. character[1] = '\0' to be strcat friendly
extern char character[2];

// Round the given number: 0.9 = 1, 0.4 = 0.
inline int roundNum(float num);

// Prints the menu and some useful information
void menu();
// Command line management
void input();
// Adds a new marker to the "list"
void addObject(char *p, int patt_id, double w, double c[2], void (*draw)(void));
// Adds a new voxel to the "list" and consequently to the canvas
void addVoxel(struct TColour* colour, int x, int y, int z);
// Removes the last added voxel from the "list" and consequently from the canvas
// Used with the 'undo' feature
void removeLastVoxel();
// Removes the given voxel from the "list" and consequently from the canvas
void removeVoxel(struct TVoxel* voxel);
// Loads a model from disk and draw onto the canvas
void loadModel(char *filename);
// Saves the drawed model to disk
void saveModel(char *filename);

// Callbacked function to handle the keyboard
void keyboard(unsigned char key, int x, int y);

// Checks whether he given location is populated by a voxel
// In case it is, a provided callback function can be executed for that voxel
// - Returns:
//     >= 0 the index of the 'voxels' array the voxel is occupying
//       -1 location is populated
//       -2 location is empty
int isPopulated(int x, int y, int z, void (*cb)(struct TVoxel *voxel));
// Returns the number of colours of the drawed model
int countColours();
// Changes the colour of the given voxel by the current selected
// Usually used as a callback to 'isPopulated'
void changeColour(struct TVoxel* voxel);
// Prints 2D text on the screen
// If top = 1, the text is printed with (0, 0) location as top-left corner
// If top = 0, the text is printer with (0, 0) location as bottom-left corner
void printText(float r, float g, float b, int x, int y, void *font, char *string, int top);
// Draws a plane
void drawPlane(struct TColour* colour, float x1, float y1, float x2, float y2, float z);
// Draws a square
void drawSquare(float size, struct TColour* colour, float x, float y, float z);
// Draws a circle. xy, xz, yz are the planes the circle is drawed
// For example, if xy = 1, xz = 0, yz = 0, the circle will be drawed over the XY plane
void drawCircle(float size, struct TColour* colour, float x, float y, float z, int xy, int xz, int yz);
// Draws a line
void drawLine(float size, struct TColour* colour, float x1, float y1, float z1, float x2, float y2, float z2);
// Draws a cube. wired = 1 the cube is wired else it's solid
void drawCube(float size, struct TColour* colour, float x, float y, float z, int wired);
// Draws the reference of the multimarker, i.e., the canvas and axis
void drawReference();
// Draws the brush marker, i.e., the wired voxel, the shadow and axis
void drawBrush();
// Draws the whole scene
void draw();
// Removes all the voxels from the canvas
void cleanCanvas();

// Release all the memory from the program
void cleanup();

#endif
