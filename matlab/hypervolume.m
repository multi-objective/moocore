%2D Example;
n = 1000;
theta = linspace(-3*pi/2,-pi,n)';
FP    = [1+cos(theta) 1-sin(theta)];
Res  = Hypervolume_MEX(FP,[1 1])

%3D Example;
n     = 500;
[X,Y] = meshgrid([0:1/n:1]);
X=X(:);Y=Y(:);
R = X.^2+Y.^2;
Ind = R<=1;
XX=X(Ind);
YY=Y(Ind);
ZZ=sqrt(1-R(Ind));
Res  = Hypervolume_MEX([XX,YY,ZZ],[1 1 1])
