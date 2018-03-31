%
% MATLAB script to output all symbolic math of the LQG.
%
syms A00 A10 A20 A01 A11 A21 A02 A12 A22;
syms P00 P10 P20 P01 P11 P21 P02 P12 P22;
syms Q00 Q10 Q20 Q01 Q11 Q21 Q02 Q12 Q22;
syms B0 B1 B2;
syms X0 X1 X2;
syms K0 K1 K2;
syms R;
syms beta tau u y;

A = [ A00 A01 A02 ; 0 A11 A12 ; 0 0 A22 ];
P = [ P00 P01 P02 ; P10 P11 P12 ; P20 P21 P22 ];
Q = [ Q00 0 0 ; 0 Q11 0 ; 0 0 Q22 ];
B = [ B0 ; B1 ; B2 ];
X = [ X0 ; X1 ; X2 ];
K = [ K0 ; K1 ; K2 ];
H = [ 1 0 0 ];
I = [ 1 0 0 ; 0 1 0 ; 0 0 1 ];

%
% Kalman stuff
%
disp('Kalman covariance cycle')
disp('=====================================')

disp('Kalman covariance P =')
simplify(A*P*transpose(A) + Q)

disp('Kalman gain K =')
simplify(P*transpose(H)*inv(R+H*P*transpose(H)))

disp('Kalman a posteriori cov P=')
simplify((I-K*H)*P)

disp('Kalman predictor')
disp('=====================================')
simplify(A*X + B*u + K*(y - H*X))

%
% LQR stuff
%
A = [ A00 A01 ; A10 A11 ];
P = [ P00 P01 ; P10 P11 ];
Q = [ Q00 0 ; 0 Q11  ];
B = [ B0 ; B1 ];

disp('LQR covariance cycle')
disp('=====================================')
disp('LQR covariance P =')
simplify(transpose(A)*P*A - transpose(A)*P*B*inv(R+transpose(B)*P*B)*transpose(B)*P*A + Q)

disp('LQR gain K=')
simplify(inv(R+transpose(B)*P*B)*transpose(B)*P*A)