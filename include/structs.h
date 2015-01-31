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

#ifndef STRUCTS_H
#define STRUCTS_H

/**
 * An enum for the available patterns. Currently just one pattern;
 * scalable for other patterns (colour wheel for example? eraser?)
 */
enum PATT_ID { BRUSH_PATT };

/**
 * Represent a pattern to be identified by ARToolKit
 */
struct TObject {
  int id;                    // Pattern ID
  int visible;               // Whether the object is visible
  double width;              // Width of the pattern (cm)
  double center[2];          // Center of the pattern
  double patt_trans[3][4];   // Pattern matrix
  void (*draw)(void);        // Draw function to be executed on pattern matching
};

/**
 * The brush used to place voxels on the canvas
 */
struct TBrush {
  int put_voxel;                           // Control flag to put a voxel
  int remove_voxel;                        // Control flag to remove a voxel
  struct TColour* colour;                  // The current colour of the voxel
  struct TObject* parent;                  // The pattern associated to the brush

  /**
   * Draw function actually used to put voxels.
   * Pointer function to be used as any draw call
   * in any possible future (spherical voxels you say?)
   */
  void (*draw)(float size,                 // Size of the object
               struct TColour* colour,     // Colour
               float x, float y, float z,  // Position
               int wired);                 // Wired (1) or solid (0)?
};

/**
 * A simple voxel data structure
 */
struct TVoxel {
  struct TColour* colour; // Colour
  int x, y, z;            // Location as integer to be mapped to the grid
  int dirty;              // Whether the voxel exists or can be replaced by another
};

#endif
