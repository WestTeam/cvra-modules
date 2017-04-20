/*
 *  Copyright Droids Corporation, Microb Technology, Eirbot (2005)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Revision : $Id: trajectory_manager.c,v 1.4.4.17 2009-05-18 12:28:36 zer0 Exp $
 *
 */

/* Trajectory Manager v3 - zer0 - for Eurobot 2010 */


#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <tools.h>
//#include <platform.h>
//#include <aversive/error.h>
#include <vect2.h>

#include <2wheels/position_manager.h>
#include <2wheels/robot_system.h>
#include <control_system_manager.h>
#include <quadramp.h>

#include "trajectory_manager.h"
#include "trajectory_manager_utils.h"
#include "trajectory_manager_core.h"


/** set speed consign in quadramp filter */
void set_quadramp_speed(struct trajectory *traj, float d_speed, float a_speed)
{
    struct quadramp_filter * q_d, * q_a;
    q_d = traj->csm_distance->consign_filter_params;
    q_a = traj->csm_angle->consign_filter_params;
    quadramp_set_1st_order_vars(q_d, ABS(d_speed), ABS(d_speed));
    quadramp_set_1st_order_vars(q_a, ABS(a_speed), ABS(a_speed));
}

/** get angle speed consign in quadramp filter */
float get_quadramp_angle_speed(struct trajectory *traj)
{
    struct quadramp_filter *q_a;
    q_a = traj->csm_angle->consign_filter_params;
    return q_a->var_1st_ord_pos;
}

/** get distance speed consign in quadramp filter */
float get_quadramp_distance_speed(struct trajectory *traj)
{
    struct quadramp_filter *q_d;
    q_d = traj->csm_distance->consign_filter_params;
    return q_d->var_1st_ord_pos;
}

/** set speed consign in quadramp filter */
void set_quadramp_acc(struct trajectory *traj, float d_acc, float a_acc)
{
    struct quadramp_filter * q_d, * q_a;
    q_d = traj->csm_distance->consign_filter_params;
    q_a = traj->csm_angle->consign_filter_params;
    quadramp_set_2nd_order_vars(q_d, ABS(d_acc), ABS(d_acc));
    quadramp_set_2nd_order_vars(q_a, ABS(a_acc), ABS(a_acc));
}

/** remove event if any */
void delete_event(struct trajectory *traj)
{

    set_quadramp_speed(traj, traj->d_speed, traj->a_speed);
    set_quadramp_acc(traj, traj->d_acc, traj->a_acc);
    //OSTaskDel(TRAJ_EVT_PRIO);
}

/** schedule the trajectory event */
void schedule_event(struct trajectory *traj)
{

    /*traj->scheduler_task = TRAJ_EVT_PRIO;
    OSTaskCreateExt(trajectory_manager_event,
                    (void*)traj,
                    &traj->task_stk[2047],
                    TRAJ_EVT_PRIO,
                    TRAJ_EVT_PRIO,
                    &traj->task_stk[0],
                    2048,
                    NULL, NULL);*/
}

/** do a modulo 2.pi -> [-Pi,+Pi], knowing that 'a' is in [-3Pi,+3Pi] */
float simple_modulo_2pi(float a)
{
    if (a < -M_PI) {
        a += M_2PI;
    }
    else if (a > M_PI) {
        a -= M_2PI;
    }
    return a;
}

/** do a modulo 2.pi -> [-Pi,+Pi] */
float modulo_2pi(float a)
{
        float res = a - (((int32_t) (a/M_2PI)) * M_2PI);
    return simple_modulo_2pi(res);
}

/** near the target (dist) ? */
uint8_t is_robot_in_dist_window(struct trajectory *traj, float d_win)
{
    float d = traj->target.pol.distance - rs_get_distance(traj->robot);
    d = ABS(d);
/*    d = d / traj->position->phys.distance_imp_per_mm; */
    return (d < d_win);
}

