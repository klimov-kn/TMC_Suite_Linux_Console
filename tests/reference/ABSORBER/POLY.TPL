#TMC_RT_H

#define L_waveg @ (  200.00)
#define W_waveg @ (  200.00)
#define W_input @ (   70.00)
#define W_delta @ (    2.00)

#define a1 @ ( W_input )
#define a2 @ ( a1*0.72 )

#define h1 @ ( 20 )
#define l1 @ ( 20 )


#STEP

#PARM
 ANGLE_UNIT radian;
 FREQ_UNIT GHz;
 LONG_UNIT mm;
 TIME_UNIT s;
 DELTA W_delta;
 TIME 0.; 200.00/42397056;
 X_MIN  0.0;
 X_MAX L_waveg;
 Y_MIN  0.0;
 Y_MAX W_waveg;
 FREQ  10.0;
#END_PARM

#TOPOLOGY

 BLOCK 7;
  INPUT_X  ; -a1/2; a1/2; 0; 2;
 END_B
 BLOCK 15;
  POLYGON_STAT ABSORBER;
   L   0.0;    a1/2;
   L   -h1;    a1/2-l1;
   L   0.0;    a1/2-2*l1;
   L   -h1;    a1/2-3*l1;
   L   0.0;    a1/2-4*l1;
   L   -h1;    a1/2-5*l1;
   L   0.0;    a1/2-6*l1;
   L   -h1;    a1/2-7*l1;
   L   0.0;    a1/2-8*l1;
   L   -h1;    a1/2-9*l1;
   L   0.0;    a1/2-10*l1;
   L   -h1;    a1/2-11*l1;
   L   0.0;    a1/2-12*l1;
  END_B
 BLOCK 1;
  RECT_STAT METAL; 0; (L_waveg-a1)/2; -a1/2; -a1/2+0.1;
 END_B
 BLOCK 2;
  RECT_STAT METAL; 0; (L_waveg+a1)/2; a1/2; a1/2+0.1;
 END_B
 BLOCK 3;
  RECT_STAT METAL; (L_waveg+a1)/2-0.1; (L_waveg+a1)/2; -a1/2-W_waveg/2; a1/2+0.1;
 END_B
 BLOCK 4;
  RECT_STAT METAL; (L_waveg-a1)/2-0.1; (L_waveg-a1)/2; -a1/2-W_waveg/2; -a1/2+0.1;
 END_B
 BLOCK 5;
  POLYGON_STAT ABSORBER;
   L   (L_waveg+a1)/2-a2;    a1/2;
   L   (L_waveg+a1)/2;       a1/2;
   L   (L_waveg+a1)/2;    a1/2-a2;
!   L   (L_waveg+a1)/2-a2;    a1/2-a2;
!   L   (L_waveg+a1)/2-a2;    a1/2+5*W_delta;
   L   (L_waveg+a1)/2-a2+0*W_delta;    a1/2+0*W_delta;
  END_B
 BLOCK 8;
  INPUT_Y  ; -a1/2; a1/2; 1; 2;
 END_B

#END_TOPOLOGY

#LINK_LIST
 T 1; 0.0; W_waveg/2;
 T 2; 0.0; W_waveg/2;
 T 3; 0.0; W_waveg/2;
 T 4; 0.0; W_waveg/2;
 T 5; -10*W_delta; W_waveg/2-10*W_delta;
 T 15; L_waveg/2+a1/2; W_waveg/2;
 T 7; 0.0; W_waveg/2;
 T 8; L_waveg/2; 0.0;
#END_LINK

#OUTPUT
 FILE poly;
 FIELDS;
! TOPOLOGY;
#END_OUTPUT

#END_STEP


#EOF


#STEP

#PARM
 X_MIN  0.0;
 X_MAX W_waveg;
 Y_MIN  0.0;
 Y_MAX L_waveg;
#END_PARM

#TOPOLOGY


 BLOCK 7;
  INPUT_Y sin(x*3.141592653589/(W_waveg)); 0.0; W_waveg; 0; 2;
 END_B

 BLOCK 8;
  INPUT_Y sin(x*3.141592653589/(W_waveg)); 0.0; W_waveg; 1; 2;
 END_B

#END_TOPOLOGY

#LINK_LIST
 T 7; 0.0; 0.0;
 T 8; 0.0; L_waveg;
#END_LINK

#OUTPUT
 FILE out2;
 FIELDS;
 TOPOLOGY;
#END_OUTPUT

#END_STEP



#STEP

#PARM
 DELTA 0.35;
 TIME 0.; 0.0001;
 X_MIN  0.0;
 X_MAX  400;
 Y_MIN  0.0;
 Y_MAX  300;
 FREQ  40.0;
#END_PARM

#TOPOLOGY

 BLOCK 1;
 CIRCLE_STAT 1-1.4*(1-(r/300)*(r/300))*(1-(r/300)*(r/300)); 0; 300; 0; 2*3.141592;
 END_B

 BLOCK 7;
  INPUT_X sin(y*3.141592653589/19); 0.0; 19.0; 0; 2; 
 END_B

#END_TOPOLOGY

#LINK_LIST
T 1; 0.0; 0.0;
T 7; 0.0; 100.0;
#END_LINK

#OUTPUT
 FILE out3;
 FIELDS;
 TOPOLOGY;
#END_OUTPUT

#END_STEP

#EOF

#STEP

#TOPOLOGY

BLOCK 1;
 RECT_STAT 1-1.4*(1-(r/300)*(r/300))*(1-(r/300)*(r/300)); 0; 300; 0; 300;
END_B

 BLOCK 7;
!     ,
!                                   X
! INPUT_X  U( y ); y_min; y_max; t_min; t_max;
 INPUT_X sin(y*3.141592653589/19); 0.0; 19.0; 0; 2;
END_B

BLOCK 8;
!     ,
!                                   Y
! INPUT_Y  U( x ); x_min; x_max; t_min; t_max;
 INPUT_Y sin(x*3.141592653589/19); 0.0; 19.0; 0; 2;
END_B

#END_TOPOLOGY

#LINK_LIST
!                  block n                       x0, y0
!                                       #PARAM
! T n; x0; y0;
T 1; 300.0; 300.0;
T 7; 0.0; 100.0;
T 8; 100; 0.0;
#END_LINK

#OUTPUT
 FILE out1;
 FIELDS;
 TOPOLOGY;
#END_OUTPUT

#END_STEP



#STEP

#TOPOLOGY

BLOCK 1;
 POLYGON_STAT 1-1.4*(1-(r/300)*(r/300))*(1-(r/300)*(r/300));
L   0;   0;
L   0; 300;
L 300; 300;
L 300;   0;
END_B

 BLOCK 7;
!     ,
!                                   X
! INPUT_X  U( y ); y_min; y_max; t_min; t_max;
 INPUT_X sin(y*3.141592653589/19); 0.0; 19.0; 0; 2;
END_B

BLOCK 8;
!     ,
!                                   Y
! INPUT_Y  U( x ); x_min; x_max; t_min; t_max;
 INPUT_Y sin(x*3.141592653589/19); 0.0; 19.0; 0; 2;
END_B

#END_TOPOLOGY

#LINK_LIST
!                  block n                       x0, y0
!                                       #PARAM
! T n; x0; y0;
T 1; 300.0; 300.0;
T 7; 0.0; 100.0;
T 8; 100; 0.0;
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
!     ,
!                                   X
! INPUT_X  U( y ); y_min; y_max; t_min; t_max;
 INPUT_X sin(y*3.141592653589); 0.0; 1.0; 0; 2;
END_B

BLOCK 8;
!     ,
!                                   Y
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

