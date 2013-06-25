/*
 * skeltrack-smooth.h
 *
 * Skeltrack - A Free Software skeleton tracking library
 * Copyright (C) 2012 Igalia S.L.
 *
 * Authors:
 *  Joaquim Rocha <jrocha@igalia.com>
 *  Eduardo Lima Mitev <elima@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License at http://www.gnu.org/licenses/lgpl-3.0.txt
 * for more details.
 */

#include "skeltrack-joint.h"

typedef struct {
  SkeltrackJointList smoothed_joints;
  SkeltrackJointList trend_joints;
  guint joints_persistency;
  gfloat smoothing_factor;
  guint joints_persistency_counter[SKELTRACK_JOINT_MAX_JOINTS];
} SmoothData;

void    reset_joints_persistency_counter    (SmoothData *smooth_data);

void    smooth_joints                       (SmoothData           *data,
                                             SkeltrackJointList    new_joints);
