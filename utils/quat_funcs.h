//
// Created by alvaregd on 26/11/15.
//

#ifndef FPS_STYLE_ROOM_QUAT_FUNCS_H
#define FPS_STYLE_ROOM_QUAT_FUNCS_H

void create_versor (float* q, float a, float x, float y, float z);
void quat_to_mat4 (float* m, float* q) ;
void normalise_quat (float* q);
void mult_quat_quat (float* result, float* r, float* s);


#endif //FPS_STYLE_ROOM_QUAT_FUNCS_H
