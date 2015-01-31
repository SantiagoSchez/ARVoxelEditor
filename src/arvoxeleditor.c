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

#include <GL/glut.h>

#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/arMulti.h>

#include <math.h>
#include <unistd.h>

#include "colours.h"
#include "functions.h"
#include "structs.h"

void init(char *arg) {
  ARParam  wparam, cparam;
  double c[2] = {0.0, 0.0};

  // Open video device
  if(arVideoOpen(arg) < 0) exit(0);
  if(arVideoInqSize(&dim[0], &dim[1]) < 0) exit(0);

  // Load intrinsics parameters of the camera
  if(arParamLoad("data/camera_para.dat", 1, &wparam) < 0)
    ERROR("Error loading camera parameters");

  // Initialize camera
  arParamChangeSize(&wparam, dim[0], dim[1], &cparam);
  arInitCparam(&cparam);

  // Load brush marker
  addObject("data/simple.patt", BRUSH_PATT, 120.0, c, drawBrush);

  // Load multimarker file (canvas)
  if((mMarker = arMultiReadConfigFile("data/marker.dat")) == NULL)
    ERROR("Error in marker.dat file");

  // Some brush initialization
  brush.colour = &colours[BLACK];
  brush.draw = drawCube;

  // Open the window
  argInit(&cparam, 1.0, 0, 0, 0, 0);
}

void mainLoop() {
  static int useCont = 0;

  ARUint8 *dataPtr;
  ARMarkerInfo *marker_info;
  int marker_num, i, j, k;

  // Frame grab from camera
  if((dataPtr = (ARUint8 *)arVideoGetImage()) == NULL) {
    // No new frame ready
    arUtilSleep(2);
    return;
  }

  // Draw the frame
  argDrawMode2D();
  argDispImage(dataPtr, 0,0);

  // Detect the marker on the frame (error = -1)
  if(arDetectMarker(dataPtr, 100, &marker_info, &marker_num) < 0) {
    cleanup();
    exit(0);
  }

  // Grab the next video frame
  arVideoCapNext();

  // Match the most appropriate pattern in the detected markers
  for(i = 0; i < n_objects; ++i) {
    for(j = 0, k = -1; j < marker_num; ++j) {
      if(objects[i].id == marker_info[j].id) {
        if(k == -1) k = j;
        else if(marker_info[k].cf < marker_info[j].cf) k = j;
      }
    }

    // Pattern detected?
    if(k != -1) {
      objects[i].visible = 1;

      // Using the ARToolKit built in historic
      if(useCont) {
        arGetTransMatCont(&marker_info[k], objects[i].patt_trans,
                          objects[i].center, objects[i].width,
                          objects[i].patt_trans);
      }
      else {
        useCont = 1;
        arGetTransMat(&marker_info[k], objects[i].center,
                      objects[i].width, objects[i].patt_trans);
      }
    } else {
      objects[i].visible = 0;
    }
  }

  // If canvas is detected, draw all
  if(arMultiGetTransMat(marker_info, marker_num, mMarker) > 0)
    draw();

  // Print the menu and some information
  menu();

  argSwapBuffers();
}

int main(int argc, char **argv) {
  // Using GLUT for windowing stuff
  glutInit(&argc, argv);

  // ./arvoxeleditor [video_device=""] [voxel_size=16]
  switch(argc) {
  case 1:
    init("");
    break;
  case 2:
    init(argv[1]);
    break;
  default:
    voxel_size = atoi(argv[2]);
    grid_height = PAPER_HEIGHT / voxel_size;
    grid_width = PAPER_WIDTH / voxel_size;
    init(argv[1]);
    break;
  }

  arVideoCapStart();
  argMainLoop(NULL, keyboard, mainLoop);

  return 0;
}
