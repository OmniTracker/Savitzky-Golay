/*  
*   MatrixXi vander(const int F, double A[25])
*     -Computes the vandermonde matrix (the polynomial basis vectors) and flips it column-wise from left to right
*
*   MatrixXd B = MatrixXd sgdiff(int k, double F) 
*      - designs a Savitzky-Golay (polynomial) FIR smoothing
*   filter B.  The polynomial order, k, must be less than the frame size of the convolution coefficients,
*   F. F must be odd. 
*
*   Author: Olalekan Ogunmolu  
*           August 12, 2015
*   
    Reference: INTRODUCTION TO SIGNAL PROCESSING [Chapter 8; Section 8.3.5]
                Sophocles J. Orfanidis, Prentice Hall, 2010
*/

// Include Files
#include "savgol.h"
#include <iostream>
#include <Eigen/LU>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/QR>

using namespace Eigen;
using namespace std;

//Global Variables
int F = 5;      //Frame Size
int k = 3;      //Example Polynomial Order
double Fd = (double) F;        //sets the frame size for the savgol differentiation coefficients. This must be odd

// Function Prototypes
//MatrixXi vander(const int F, double A[25]);
MatrixXi vander(const int F);
MatrixXd sgdiff(int k, double Fd);
void savgolfilt(VectorXf x, int k, int F, MatrixXd DIM);

class RMatrix
{
  public:
    MatrixXd R;

};

/*Compute the polynomial basis vectors s_0, s_1, s_2 ... s_n using the vandermonde matrix.
This is super-hacky!  
*/
MatrixXi vander(const int F)
{
  VectorXi v = VectorXi::LinSpaced(F,(-(F-1)/2),((F-1)/2)).transpose().eval();

  MatrixXi A(F, F+1);     //We basically compute an F X F+1 matrix;

  for(int i = 0; i < F; ++ i)
  {
    for(int j=1; j < F+1; ++j)
    {
      A(i,j) = pow(v(i), (j-1) ); 
    }
  }

  A = A.block(0, 1, F, F );   //and retrieve the right F X F block to find the vandermonde matrix.

  return A;
}

/*Compute the S-Golay Matrix of differentiators*/
MatrixXd sgdiff(int k, double Fd)
{
  //We set the weighting matrix to an identity matrix if no weighting matrix is supplied
  MatrixXd W = MatrixXd::Identity(Fd, Fd);      

  //Compute Projection Matrix B
  MatrixXi s = vander(F);   

  //Retrieve vandermonde block from projection matrix
  MatrixXi S = s.block(0, 0, s.rows(), (k+1) ) ; 

  //Compute sqrt(W)*S
  MatrixXd Sd = S.cast<double> ();    //cast S to double
  MatrixXd inter = W * Sd;              //W is assumed to be identity. Change this if you have reasons to.

  //Compute the QR Decomposition
  HouseholderQR<MatrixXd> qr(inter);
  qr.compute(inter);

  FullPivLU<MatrixXd>lu_decomp(inter);      //retrieve rank of matrix
  
  int Rank = lu_decomp.rank() ;
   
  //For rank deficient matrices. The S matrix block will always be rank deficient.        
  MatrixXd Q = qr.householderQ();
  MatrixXd R = qr.matrixQR().topLeftCorner(Rank, Rank).template triangularView<Upper>();

  //Compute Matrix of Differentiators
  MatrixXd Rinv = R.inverse();
  MatrixXd RinvT = Rinv.transpose();

  MatrixXd G = Sd * Rinv * RinvT;           /*G = S(S'S)^(-1)   -- eqn 8.3.90 (matrix of differentiation filters)*/
 
  MatrixXd SdT = Sd.transpose().eval();

  MatrixXd B = G * SdT * W;   //SG-Smoothing filters of length F and polynomial order k

  return B;
}

void savgolfilt(VectorXf x, int k, int F)
{  
  Matrix4d DIM = Matrix4d::Zero();        //initialize DIM as a matrix of zeros if it is not supplied
  cout <<"\nDIM: \n" << DIM <<endl;
  //Reshape depth values by working along the first non-singleton dimension
  int siz = x.size();
  cout <<"\nSize of x: \n" << siz << endl;

  //Find leading singleton dimensions
  
  //Pre-allocate output vector
  VectorXd y(siz);
  cout <<"\ny: \n" << y << endl;

  MatrixXd B = sgdiff(k, Fd);       //retrieve matrix B

  //Compute Transient On
  int id_size = (F+1)/2 - 1;
  VectorXi y_on = VectorXi::LinSpaced(id_size, 1, (F+1)/2-1) ;
  cout << "\ny_on: \n" << y_on << endl;

 /*In smoothing a long block of data, we will use the the filter b_0 during the steady state period */
  size_t idzeroth = floor(B.cols()/2);
  VectorXd Bzeroth = B.col(idzeroth);
  cout << "\nB: \n" << B <<
          "\n\nMiddle Column of B: \n" << Bzeroth <<
          "\n\nBzeroth dims: " << Bzeroth.rows() <<" X " << Bzeroth.cols() << 
          "\n\nx dims: " << x.rows() <<" X " << x.cols() << endl; 
  //compute y_0
  VectorXf Bzerothf = Bzeroth.cast<float>();
  y(0) = Bzerothf.transpose().eval() * x;
  cout << "\ny(0): " << y(0) << endl;

}


int main ()
{
  MatrixXi s = vander(F);        //Compute vandermonde matrix

  cout << "\n Vandermonde Matrix: \n" << s  << endl;

  MatrixXd B = sgdiff(k, Fd);

  VectorXf xinit = VectorXf::LinSpaced(5, 900, 980);

  //To express as a real filtering operation, we need to shift x around the nth time instant
  //write an infinite for/while loop
  VectorXf x = VectorXf::LinSpaced(5, 900, 980);

  savgolfilt(x, k, F);

  return 0;
}

/* Compile:
cd ../; rm build -rf; mkdir build; cd build; cmake ../; make; ./savgol
*/