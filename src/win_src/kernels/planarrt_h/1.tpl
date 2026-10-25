#TMC_RT_H

#include planrt2.h

#STEP

#PARM
 ANGLE_UNIT radian;
 FREQ_UNIT GHz;
 LONG_UNIT mm;
 TIME_UNIT s;
 DELTA 0.35;
 TIME -.0001; 0.0001;
 X_MIN -10.0*2-4 ;
 X_MAX  ( + 500 . 0 ) ;
 Y_MIN  (-5.0);
 Y_MAX  300.0;
 FREQ  40.0;
#END_PARM

#TOPOLOGY

rect    9  ;   - (x      ^ 2+y     ^ 2 ) ;  -0.4 ; 2.5; -1.2; 6 . 3;

 BLOCK 1;
! прямоугольник,
! описывающий стационарное распределение добавки диэл.проницаемости
! RECT_STAT E( x, y); x_min; x_max; y_min; y_max;
 RECT_STAT   - ( x            ^ 2 + y^2 ) ; - 1 ; 3  ; -2; 4;
 END_B

 BLOCK 2;
! сектор,
! описывающий стационарное распределение добавки диэл.проницаемости
! CIRCLE_STAT E( r, f); r_min; r_max; f_min; f_max;
 CIRCLE_STAT  - ( 3 - r ) ; 0; 3; 0; 2*3.14159;
 END_B

 BLOCK 3;
! прямоугольник,
! описывающий распределение добавки диэл. проницаемости,
! двигающейся с линейной скоростью Vx, Vy
! и угловой скоростью w
! линейная скорость задаётся в LONG_UNIT/TIME_UNIT из секции параметров
! угловая скорость задаётся в ANGLE_UNIT/TIME_UNIT из секции параметров
! RECT_MOVE E( x, y); x_min; x_max; y_min; y_max; Vx; Vy; w;
 RECT_MOVE - ( x^2 + y^2 ) ; -1; 3; -2; 4; 1.0; -2; 10;
 END_B

 BLOCK 4;
! сектор,
! описывающий распределение добавки диэл. проницаемости
! двигающейся с линейной скоростью Vx, Vy
! и угловой скоростью w
! линейная скорость задаётся в LONG_UNIT/TIME_UNIT из секции параметров
! угловая скорость задаётся в ANGLE_UNIT/TIME_UNIT из секции параметров
! CIRCLE_MOVE E( r, f); r_min; r_max; f_min; f_max; Vx; Vy; w;
 CIRCLE_MOVE - ( 3 - r ) ; 0; 3; 0; 2*3.14159; -1.2; 3.4; 2;
 END_B

 BLOCK 5;
! выпуклый многоугольник (односвязанная область),
! описывающий стационарное распределение добавки диэл.проницаемости
! POLYGON_STAT E( x, y);
!  L x; y;
! E( x, y) - металл (METAL), красное тело (ABSORBER); распр. добавки eps;
! обход многоугольника по часовой стрелке
 POLYGON_STAT - ( 3 - x + y );

  L -2; 0;

  L  0; 1;
  L  2; 0;
 END_B

 BLOCK 6;
! выпуклый многоугольник (односвязанная область),
! описывающий распределение добавки диэл.проницаемости
! двигающейся с линейной скоростью Vx, Vy
! и угловой скоростью w
! линейная скорость задаётся в LONG_UNIT/TIME_UNIT из секции параметров
! угловая скорость задаётся в ANGLE_UNIT/TIME_UNIT из секции параметров
! POLYGON_MOVE E( x, y); Vx; Vy; w;
!  L x; y;
! E( x, y) - металл (METAL), красное тело (ABSORBER); распр. добавки eps;
! обход многоугольника по часовой стрелке
 POLYGON_MOVE - ( 3 - x + y ); 25; 36; 5;
  L -2; 0;
  L  0; 1;
  L  2; 0;
 END_B

 BLOCK 7;
! вход,
! описывающий возбуждение вдоль оси X
! INPUT_X  U( y ); y_min; y_max; t_min; t_max;
 INPUT_X sin(y*3.141592653589); 0.0; 1.0; 0; 2;
END_B

BLOCK 8;
! вход,
! описывающий возбуждение вдоль оси Y
! INPUT_Y  U( x ); x_min; x_max; t_min; t_max;
 INPUT_Y sin(x*3.141592653589); 0.0; 1.0; 0; 2;
END_B

#END_TOPOLOGY

#LINK_LIST
! начало координат block n совмещается с точками x0, y0
!глобальной системы координат из секции #PARAM
! T n; x0; y0;
 T 1; -1.0; -0.5;
 T 2; -1.0; -0.5;
 T 3; 1.4;  0.5;
 T 4; 1.7; -0.5;
 T 5; 3.7; -0.5;
 T 6; 3.7; -1.5;
 T 7; -24; 1.0;
 T 8; -25; -5.1;
 T 9; 0.0; 0.0;
#END_LINK

#OUTPUT
 FILE out1;
 FIELDS;
 TOPOLOGY;
#END_OUTPUT

#END_STEP


#STEP

#TOPOLOGY

BLOCK 2;
 CIRCLE_STAT  METAL; 0; 3; 0; 2*3.141592653589;
 END_B

 BLOCK 3;
 RECT_MOVE ABSORBER; -1; 3; -2; 4; 1.0; -2; 10;
 END_B

 BLOCK 4;
 CIRCLE_MOVE ABSORBER; 0; 3; 0; 2*3.141592653589; -1.2; 3.4; 4.6;
 END_B

 BLOCK 5;
 POLYGON_STAT METAL;
  L -2; 0;
  L  0; 1;
  L  2; 0;
 END_B

 BLOCK 6;
 POLYGON_MOVE - ( 3 - x + y ); -5.6; -24.; -4;
  L -2; 0;
  L  0; 1;
  L  2; 0;
 END_B

 BLOCK 7;
! вход,
! описывающий возбуждение вдоль оси X
! INPUT_X  U( y ); y_min; y_max; t_min; t_max;
 INPUT_X sin(y*3.141592653589); 0.0; 1.0; 0; 2;
END_B

BLOCK 8;
! вход,
! описывающий возбуждение вдоль оси Y
! INPUT_Y U( x ); x_min; x_max; t_min; t_max;
 INPUT_Y sin(x*3.141592653589); 0.0; 1.0; 0; 2;
END_B

#END_TOPOLOGY

#LINK_LIST
 T 2; -1.0; -0.5;
 T 3; 1.4;  0.5;
 T 4; 1.7; -0.5;
 T 5; 3.7; -0.5;
 T 6; 3.7; -1.5;
 T 7; -24; 1.0;
 T 8; 2; -5.;
#END_LINK

#OUTPUT
 FILE out12;
! TOPOLOGY
! FIELD
#END_OUTPUT

#END_STEP

#EOF