/** near the target (dist in x,y) ? */
uint8_t is_robot_in_xy_window(struct trajectory *traj, float d_win)
{
    float x1 = traj->target.cart.x;
    float y1 = traj->target.cart.y;
    float x2 = position_get_x_float(traj->position);
    float y2 = position_get_y_float(traj->position);
    return ( __ieee754_sqrtf ((x2-x1) * (x2-x1) + (y2-y1) * (y2-y1)) < d_win );
}

/** near the angle target in radian ? Only valid if
 *  traj->target.pol.angle is set (i.e. an angle command, not an xy
 *  command) */
uint8_t is_robot_in_angle_window(struct trajectory *traj, float a_win_rad)
{
    float a;

    /* convert relative angle from imp to rad */
    a = traj->target.pol.angle - rs_get_ext_angle(traj->robot);
/*    a /= traj->position->phys.distance_imp_per_mm;
    a /= traj->position->phys.track_mm;
    a *= 2.;
*/
    return ABS(a) < a_win_rad;
}

enum trajectory_state trajectory_get_state(struct trajectory *traj)
{
    return traj->state;
}
/*
int trajectory_moving_backward(struct trajectory *traj) {
    return cs_get_consign(traj->csm_distance) < cs_get_filtered_consign(traj->csm_distance);
}

int trajectory_moving_forward(struct trajectory *traj) {
    return cs_get_consign(traj->csm_distance) > cs_get_filtered_consign(traj->csm_distance);
}

int trajectory_turning(struct trajectory *traj) {
    if(trajectory_moving_forward(traj)) return 0;
    if(trajectory_moving_backward(traj)) return 0;
    return cs_get_consign(traj->csm_angle) != cs_get_filtered_consign(traj->csm_angle);
}
*/


/* distance unit conversions */
#if 0
float pos_mm2imp(struct trajectory *traj, float pos)
{
    return pos * traj->position->phys.distance_imp_per_mm;
}

float pos_imp2mm(struct trajectory *traj, float pos)
{
    return pos / traj->position->phys.distance_imp_per_mm;
}

float speed_mm2imp(struct trajectory *traj, float speed)
{
    return speed *
        traj->position->phys.distance_imp_per_mm /
        traj->cs_hz;
}

float speed_imp2mm(struct trajectory *traj, float speed)
{
    return speed * traj->cs_hz /
        traj->position->phys.distance_imp_per_mm;
}

float acc_mm2imp(struct trajectory *traj, float acc)
{
    return acc * traj->position->phys.distance_imp_per_mm /
        (traj->cs_hz * traj->cs_hz);
}

float acc_imp2mm(struct trajectory *traj, float acc)
{
    return acc * traj->cs_hz * traj->cs_hz /
        traj->position->phys.distance_imp_per_mm;
}

/* angle unit conversions */

float pos_rd2imp(struct trajectory *traj, float pos)
{
    return pos *
        (traj->position->phys.distance_imp_per_mm *
         traj->position->phys.track_mm / 2.);
}

float pos_imp2rd(struct trajectory *traj, float pos)
{
    return pos /
        (traj->position->phys.distance_imp_per_mm *
         traj->position->phys.track_mm / 2.);
}

float speed_rd2imp(struct trajectory *traj, float speed)
{
    return speed *
        (traj->position->phys.distance_imp_per_mm *
         traj->position->phys.track_mm /
         (2. * traj->cs_hz));
}

float speed_imp2rd(struct trajectory *traj, float speed)
{
    return speed /
        (traj->position->phys.distance_imp_per_mm *
         traj->position->phys.track_mm /
         (2. * traj->cs_hz));
}

float acc_rd2imp(struct trajectory *traj, float acc)
{
    return acc *
        (traj->position->phys.distance_imp_per_mm *
         traj->position->phys.track_mm /
         (2. * traj->cs_hz * traj->cs_hz));
}

float acc_imp2rd(struct trajectory *traj, float acc)
{
    return acc /
        (traj->position->phys.distance_imp_per_mm *
         traj->position->phys.track_mm /
         (2. * traj->cs_hz * traj->cs_hz));
}
#endif 
